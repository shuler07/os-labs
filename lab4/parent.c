#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>

#include "shared.h"


static int receive_reply(struct shared_data *data, pid_t child) {
    if (wait_message(&data->response, child, 0) == -1) {
        perror("wait response");
        return 1;
    }
    if (data->reply != REPLY_OK) {
        fprintf(stderr, "%s\n", data->message);
    }
    return data->reply == REPLY_ERROR;
}

static int send_message(struct shared_data *data, pid_t child) {
    if (sem_post(&data->request) == -1) {
        perror("sem_post request");
        return 1;
    }
    return receive_reply(data, child);
}

int main() {
    char *filename = NULL;
    size_t filename_capacity = 0;
    printf("Input output filename: ");
    if (fflush(stdout) == EOF) {
        perror("fflush stdout");
        return 1;
    }

    ssize_t filename_length = getline(&filename, &filename_capacity, stdin);
    if (filename_length == -1) {
        fprintf(stderr, "Error reading filename\n");
        free(filename);
        return 1;
    }
    if (filename_length > 0 && filename[filename_length - 1] == '\n') {
        filename[--filename_length] = '\0';
    }
    if (filename_length > 0 && filename[filename_length - 1] == '\r') {
        filename[--filename_length] = '\0';
    }
    if (filename_length == 0 || memchr(filename, '\0', (size_t)filename_length)) {
        fprintf(stderr, "Invalid filename\n");
        free(filename);
        return 1;
    }

    struct shared_data *data;
    int descriptor = create_shared(&data);
    if (descriptor == -1) {
        free(filename);
        return 1;
    }

    int result = 1;
    char *line = NULL;
    size_t capacity = 0;
    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        goto cleanup;
    }
    if (pid == 0) {
        char descriptor_string[32];
        snprintf(descriptor_string, sizeof(descriptor_string), "%d", descriptor);
        execl("./child.exe", "child.exe", descriptor_string, filename, (char *)NULL);
        perror("execl");
        _exit(1);
    }

    fprintf(stderr, "Parent PID: %ld, child PID: %ld\n", (long)getpid(), (long)pid);
    int close_result = close(descriptor);
    descriptor = -1;
    if (close_result == -1) {
        perror("close");
        goto cleanup;
    }
    if (receive_reply(data, pid) != 0) {
        goto cleanup;
    }

    printf("Enter strings:\n");
    if (fflush(stdout) == EOF) {
        perror("fflush stdout");
        goto cleanup;
    }

    ssize_t length;
    errno = 0;
    while ((length = getline(&line, &capacity, stdin)) != -1) {
        if (length > 0 && line[length - 1] == '\n') {
            length--;
        }

        // длинные строки передаём частями через одно отображение
        size_t offset = 0;
        do {
            size_t part = (size_t)length - offset;
            if (part > sizeof(data->buffer)) {
                part = sizeof(data->buffer);
            }
            memcpy(data->buffer, line + offset, part);
            offset += part;
            data->length = part;
            data->type = offset < (size_t)length ? MESSAGE_PART : MESSAGE_LINE;
            if (send_message(data, pid) != 0) {
                goto cleanup;
            }
        } while (offset < (size_t)length);
        errno = 0;
    }

    if (!feof(stdin)) {
        perror("getline");
        goto cleanup;
    }

    data->type = MESSAGE_FINISH;
    data->length = 0;
    if (send_message(data, pid) != 0) {
        goto cleanup;
    }
    result = 0;

cleanup:
    if (pid > 0) {
        if (result != 0 && kill(pid, SIGTERM) == -1 && errno != ESRCH) {
            perror("kill");
        }
        int status;
        pid_t waited;
        do {
            waited = waitpid(pid, &status, 0);
        } while (waited == -1 && errno == EINTR);
        if (waited == -1) {
            perror("waitpid");
            result = 1;
        } else if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
            fprintf(stderr, "Child process terminated with an error\n");
            result = 1;
        }
    }
    if (descriptor != -1 && close(descriptor) == -1) {
        perror("close");
        result = 1;
    }
    if (close_shared(data, 1) != 0) {
        result = 1;
    }
    free(filename);
    free(line);
    return result;
}
