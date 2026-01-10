#include "embedded.h"
#include <math.h>

// Simulate realistic sensor values with some noise
static float random_float(float min, float max) {
    return min + ((float)rand() / RAND_MAX) * (max - min);
}

void sensor_update(VirtualSensor* sensor) {
    if (!sensor) return;
    
    // Update temperature (simulate daily cycle)
    float time_of_day = fmod(get_time_ms() / 1000.0, 86400.0) / 86400.0;
    float base_temp = 20.0 + 10.0 * sin(2 * M_PI * time_of_day);
    sensor->temperature = base_temp + random_float(-0.5, 0.5);
    
    // Update humidity (inverse relationship with temperature)
    sensor->humidity = 50.0 + 30.0 * sin(2 * M_PI * time_of_day + M_PI) + 
                      random_float(-2.0, 2.0);
    if (sensor->humidity < 0) sensor->humidity = 0;
    if (sensor->humidity > 100) sensor->humidity = 100;
    
    // Update pressure (slow changes)
    sensor->pressure = 1013.25 + 10.0 * sin(2 * M_PI * time_of_day / 24.0) +
                      random_float(-0.5, 0.5);
    
    // Update acceleration (simulate occasional movement)
    static uint64_t last_movement = 0;
    uint64_t now = get_time_ms();
    
    if (now - last_movement > 5000) {  // Every 5 seconds
        // Simulate a movement
        sensor->acceleration[0] = random_float(-1.0, 1.0);
        sensor->acceleration[1] = random_float(-1.0, 1.0);
        sensor->acceleration[2] = 9.8 + random_float(-0.1, 0.1);  // Gravity
        last_movement = now;
    } else {
        // Normal stationary state
        sensor->acceleration[0] = random_float(-0.01, 0.01);
        sensor->acceleration[1] = random_float(-0.01, 0.01);
        sensor->acceleration[2] = 9.8 + random_float(-0.01, 0.01);
    }
    
    // Update light level (day/night cycle)
    float light_factor = 0.5 + 0.5 * sin(2 * M_PI * time_of_day);
    sensor->light_level = (uint32_t)(1000 * light_factor + random_float(-50, 50));
    
    sensor->last_update = now;
}

void sensor_print(VirtualSensor* sensor) {
    if (!sensor) return;
    
    printf("\n=== Sensor Readings ===\n");
    printf("Temperature: %.2f °C\n", sensor->temperature);
    printf("Humidity: %.2f %%\n", sensor->humidity);
    printf("Pressure: %.2f hPa\n", sensor->pressure);
    printf("Acceleration: [%.3f, %.3f, %.3f] m/s²\n",
           sensor->acceleration[0],
           sensor->acceleration[1],
           sensor->acceleration[2]);
    printf("Light level: %u lux\n", sensor->light_level);
    printf("Last update: %lu ms ago\n", 
           get_time_ms() - sensor->last_update);
}