#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/wait.h>

#include "shared.h"


int create_shared(struct shared_data **data) {
    char filename[] = "/tmp/os_lab4_XXXXXX";
    int descriptor = mkstemp(filename);
    if (descriptor == -1) {
        perror("mkstemp");
        return -1;
    }

    struct shared_data *mapping = MAP_FAILED;
    int request_ready = 0;

    // файл остаётся доступен через унаследованный дескриптор
    if (unlink(filename) == -1) {
        perror("unlink");
        goto cleanup;
    }
    if (ftruncate(descriptor, sizeof(*mapping)) == -1) {
        perror("ftruncate");
        goto cleanup;
    }

    mapping = mmap(NULL, sizeof(*mapping), PROT_READ | PROT_WRITE,
        MAP_SHARED, descriptor, 0);
    if (mapping == MAP_FAILED) {
        perror("mmap");
        goto cleanup;
    }

    memset(mapping, 0, sizeof(*mapping));
    mapping->parent_pid = getpid();
    if (sem_init(&mapping->request, 1, 0) == -1) {
        perror("sem_init request");
        goto cleanup;
    }
    request_ready = 1;
    if (sem_init(&mapping->response, 1, 0) == -1) {
        perror("sem_init response");
        goto cleanup;
    }

    *data = mapping;
    return descriptor;

cleanup:
    if (request_ready && sem_destroy(&mapping->request) == -1) {
        perror("sem_destroy");
    }
    if (mapping != MAP_FAILED && munmap(mapping, sizeof(*mapping)) == -1) {
        perror("munmap");
    }
    if (close(descriptor) == -1) {
        perror("close");
    }
    return -1;
}

int close_shared(struct shared_data *data, int owner) {
    int result = 0;
    if (owner) {
        if (sem_destroy(&data->request) == -1) {
            perror("sem_destroy request");
            result = 1;
        }
        if (sem_destroy(&data->response) == -1) {
            perror("sem_destroy response");
            result = 1;
        }
    }
    if (munmap(data, sizeof(*data)) == -1) {
        perror("munmap");
        result = 1;
    }
    return result;
}

int wait_message(sem_t *semaphore, pid_t peer, int child_process) {
    for (;;) {
        struct timespec deadline;
        if (clock_gettime(CLOCK_REALTIME, &deadline) == -1) {
            return -1;
        }
        deadline.tv_sec++;

        if (sem_timedwait(semaphore, &deadline) == 0) {
            return 0;
        }
        if (errno == EINTR) {
            continue;
        }
        if (errno != ETIMEDOUT) {
            return -1;
        }

        // не ждём бесконечно, если второй процесс завершился
        int finished;
        if (child_process) {
            finished = getppid() != peer;
        } else {
            siginfo_t status = {0};
            if (waitid(P_PID, (id_t)peer, &status, WEXITED | WNOHANG | WNOWAIT) == -1) {
                if (errno == EINTR) {
                    continue;
                }
                return -1;
            }
            finished = status.si_pid != 0;
        }

        if (finished) {
            if (sem_trywait(semaphore) == 0) {
                return 0;
            }
            if (errno == EAGAIN) {
                errno = EPIPE;
            }
            return -1;
        }
    }
}

void set_error(struct shared_data *data, const char *operation, int error) {
    data->reply = REPLY_ERROR;
    snprintf(data->message, sizeof(data->message), "%s: %s", operation, strerror(error));
}
