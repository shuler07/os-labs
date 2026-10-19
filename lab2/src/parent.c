// #define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>


int main() {
    char *filename = NULL;
    size_t filename_capacity = 0;

    printf("Input output filename: ");
    ssize_t filename_length = getline(&filename, &filename_capacity, stdin);

    // ошибка при чтении filename
    if (filename_length == -1) {
        fprintf(stderr, "Error reading filename\n");
        free(filename);
        return 1;
    }

    // меняем "filename\n\0" на "filename\0"
    if (filename_length > 0 && filename[filename_length - 1] == '\n') {
        filename[--filename_length] = '\0';
    }

    // пустое имя файла
    if (filename_length == 0) {
        fprintf(stderr, "Filename cannot be empty\n");
        free(filename);
        return 1;
    }

    int pipe1[2], pipe2[2];

    // ошибка при создании pipe
    if (pipe(pipe1) == -1 || pipe(pipe2) == -1) {
        perror("pipe");
        free(filename);
        return 1;
    }

    // ошибка при форке процесса
    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        close(pipe1[0]);
        close(pipe1[1]);
        close(pipe2[0]);
        close(pipe2[1]);
        free(filename);
        return 1;
    }

    if (pid == 0) {
        // дочерний процесс: чтение из pipe1, запись в pipe2
        close(pipe1[1]);
        close(pipe2[0]);

        // ошибка при дюпе stdin
        if (dup2(pipe1[0], STDIN_FILENO) == -1) {
            perror("dup2 stdin");
            _exit(1);
        }

        // ошибка при дюпе stderr
        if (dup2(pipe2[1], STDERR_FILENO) == -1) {
            perror("dup2 stdout");
            _exit(1);
        }

        close(pipe1[0]);
        close(pipe2[1]);

        execl("./child", "child", filename, (char *)NULL);

        perror("execl");
        _exit(1);
    }

    // родительский процесс
    close(pipe1[0]);
    close(pipe2[1]);
    free(filename);

    char *line = NULL;
    size_t line_capacity = 0;

    printf("Enter strings:\n");
    while (getline(&line, &line_capacity, stdin) != -1) {
        if (write(pipe1[1], line, strlen(line)) == -1) {
            perror("write");
            break;
        };
    }
    free(line);
    close(pipe1[1]);

    char message[512];
    ssize_t bytes_read;
    while ((bytes_read = read(pipe2[0], message, sizeof(message))) > 0) {
        fwrite(message, 1, bytes_read, stderr);
    }
    if (bytes_read == -1) {
        perror("read");
    }
    close(pipe2[0]);

    int status;
    if (waitpid(pid, &status, 0) == -1) {
        perror("waitpid");
        return 1;
    }
    if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
        fprintf(stderr, "Child process exited with code %d\n",
                WEXITSTATUS(status));
        return 1;
    }

    return 0;
}
