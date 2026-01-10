#ifndef CONFIG_H
#define CONFIG_H

// System configuration
#define SYSTEM_NAME "Spectre Simulator"
#define VERSION_MAJOR 1
#define VERSION_MINOR 0
#define VERSION_PATCH 0

// CPU configuration
#define DEFAULT_MEMORY_SIZE (64 * 1024 * 1024)  // 64MB
#define DEFAULT_CACHE_L1_SIZE (32 * 1024)       // 32KB
#define DEFAULT_CACHE_L2_SIZE (256 * 1024)      // 256KB
#define DEFAULT_BRANCH_PREDICTOR_SIZE 4096

// Kernel configuration
#define MAX_PROCESSES 64
#define MAX_THREADS_PER_PROCESS 16
#define MAX_OPEN_FILES_PER_PROCESS 32
#define MAX_MESSAGE_QUEUES 32

// Embedded configuration
#define MAX_RT_TASKS 16
#define MAX_VIRTUAL_SENSORS 8
#define MAX_VIRTUAL_TIMERS 8
#define GPIO_PIN_COUNT 32

// Performance tuning
#define SCHEDULER_TICK_MS 10
#define CACHE_HIT_LATENCY 1
#define CACHE_MISS_LATENCY 10
#define BRANCH_MISPREDICT_PENALTY 3

// Debug options
#define DEBUG_PIPELINE 0
#define DEBUG_CACHE 0
#define DEBUG_SCHEDULER 0
#define DEBUG_RTOS 0
#define DEBUG_POWER 0

// Feature flags
#define FEATURE_OUT_OF_ORDER 0
#define FEATURE_MULTICORE 0
#define FEATURE_VIRTUALIZATION 0
#define FEATURE_SECURITY 0

// Optimization flags
#define OPTIMIZE_FOR_SIZE 0
#define OPTIMIZE_FOR_SPEED 1
#define USE_SIMD 0
#define USE_JIT 0

#endif