#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <limits.h>
#include <float.h>
#include <math.h>
#include <unistd.h>
#include <errno.h>

#include "monte_carlo.h"


static void print_usage(const char *program) {
    fprintf(stderr, "Usage: %s -t threads -n experiments -r radius [-s seed]\n", program);
}

static int read_number(const char *string, uint64_t *number) {
    char *end;
    errno = 0;
    unsigned long long value = strtoull(string, &end, 10);

    if (errno != 0 || string[0] < '0' || string[0] > '9' || *end != '\0' ||
        value > UINT64_MAX) {
        return 1;
    }

    *number = (uint64_t)value;
    return 0;
}

int main(int argc, char *argv[]) {
    uint64_t thread_limit = 0;
    uint64_t experiments = 0;
    uint64_t seed = 1;
    double radius = 0.0;
    int option;

    while ((option = getopt(argc, argv, "t:n:r:s:h")) != -1) {
        switch (option) {
            case 't':
                if (read_number(optarg, &thread_limit) != 0 || thread_limit == 0 ||
                    thread_limit > SIZE_MAX) {
                    fprintf(stderr, "Invalid thread limit\n");
                    return 1;
                }
                break;
            case 'n':
                if (read_number(optarg, &experiments) != 0 || experiments == 0) {
                    fprintf(stderr, "Invalid number of experiments\n");
                    return 1;
                }
                break;
            case 'r': {
                char *end;
                errno = 0;
                radius = strtod(optarg, &end);
                if (errno != 0 || end == optarg || *end != '\0' ||
                    !isfinite(radius) || radius <= 0.0 || radius > sqrt(DBL_MAX / 4.0)) {
                    fprintf(stderr, "Invalid radius\n");
                    return 1;
                }
                break;
            }
            case 's':
                if (read_number(optarg, &seed) != 0 || seed > UINT_MAX) {
                    fprintf(stderr, "Invalid seed\n");
                    return 1;
                }
                break;
            case 'h':
                print_usage(argv[0]);
                return 0;
            default:
                print_usage(argv[0]);
                return 1;
        }
    }

    if (optind != argc || thread_limit == 0 || experiments == 0 || radius == 0.0) {
        print_usage(argv[0]);
        return 1;
    }

    size_t threads = (size_t)(thread_limit < experiments ? thread_limit : experiments);
    printf("PID: %ld, threads: %zu\n", (long)getpid(), threads);
    printf("Radius: %.12g, experiments: %" PRIu64 ", seed: %" PRIu64 "\n",
           radius, experiments, seed);

    double area;
    double elapsed;
    if (calculate_area(radius, experiments, threads, (unsigned int)seed, &area, &elapsed) != 0) {
        return 1;
    }

    double reference = acos(-1.0) * radius * radius;
    printf("Area: %.12g\n", area);
    printf("Reference area: %.12g\n", reference);
    if (reference > 0.0) {
        printf("Relative error: %.6f%%\n", fabs(area - reference) / reference * 100.0);
    }
    printf("Time: %.6f s\n", elapsed);

    return 0;
}
