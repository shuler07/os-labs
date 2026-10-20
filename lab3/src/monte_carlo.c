#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>

#include "monte_carlo.h"


struct thread_data {
    uint64_t experiments;
    uint64_t inside;
    unsigned int seed;
    int error;
};

static void *run_experiments(void *argument) {
    struct thread_data *data = argument;
    unsigned int seed = data->seed;
    uint64_t inside = 0;

    if (printf("PID: %ld, TID: %ld, experiments: %" PRIu64 "\n",
               (long)getpid(), (long)gettid(), data->experiments) < 0) {
        data->error = 1;
        return NULL;
    }

    // считаем точки в четверти единичного круга
    for (uint64_t i = 0; i < data->experiments; i++) {
        double x = rand_r(&seed) / ((double)RAND_MAX + 1.0);
        double y = rand_r(&seed) / ((double)RAND_MAX + 1.0);
        if (x * x + y * y <= 1.0) {
            inside++;
        }
    }

    data->inside = inside;
    return NULL;
}

static void join_threads(pthread_t *workers, size_t count) {
    for (size_t i = 0; i < count; i++) {
        int error = pthread_join(workers[i], NULL);
        if (error != 0) {
            fprintf(stderr, "pthread_join: %s\n", strerror(error));
            exit(1);
        }
    }
}

int calculate_area(double radius, uint64_t experiments, size_t threads, 
        unsigned int seed, double *area, double *elapsed) {
    if (threads == 0 || threads > experiments ||
        threads > SIZE_MAX / sizeof(struct thread_data) ||
        threads > SIZE_MAX / sizeof(pthread_t)) {
        fprintf(stderr, "Invalid number of threads\n");
        return 1;
    }

    struct thread_data *data = calloc(threads, sizeof(*data));
    if (!data) {
        perror("calloc");
        return 1;
    }

    pthread_t *workers = NULL;
    if (threads > 1) {
        workers = malloc((threads - 1) * sizeof(*workers));
        if (!workers) {
            perror("malloc");
            free(data);
            return 1;
        }
    }

    for (size_t i = 0; i < threads; i++) {
        data[i].experiments = experiments / threads + (i < experiments % threads);
        data[i].seed = seed + (unsigned int)i * 2654435761u;
    }

    struct timespec start, finish;
    if (clock_gettime(CLOCK_MONOTONIC, &start) == -1) {
        perror("clock_gettime");
        free(workers);
        free(data);
        return 1;
    }

    // основной поток тоже работает и входит в лимит
    for (size_t i = 1; i < threads; i++) {
        int error = pthread_create(&workers[i - 1], NULL, run_experiments, &data[i]);
        if (error != 0) {
            fprintf(stderr, "pthread_create: %s\n", strerror(error));
            join_threads(workers, i - 1);
            free(workers);
            free(data);
            return 1;
        }
    }

    run_experiments(&data[0]);
    join_threads(workers, threads - 1);

    int result = 0;
    if (clock_gettime(CLOCK_MONOTONIC, &finish) == -1) {
        perror("clock_gettime");
        result = 1;
    }

    uint64_t inside = 0;
    for (size_t i = 0; i < threads; i++) {
        inside += data[i].inside;
        if (data[i].error) {
            fprintf(stderr, "Error writing thread information\n");
            result = 1;
        }
    }

    if (result == 0) {
        *area = (4.0 * radius * radius) * ((double)inside / (double)experiments);
        *elapsed = (double)(finish.tv_sec - start.tv_sec) + 
            (double)(finish.tv_nsec - start.tv_nsec) / 1000000000.0;
    }

    free(workers);
    free(data);
    return result;
}
