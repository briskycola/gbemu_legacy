#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define BIT(a, n) ((a & (1 << n)) ? 1 : 0)

#define BIT_SET(a, n, on)   \
    {                       \
        if (on)             \
            a |= (1 << n);  \
        else                \
            a &= ~(1 << n); \
    }

#define BETWEEN(a, b, c) ((a >= b) && (a <= c))

uint32_t getTicks();
void delay(uint32_t ms);

#define NO_IMPL                                   \
    {                                             \
        fprintf(stderr, "NOT YET IMPLEMENTED\n"); \
        exit(-5);                                 \
    }
