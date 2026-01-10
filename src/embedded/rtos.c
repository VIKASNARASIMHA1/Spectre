#include "embedded.h"

RTOS* rtos_create() {
    RTOS* rtos = (RTOS*)malloc(sizeof(RTOS));
    if (!rtos) return NULL;
    
    memset(rtos, 0, sizeof(RTOS));
    rtos->system_time = get_time_ms();
    rtos->running = false;
    
    // Initialize virtual hardware
    gpio_init(&rtos->gpio);
    uart_init(&rtos->uart, 115200);
    
    for (int i = 0; i < MAX_TIMERS; i++) {
        timer_init(&rtos->timers[i], 1, true);
    }
    
    for (int i = 0; i < MAX_SENSORS; i++) {
        sensor_update(&rtos->sensors[i]);
    }
    
    return rtos;
}

void rtos_destroy(RTOS* rtos) {
    free(rtos);
}

uint32_t rtos_create_task(RTOS* rtos, void (*func)(void*), void* arg,
                         TaskPriority prio, uint32_t period, uint32_t wcet) {
    if (rtos->task_count >= MAX_TASKS) {
        ERROR("Too many tasks\n");
        return 0;
    }
    
    RTTask* task = &rtos->tasks[rtos->task_count];
    task->id = rtos->task_count + 1;
    task->state = TASK_READY;
    task->priority = prio;
    task->function = func;
    task->arg = arg;
    task->period = period;
    task->deadline = period;  // Assume deadline equals period
    task->wcet = wcet;
    task->last_run = 0;
    task->next_run = rtos->system_time;
    task->executions = 0;
    task->misses = 0;
    task->total_time = 0;
    
    rtos->task_count++;
    return task->id;
}

bool rtos_schedulable(RTOS* rtos) {
    // Rate Monotonic Analysis
    double utilization = 0.0;
    
    for (uint32_t i = 0; i < rtos->task_count; i++) {
        RTTask* task = &rtos->tasks[i];
        if (task->period > 0) {
            utilization += (double)task->wcet / task->period;
        }
    }
    
    // Liu & Layland bound
    double bound = rtos->task_count * (pow(2.0, 1.0/rtos->task_count) - 1.0);
    
    printf("Total utilization: %.2f%%\n", utilization * 100);
    printf("RMA bound for %d tasks: %.2f%%\n", 
           rtos->task_count, bound * 100);
    
    return utilization <= bound;
}

void rtos_schedule(RTOS* rtos) {
    uint64_t now = get_time_ms();
    rtos->system_time = now;
    
    // Check for timer events
    for (int i = 0; i < MAX_TIMERS; i++) {
        if (rtos->timers[i].enabled) {
            rtos->timers[i].counter++;
            if (rtos->timers[i].counter >= rtos->timers[i].compare) {
                if (rtos->timers[i].callback) {
                    rtos->timers[i].callback();
                }
                if (rtos->timers[i].auto_reload) {
                    rtos->timers[i].counter = 0;
                } else {
                    rtos->timers[i].enabled = false;
                }
            }
        }
    }
    
    // Find highest priority ready task
    RTTask* next_task = NULL;
    int highest_prio = -1;
    
    for (uint32_t i = 0; i < rtos->task_count; i++) {
        RTTask* task = &rtos->tasks[i];
        
        if (task->state == TASK_READY || task->state == TASK_RUNNING) {
            // Check if task needs to run
            if (now >= task->next_run) {
                if ((int)task->priority > highest_prio) {
                    highest_prio = task->priority;
                    next_task = task;
                }
            }
        }
    }
    
    // Execute task if found
    if (next_task) {
        rtos->current_task = next_task;
        next_task->state = TASK_RUNNING;
        next_task->last_run = now;
        
        uint64_t start = get_time_ms();
        
        // Execute task function
        if (next_task->function) {
            next_task->function(next_task->arg);
        }
        
        uint64_t end = get_time_ms();
        uint64_t exec_time = end - start;
        
        next_task->executions++;
        next_task->total_time += exec_time;
        
        // Check for deadline miss
        if (end > next_task->next_run + next_task->deadline) {
            next_task->misses++;
            ERROR("Task %d missed deadline!\n", next_task->id);
        }
        
        // Schedule next execution
        if (next_task->period > 0) {
            next_task->next_run = next_task->last_run + next_task->period;
        }
        
        next_task->state = TASK_READY;
        rtos->current_task = NULL;
    } else {
        // Idle
        rtos->idle_time++;
    }
}

