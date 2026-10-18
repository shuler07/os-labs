// #define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


int main(int argc, char *argv[]) {
    const char *filename = argv[1];

    FILE *output = fopen(filename, "w");
    if (!output) {
        perror("fopen");
        return 1;
    }

    char *line = NULL;
    size_t capacity = 0;
    ssize_t length;
    int line_number = 0;

    while ((length = getline(&line, &capacity, stdin)) != -1) {
        line_number++;

        // меняем "string\n\0" на "string\0"
        if (length > 0 && line[length - 1] == '\n') {
            line[--length] = '\0';
        }

        // тоже самое для винды
        if (length > 0 && line[length - 1] == '\r') {
            line[--length] = '\0';
        }

        if (length > 0 && (line[length - 1] == '.' || line[length - 1] == ';')) {
            if (fprintf(output, "%s\n", line) == -1) {
                perror("Writing to output file");
                free(line);
                fclose(output);
                return 1;
            }
        } else {
            fprintf(stderr, "Error on line %d: string must end with '.' or ';'\n", line_number);
        }
    }

    free(line);
    fclose(output);

    return 0;
}
