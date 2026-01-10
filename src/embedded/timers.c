#include "embedded.h"

void timer_init(VirtualTimer* timer, uint64_t prescaler, bool auto_reload) {
    if (!timer) return;
    
    timer->counter = 0;
    timer->compare = 0;
    timer->prescaler = prescaler > 0 ? prescaler : 1;
    timer->enabled = false;
    timer->auto_reload = auto_reload;
    timer->callback = NULL;
}

void timer_start(VirtualTimer* timer, uint64_t compare_value) {
    if (!timer) return;
    
    timer->counter = 0;
    timer->compare = compare_value;
    timer->enabled = true;
}

void timer_stop(VirtualTimer* timer) {
    if (timer) {
        timer->enabled = false;
    }
}

void timer_set_callback(VirtualTimer* timer, void (*callback)(void)) {
    if (timer) {
        timer->callback = callback;
    }
}

uint64_t timer_get_value(VirtualTimer* timer) {
    return timer ? timer->counter : 0;
}

bool timer_is_running(VirtualTimer* timer) {
    return timer ? timer->enabled : false;
}

void timer_tick(VirtualTimer* timer) {
    if (!timer || !timer->enabled) return;
    
    timer->counter++;
    
    if (timer->counter >= timer->compare) {
        if (timer->callback) {
            timer->callback();
        }
        
        if (timer->auto_reload) {
            timer->counter = 0;
        } else {
            timer->enabled = false;
        }
    }
}