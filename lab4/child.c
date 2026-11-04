#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <limits.h>
#include <errno.h>
#include <unistd.h>
#include <sys/mman.h>

#include "shared.h"


static int append_part(char **line, size_t *length, size_t *capacity,
        const char *buffer, size_t part) {
    if (part > SIZE_MAX - *length) {
        errno = EOVERFLOW;
        return 1;
    }

    size_t needed = *length + part;
    if (needed > *capacity) {
        size_t new_capacity = *capacity ? *capacity : 4096;
        while (new_capacity < needed) {
            if (new_capacity > SIZE_MAX / 2) {
                new_capacity = needed;
                break;
            }
            new_capacity *= 2;
        }
        char *new_line = realloc(*line, new_capacity);
        if (!new_line) {
            return 1;
        }
        *line = new_line;
        *capacity = new_capacity;
    }
    if (part > 0) {
        memcpy(*line + *length, buffer, part);
    }
    *length = needed;
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s shared_descriptor output_filename\n", argv[0]);
        return 1;
    }

    char *end;
    errno = 0;
    long value = strtol(argv[1], &end, 10);
    if (errno != 0 || end == argv[1] || *end != '\0' || value < 0 || value > INT_MAX) {
        fprintf(stderr, "Invalid shared descriptor\n");
        return 1;
    }
    int descriptor = (int)value;
    struct shared_data *data = mmap(NULL, sizeof(*data), PROT_READ | PROT_WRITE,
        MAP_SHARED, descriptor, 0);
    int mapping_error = errno;
    if (close(descriptor) == -1) {
        perror("close");
        if (data != MAP_FAILED) {
            close_shared(data, 0);
        }
        return 1;
    }
    if (data == MAP_FAILED) {
        errno = mapping_error;
        perror("mmap");
        return 1;
    }

    fprintf(stderr, "Child PID: %ld, parent PID: %ld\n",
        (long)getpid(), (long)data->parent_pid);

    int result = 1;
    char *line = NULL;
    size_t length = 0;
    size_t capacity = 0;
    size_t line_number = 0;
    FILE *output = fopen(argv[2], "w");
    data->reply = REPLY_OK;
    data->message[0] = '\0';
    if (!output) {
        set_error(data, "fopen", errno);
    }
    if (sem_post(&data->response) == -1) {
        perror("sem_post response");
        goto cleanup;
    }
    if (!output) {
        goto cleanup;
    }

    for (;;) {
        if (wait_message(&data->request, data->parent_pid, 1) == -1) {
            perror("wait request");
            break;
        }
        data->reply = REPLY_OK;
        data->message[0] = '\0';

        if (data->type == MESSAGE_FINISH) {
            if (fclose(output) == EOF) {
                set_error(data, "fclose", errno);
            }
            output = NULL;
        } else if (data->length > sizeof(data->buffer) ||
                (data->type != MESSAGE_PART && data->type != MESSAGE_LINE)) {
            set_error(data, "message", EINVAL);
        } else if (append_part(&line, &length, &capacity, data->buffer, data->length) != 0) {
            set_error(data, "realloc", errno);
        } else if (data->type == MESSAGE_LINE) {
            line_number++;
            if (length > 0 && line[length - 1] == '\r') {
                length--;
            }

            if (length > 0 && (line[length - 1] == '.' || line[length - 1] == ';')) {
                if (fwrite(line, 1, length, output) != length ||
                    fputc('\n', output) == EOF || fflush(output) == EOF) {
                    set_error(data, "write output", errno);
                }
            } else {
                data->reply = REPLY_INVALID;
                snprintf(data->message, sizeof(data->message),
                    "Error on line %zu: string must end with '.' or ';'", line_number);
            }
            length = 0;
        }

        int finished = data->type == MESSAGE_FINISH;
        int failed = data->reply == REPLY_ERROR;
        if (sem_post(&data->response) == -1) {
            perror("sem_post response");
            break;
        }
        if (failed || finished) {
            result = failed;
            break;
        }
    }

cleanup:
    if (output && fclose(output) == EOF) {
        perror("fclose");
        result = 1;
    }
    free(line);
    if (close_shared(data, 0) != 0) {
        result = 1;
    }
    return result;
}
