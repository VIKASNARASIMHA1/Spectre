#ifndef COMMON_H
#define COMMON_H

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

// Debug macros
#define DEBUG 1
#if DEBUG
#define DBG_PRINT(fmt, ...) printf("[DEBUG] " fmt, ##__VA_ARGS__)
#else
#define DBG_PRINT(fmt, ...)
#endif

#define ERROR(fmt, ...) fprintf(stderr, "[ERROR] " fmt, ##__VA_ARGS__)
#define INFO(fmt, ...) printf("[INFO] " fmt, ##__VA_ARGS__)
#define WARN(fmt, ...) printf("[WARN] " fmt, ##__VA_ARGS__)

// Memory constants
#define KiB 1024
#define MiB (1024 * KiB)
#define GiB (1024 * MiB)
#define PAGE_SIZE 4096
#define CACHE_LINE_SIZE 64

// Utility functions
static inline uint64_t get_time_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

static inline uint64_t get_time_us(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000 + ts.tv_nsec / 1000;
}

// Bit manipulation
#define BIT(n) (1ULL << (n))
#define SET_BIT(var, bit) ((var) |= BIT(bit))
#define CLEAR_BIT(var, bit) ((var) &= ~BIT(bit))
#define TOGGLE_BIT(var, bit) ((var) ^= BIT(bit))
#define TEST_BIT(var, bit) (((var) >> (bit)) & 1)

// Memory alignment
#define ALIGN_UP(addr, align) (((addr) + (align) - 1) & ~((align) - 1))
#define ALIGN_DOWN(addr, align) ((addr) & ~((align) - 1))

// Min/Max
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

// Array size
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

// Assert macro
#ifdef NDEBUG
#define ASSERT(expr) ((void)0)
#else
#define ASSERT(expr) \
    do { \
        if (!(expr)) { \
            fprintf(stderr, "[ASSERT] %s:%d: %s\n", \
                    __FILE__, __LINE__, #expr); \
            abort(); \
        } \
    } while (0)
#endif

#endif