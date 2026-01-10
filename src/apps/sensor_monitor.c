#include "embedded.h"

typedef struct {
    RTOS* rtos;
    uint32_t update_interval;
    bool display_raw;
} SensorMonitor;

void sensor_task(void* arg) {
    SensorMonitor* monitor = (SensorMonitor*)arg;
    
    // Update all sensors
    for (int i = 0; i < MAX_SENSORS; i++) {
        sensor_update(&monitor->rtos->sensors[i]);
    }
    
    // Display readings
    if (monitor->display_raw) {
        printf("\n--- Sensor Update ---\n");
        for (int i = 0; i < MAX_SENSORS; i++) {
            printf("Sensor %d: Temp=%.1fC, Hum=%.1f%%, Light=%u\n",
                   i,
                   monitor->rtos->sensors[i].temperature,
                   monitor->rtos->sensors[i].humidity,
                   monitor->rtos->sensors[i].light_level);
        }
    }
}

void threshold_check_task(void* arg) {
    SensorMonitor* monitor = (SensorMonitor*)arg;
    
    // Check temperature thresholds
    for (int i = 0; i < MAX_SENSORS; i++) {
        VirtualSensor* s = &monitor->rtos->sensors[i];
        
        if (s->temperature > 30.0) {
            printf("[ALERT] Sensor %d temperature high: %.1f°C\n", 
                   i, s->temperature);
        }
        
        if (s->temperature < 10.0) {
            printf("[ALERT] Sensor %d temperature low: %.1f°C\n", 
                   i, s->temperature);
        }
        
        if (s->humidity > 80.0) {
            printf("[WARNING] Sensor %d humidity high: %.1f%%\n", 
                   i, s->humidity);
        }
    }
}

void demo_sensor_monitor() {
    printf("\n=== Sensor Monitoring System ===\n");
    
    RTOS* rtos = rtos_create();
    SensorMonitor monitor = {
        .rtos = rtos,
        .update_interval = 1000,  // 1 second
        .display_raw = true
    };
    
    // Create sensor monitoring tasks
    rtos_create_task(rtos, sensor_task, &monitor,
                    PRIO_NORMAL, monitor.update_interval, 5);
    
    rtos_create_task(rtos, threshold_check_task, &monitor,
                    PRIO_LOW, monitor.update_interval * 5, 10);
    
    // Create a timer for periodic system check
    timer_set_callback(&rtos->timers[0], []() {
        printf("[SYSTEM] Periodic check at %lu ms\n", get_time_ms());
    });
    timer_start(&rtos->timers[0], 10000);  // Every 10 seconds
    
    printf("Starting sensor monitor for 60 seconds...\n");
    
    uint64_t start = get_time_ms();
    while (get_time_ms() - start < 60000) {
        rtos_schedule(rtos);
        
        // Update timers
        for (int i = 0; i < MAX_TIMERS; i++) {
            timer_tick(&rtos->timers[i]);
        }
        
        struct timespec ts = {0, 1000000};  // 1ms
        nanosleep(&ts, NULL);
    }
    
    rtos_print_stats(rtos);
    rtos_destroy(rtos);
}