void rtos_start(RTOS* rtos) {
    if (!rtos_schedulable(rtos)) {
        ERROR("System may not be schedulable!\n");
    }
    
    rtos->running = true;
    printf("RTOS started with %u tasks\n", rtos->task_count);
    
    while (rtos->running) {
        rtos_schedule(rtos);
        
        // Small delay to prevent CPU hogging
        struct timespec ts = {0, 1000000};  // 1ms
        nanosleep(&ts, NULL);
    }
}

void rtos_stop(RTOS* rtos) {
    rtos->running = false;
}

void rtos_print_stats(RTOS* rtos) {
    printf("\n=== RTOS Statistics ===\n");
    printf("System time: %lu ms\n", rtos->system_time);
    printf("Idle time: %lu cycles\n", rtos->idle_time);
    printf("Running: %s\n", rtos->running ? "Yes" : "No");
    
    printf("\nTasks:\n");
    for (uint32_t i = 0; i < rtos->task_count; i++) {
        RTTask* task = &rtos->tasks[i];
        const char* state_str = "UNKNOWN";
        switch (task->state) {
            case TASK_READY: state_str = "READY"; break;
            case TASK_RUNNING: state_str = "RUNNING"; break;
            case TASK_BLOCKED: state_str = "BLOCKED"; break;
            case TASK_SUSPENDED: state_str = "SUSPENDED"; break;
            case TASK_TERMINATED: state_str = "TERMINATED"; break;
        }
        
        printf("  Task %d: %s, Prio %d, Period %u ms, WCET %u ms\n",
               task->id, state_str, task->priority,
               task->period, task->wcet);
        printf("    Executions: %u, Misses: %u, Avg time: %.2f ms\n",
               task->executions, task->misses,
               task->executions > 0 ? 
               (float)task->total_time / task->executions : 0.0);
    }
}

// Virtual hardware implementations
void gpio_init(VirtualGPIO* gpio) {
    memset(gpio, 0, sizeof(VirtualGPIO));
}

void gpio_set_direction(VirtualGPIO* gpio, uint32_t pin, bool output) {
    if (pin < GPIO_PINS) {
        if (output) {
            gpio->direction |= (1 << pin);
        } else {
            gpio->direction &= ~(1 << pin);
        }
    }
}

void gpio_write(VirtualGPIO* gpio, uint32_t pin, bool value) {
    if (pin < GPIO_PINS && (gpio->direction & (1 << pin))) {
        if (value) {
            gpio->value |= (1 << pin);
        } else {
            gpio->value &= ~(1 << pin);
        }
        
        if (gpio->callback) {
            gpio->callback(pin, value);
        }
    }
}

bool gpio_read(VirtualGPIO* gpio, uint32_t pin) {
    if (pin < GPIO_PINS) {
        return (gpio->value >> pin) & 1;
    }
    return false;
}

void uart_init(VirtualUART* uart, uint32_t baud_rate) {
    memset(uart, 0, sizeof(VirtualUART));
    uart->baud_rate = baud_rate;
}

void uart_write(VirtualUART* uart, uint8_t* data, uint32_t len) {
    for (uint32_t i = 0; i < len; i++) {
        uart->tx_buffer[uart->tx_tail] = data[i];
        uart->tx_tail = (uart->tx_tail + 1) % 256;
    }
    uart->tx_busy = true;
}

uint32_t uart_read(VirtualUART* uart, uint8_t* buffer, uint32_t len) {
    uint32_t count = 0;
    while (count < len && uart->rx_head != uart->rx_tail) {
        buffer[count++] = uart->rx_buffer[uart->rx_head];
        uart->rx_head = (uart->rx_head + 1) % 256;
    }
    return count;
}