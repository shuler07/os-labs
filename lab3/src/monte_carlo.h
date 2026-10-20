#ifndef MONTE_CARLO_H
#define MONTE_CARLO_H

#include <stddef.h>
#include <stdint.h>


int calculate_area(double radius, uint64_t experiments,
    size_t threads, unsigned int seed, double *area, double *elapsed);

#endif
