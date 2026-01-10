#include "embedded.h"

// Power states
typedef enum {
    POWER_RUN,      // Full power
    POWER_IDLE,     // CPU idle, peripherals active
    POWER_SLEEP,    // CPU stopped, some peripherals active
    POWER_DEEP_SLEEP, // CPU stopped, minimal peripherals
    POWER_OFF       // Complete power off
} PowerState;

// Power management unit
typedef struct {
    PowerState state;
    uint64_t transition_time;
    uint64_t wakeup_source;
    float voltage;
    float current;
    float temperature;
    
    // Power domains
    bool cpu_powered;
    bool memory_powered;
    bool peripherals_powered;
    
    // Wakeup sources
    uint32_t wakeup_pins;
    uint32_t wakeup_timers;
    bool wakeup_on_uart;
    
    // Statistics
    uint64_t time_in_state[5];
    uint64_t state_entries[5];
    uint64_t total_energy;  // in microjoules
} PowerManager;

// Create power manager
PowerManager* pm_create(void) {
    PowerManager* pm = (PowerManager*)malloc(sizeof(PowerManager));
    if (!pm) return NULL;
    
    pm->state = POWER_RUN;
    pm->transition_time = 0;
    pm->wakeup_source = 0;
    pm->voltage = 3.3;  // Typical 3.3V
    pm->current = 50.0; // 50mA in run mode
    pm->temperature = 25.0;
    
    pm->cpu_powered = true;
    pm->memory_powered = true;
    pm->peripherals_powered = true;
    
    pm->wakeup_pins = 0;
    pm->wakeup_timers = 0;
    pm->wakeup_on_uart = false;
    
    memset(pm->time_in_state, 0, sizeof(pm->time_in_state));
    memset(pm->state_entries, 0, sizeof(pm->state_entries));
    pm->total_energy = 0;
    
    return pm;
}

void pm_destroy(PowerManager* pm) {
    free(pm);
}

// Enter power state
void pm_enter_state(PowerManager* pm, PowerState new_state) {
    PowerState old_state = pm->state;
    uint64_t now = get_time_ms();
    
    // Update statistics for old state
    pm->time_in_state[old_state] += now - pm->transition_time;
    pm->state_entries[new_state]++;
    
    pm->state = new_state;
    pm->transition_time = now;
    
    // Configure power domains based on state
    switch (new_state) {
        case POWER_RUN:
            pm->cpu_powered = true;
            pm->memory_powered = true;
            pm->peripherals_powered = true;
            pm->current = 50.0;  // 50mA
            break;
            
        case POWER_IDLE:
            pm->cpu_powered = false;
            pm->memory_powered = true;
            pm->peripherals_powered = true;
            pm->current = 20.0;  // 20mA
            break;
            
        case POWER_SLEEP:
            pm->cpu_powered = false;
            pm->memory_powered = true;
            pm->peripherals_powered = false;
            pm->current = 5.0;   // 5mA
            break;
            
        case POWER_DEEP_SLEEP:
            pm->cpu_powered = false;
            pm->memory_powered = false;  // Only backup memory
            pm->peripherals_powered = false;
            pm->current = 0.1;   // 100μA
            break;
            
        case POWER_OFF:
            pm->cpu_powered = false;
            pm->memory_powered = false;
            pm->peripherals_powered = false;
            pm->current = 0.0;
            break;
    }
    
    INFO("Power state: %d -> %d, current: %.1fmA\n", 
         old_state, new_state, pm->current);
}

// Update power calculations
void pm_update(PowerManager* pm) {
    uint64_t now = get_time_ms();
    uint64_t elapsed = now - pm->transition_time;
    
    // Update time in current state
    pm->time_in_state[pm->state] += elapsed;
    
    // Calculate energy used (simplified: E = V * I * t)
    float power_mw = pm->voltage * pm->current;  // mW
    uint64_t energy_uj = (uint64_t)(power_mw * elapsed);  // μJ
    
    pm->total_energy += energy_uj;
    pm->transition_time = now;
    
    // Update temperature based on power
    float temp_increase = power_mw * 0.01;  // Simplified thermal model
    pm->temperature = 25.0 + temp_increase;
}

// Set wakeup source
void pm_set_wakeup_source(PowerManager* pm, uint32_t source_type, uint32_t source_id) {
    switch (source_type) {
        case 0:  // GPIO pin
            SET_BIT(pm->wakeup_pins, source_id);
            break;
        case 1:  // Timer
            SET_BIT(pm->wakeup_timers, source_id);
            break;
        case 2:  // UART
            pm->wakeup_on_uart = true;
            break;
    }
}

// Check if wakeup condition is met
bool pm_check_wakeup(PowerManager* pm, VirtualGPIO* gpio, VirtualTimer* timers) {
    if (pm->state == POWER_RUN) return false;
    
    // Check GPIO wakeup pins
    if (pm->wakeup_pins) {
        for (int i = 0; i < 32; i++) {
            if (TEST_BIT(pm->wakeup_pins, i) && gpio_read(gpio, i)) {
                pm->wakeup_source = BIT(i);
                return true;
            }
        }
    }
    
    // Check timer wakeup
    if (pm->wakeup_timers) {
        for (int i = 0; i < 8; i++) {
            if (TEST_BIT(pm->wakeup_timers, i) && 
                timers[i].enabled && timers[i].counter >= timers[i].compare) {
                pm->wakeup_source = BIT(32 + i);
                return true;
            }
        }
    }
    
    return false;
}

// Print power statistics
void pm_print_stats(PowerManager* pm) {
    printf("\n=== Power Management Statistics ===\n");
    
    const char* state_names[] = {
        "RUN", "IDLE", "SLEEP", "DEEP_SLEEP", "OFF"
    };
    
    printf("Current state: %s\n", state_names[pm->state]);
    printf("Voltage: %.2fV, Current: %.2fmA\n", pm->voltage, pm->current);
    printf("Temperature: %.1f°C\n", pm->temperature);
    printf("Total energy: %.3fJ\n", pm->total_energy / 1000000.0);
    
    printf("\nTime in each state:\n");
    for (int i = 0; i < 5; i++) {
        if (pm->time_in_state[i] > 0) {
            printf("  %-12s: %8lu ms (%5.1f%%), entries: %lu\n",
                   state_names[i],
                   pm->time_in_state[i],
                   100.0 * pm->time_in_state[i] / get_time_ms(),
                   pm->state_entries[i]);
        }
    }
    
    printf("\nWakeup sources:\n");
    printf("  GPIO pins: 0x%08x\n", pm->wakeup_pins);
    printf("  Timers: 0x%02x\n", pm->wakeup_timers);
    printf("  UART: %s\n", pm->wakeup_on_uart ? "enabled" : "disabled");
}