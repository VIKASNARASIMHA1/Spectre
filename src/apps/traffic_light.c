#include "embedded.h"

// Traffic light states
typedef enum {
    STATE_NS_GREEN,
    STATE_NS_YELLOW,
    STATE_EW_GREEN,
    STATE_EW_YELLOW
} TrafficState;

typedef struct {
    TrafficState state;
    uint64_t timer;
    VirtualGPIO* gpio;
} TrafficController;

// Pin definitions
#define NS_RED    0
#define NS_YELLOW 1
#define NS_GREEN  2
#define EW_RED    3
#define EW_YELLOW 4
#define EW_GREEN  5
#define PED_BUTTON 6
#define PED_LIGHT  7

void traffic_light_task(void* arg) {
    TrafficController* tc = (TrafficController*)arg;
    
    switch (tc->state) {
        case STATE_NS_GREEN:
            // North-South green, East-West red
            gpio_write(tc->gpio, NS_GREEN, 1);
            gpio_write(tc->gpio, NS_YELLOW, 0);
            gpio_write(tc->gpio, NS_RED, 0);
            gpio_write(tc->gpio, EW_GREEN, 0);
            gpio_write(tc->gpio, EW_YELLOW, 0);
            gpio_write(tc->gpio, EW_RED, 1);
            gpio_write(tc->gpio, PED_LIGHT, 0);
            
            if (tc->timer-- == 0) {
                tc->state = STATE_NS_YELLOW;
                tc->timer = 2000;  // 2 seconds
            }
            break;
            
        case STATE_NS_YELLOW:
            gpio_write(tc->gpio, NS_GREEN, 0);
            gpio_write(tc->gpio, NS_YELLOW, 1);
            
            if (tc->timer-- == 0) {
                tc->state = STATE_EW_GREEN;
                tc->timer = 5000;  // 5 seconds
            }
            break;
            
        case STATE_EW_GREEN:
            gpio_write(tc->gpio, EW_GREEN, 1);
            gpio_write(tc->gpio, EW_YELLOW, 0);
            gpio_write(tc->gpio, EW_RED, 0);
            gpio_write(tc->gpio, NS_GREEN, 0);
            gpio_write(tc->gpio, NS_YELLOW, 0);
            gpio_write(tc->gpio, NS_RED, 1);
            gpio_write(tc->gpio, PED_LIGHT, 1);
            
            if (tc->timer-- == 0) {
                tc->state = STATE_EW_YELLOW;
                tc->timer = 2000;  // 2 seconds
            }
            break;
            
        case STATE_EW_YELLOW:
            gpio_write(tc->gpio, EW_GREEN, 0);
            gpio_write(tc->gpio, EW_YELLOW, 1);
            
            if (tc->timer-- == 0) {
                tc->state = STATE_NS_GREEN;
                tc->timer = 5000;  // 5 seconds
            }
            break;
    }
}

void pedestrian_button_callback(uint32_t pin, uint32_t value) {
    if (pin == PED_BUTTON && value == 1) {
        printf("Pedestrian button pressed!\n");
        // In real system, this would trigger a shorter light cycle
    }
}

void demo_traffic_light() {
    printf("\n=== Traffic Light Controller Demo ===\n");
    
    RTOS* rtos = rtos_create();
    
    // Setup GPIO
    for (int i = 0; i <= 7; i++) {
        gpio_set_direction(&rtos->gpio, i, true);
    }
    gpio_set_direction(&rtos->gpio, PED_BUTTON, false);
    
    // Setup button callback
    rtos->gpio.callback = pedestrian_button_callback;
    rtos->gpio.interrupt_mask = (1 << PED_BUTTON);
    
    // Create traffic controller
    TrafficController tc = {
        .state = STATE_NS_GREEN,
        .timer = 5000,
        .gpio = &rtos->gpio
    };
    
    // Create real-time task
    rtos_create_task(rtos, traffic_light_task, &tc,
                    PRIO_HIGH, 100, 10);  // Run every 100ms, WCET 10ms
    
    printf("Starting traffic light controller...\n");
    printf("Press Ctrl+C to stop\n");
    
    // Run for 30 seconds
    uint64_t start = get_time_ms();
    while (get_time_ms() - start < 30000) {
        rtos_schedule(rtos);
        struct timespec ts = {0, 1000000};  // 1ms
        nanosleep(&ts, NULL);
    }
    
    rtos_print_stats(rtos);
    rtos_destroy(rtos);
}