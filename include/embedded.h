#ifndef EMBEDDED_H
#define EMBEDDED_H

#include "common.h"

#define MAX_TASKS 16
#define MAX_TIMERS 8
#define MAX_SENSORS 4
#define GPIO_PINS 32

// Task states
typedef enum {
    TASK_READY,
    TASK_RUNNING,
    TASK_BLOCKED,
    TASK_SUSPENDED,
    TASK_TERMINATED
} TaskState;

// Task priorities
typedef enum {
    PRIO_IDLE = 0,
    PRIO_LOW = 1,
    PRIO_NORMAL = 2,
    PRIO_HIGH = 3,
    PRIO_CRITICAL = 4,
    PRIO_MAX = 15
} TaskPriority;

// Real-time task
typedef struct {
    uint32_t id;
    TaskState state;
    TaskPriority priority;
    void (*function)(void*);
    void* arg;
    
    uint32_t period;       // in ms
    uint32_t deadline;     // in ms
    uint32_t wcet;         // worst-case execution time in ms
    uint32_t last_run;
    uint32_t next_run;
    
    uint32_t executions;
    uint32_t misses;
    uint64_t total_time;
} RTTask;

// Virtual GPIO
typedef struct {
    uint32_t direction;    // 1 = output, 0 = input
    uint32_t value;        // current pin values
    uint32_t pull;         // pull-up/down configuration
    uint32_t interrupt_mask; // which pins generate interrupts
    void (*callback)(uint32_t pin, uint32_t value);
} VirtualGPIO;

// Virtual UART
typedef struct {
    uint8_t rx_buffer[256];
    uint8_t tx_buffer[256];
    uint32_t rx_head;
    uint32_t rx_tail;
    uint32_t tx_head;
    uint32_t tx_tail;
    uint32_t baud_rate;
    bool tx_busy;
    bool rx_ready;
} VirtualUART;

// Virtual Timer
typedef struct {
    uint64_t counter;
    uint64_t compare;
    uint64_t prescaler;
    bool enabled;
    bool auto_reload;
    void (*callback)(void);
} VirtualTimer;

// Virtual Sensor
typedef struct {
    float temperature;
    float humidity;
    float pressure;
    float acceleration[3];
    uint32_t light_level;
    uint64_t last_update;
} VirtualSensor;

// Real-Time OS
typedef struct {
    RTTask tasks[MAX_TASKS];
    uint32_t task_count;
    RTTask* current_task;
    
    VirtualGPIO gpio;
    VirtualUART uart;
    VirtualTimer timers[MAX_TIMERS];
    VirtualSensor sensors[MAX_SENSORS];
    
    uint64_t system_time;
    uint64_t idle_time;
    bool running;
    
    // Power management
    uint32_t sleep_mode;
    uint64_t wakeup_time;
} RTOS;

// Function prototypes
RTOS* rtos_create();
void rtos_destroy(RTOS* rtos);
void rtos_init(RTOS* rtos);

// Task management
uint32_t rtos_create_task(RTOS* rtos, void (*func)(void*), void* arg,
                         TaskPriority prio, uint32_t period, uint32_t wcet);
void rtos_start(RTOS* rtos);
void rtos_stop(RTOS* rtos);
void rtos_schedule(RTOS* rtos);
bool rtos_schedulable(RTOS* rtos);  // Rate monotonic analysis

// Virtual hardware
void gpio_init(VirtualGPIO* gpio);
void gpio_set_direction(VirtualGPIO* gpio, uint32_t pin, bool output);
void gpio_write(VirtualGPIO* gpio, uint32_t pin, bool value);
bool gpio_read(VirtualGPIO* gpio, uint32_t pin);

void uart_init(VirtualUART* uart, uint32_t baud_rate);
void uart_write(VirtualUART* uart, uint8_t* data, uint32_t len);
uint32_t uart_read(VirtualUART* uart, uint8_t* buffer, uint32_t len);

void timer_init(VirtualTimer* timer, uint64_t prescaler, bool auto_reload);
void timer_start(VirtualTimer* timer, uint64_t compare_value);
void timer_stop(VirtualTimer* timer);

void sensor_update(VirtualSensor* sensor);

// Power management
void rtos_enter_sleep(RTOS* rtos, uint32_t mode);
void rtos_wakeup(RTOS* rtos);

// Analysis and monitoring
void rtos_print_stats(RTOS* rtos);
void rtos_print_schedule(RTOS* rtos, uint32_t duration);

#endif