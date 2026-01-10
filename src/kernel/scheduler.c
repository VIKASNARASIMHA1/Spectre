#include "kernel.h"

// Create a new process
PCB* pcb_create(uint32_t pid, void* entry_point) {
    PCB* pcb = (PCB*)malloc(sizeof(PCB));
    if (!pcb) return NULL;
    
    pcb->pid = pid;
    pcb->state = PROC_NEW;
    pcb->priority = 7;  // Default priority
    pcb->quantum = 100; // Default quantum
    pcb->quantum_remaining = pcb->quantum;
    
    memset(pcb->registers, 0, sizeof(pcb->registers));
    pcb->pc = (uint64_t)entry_point;
    pcb->sp = 0x8000;
    pcb->flags = 0;
    
    pcb->page_table = NULL;
    pcb->page_count = 0;
    pcb->heap_start = 0;
    pcb->heap_end = 0;
    
    memset(pcb->open_files, -1, sizeof(pcb->open_files));
    memset(pcb->message_queues, -1, sizeof(pcb->message_queues));
    
    pcb->start_time = 0;
    pcb->cpu_time = 0;
    pcb->wakeups = 0;
    
    return pcb;
}

void pcb_destroy(PCB* pcb) {
    if (pcb) {
        if (pcb->page_table) {
            free(pcb->page_table);
        }
        free(pcb);
    }
}

void scheduler_init(Scheduler* sched) {
    memset(sched, 0, sizeof(Scheduler));
    sched->current_process = NULL;
    sched->process_count = 0;
    sched->system_time = 0;
    sched->next_pid = 1;
    
    for (int i = 0; i < 16; i++) {
        sched->queue_counts[i] = 0;
    }
}

void scheduler_add_process(Scheduler* sched, PCB* pcb) {
    if (sched->process_count >= MAX_PROCESSES) {
        ERROR("Too many processes\n");
        return;
    }
    
    sched->processes[sched->process_count++] = pcb;
    
    // Add to appropriate ready queue
    uint8_t priority = pcb->priority & 0x0F;  // 0-15
    int idx = sched->queue_counts[priority];
    if (idx < MAX_PROCESSES) {
        sched->ready_queues[priority][idx] = pcb;
        sched->queue_counts[priority]++;
        pcb->state = PROC_READY;
    }
}

PCB* scheduler_next_process(Scheduler* sched) {
    // MLFQ scheduling: check queues from highest to lowest priority
    for (int priority = 0; priority < 16; priority++) {
        if (sched->queue_counts[priority] > 0) {
            // Get first process from this queue
            PCB* next = sched->ready_queues[priority][0];
            
            // Shift queue
            for (int i = 1; i < sched->queue_counts[priority]; i++) {
                sched->ready_queues[priority][i-1] = sched->ready_queues[priority][i];
            }
            sched->queue_counts[priority]--;
            
            next->state = PROC_RUNNING;
            next->quantum_remaining = next->quantum;
            
            return next;
        }
    }
    
    return NULL;  // No process ready
}

void scheduler_tick(Scheduler* sched) {
    sched->system_time++;
    
    if (sched->current_process) {
        PCB* pcb = sched->current_process;
        pcb->cpu_time++;
        pcb->quantum_remaining--;
        
        if (pcb->quantum_remaining == 0) {
            // Time slice expired
            pcb->state = PROC_READY;
            
            // Lower priority if process used full quantum
            if (pcb->priority < 15) {
                pcb->priority++;
            }
            
            // Add back to ready queue
            uint8_t priority = pcb->priority & 0x0F;
            int idx = sched->queue_counts[priority];
            if (idx < MAX_PROCESSES) {
                sched->ready_queues[priority][idx] = pcb;
                sched->queue_counts[priority]++;
            }
            
            sched->current_process = NULL;
        }
    }
    
    // If no process running, schedule next one
    if (!sched->current_process) {
        sched->current_process = scheduler_next_process(sched);
    }
}

void scheduler_print(Scheduler* sched) {
    printf("\n=== Scheduler Status ===\n");
    printf("System time: %lu\n", sched->system_time);
    printf("Total processes: %d\n", sched->process_count);
    printf("Current process: %s\n", 
           sched->current_process ? "Yes" : "No");
    
    if (sched->current_process) {
        printf("  PID: %u, State: RUNNING, Priority: %d\n",
               sched->current_process->pid,
               sched->current_process->priority);
    }
    
    printf("\nReady queues:\n");
    for (int i = 0; i < 16; i++) {
        if (sched->queue_counts[i] > 0) {
            printf("  Priority %d: %d processes\n", i, sched->queue_counts[i]);
        }
    }
    
    printf("\nAll processes:\n");
    for (int i = 0; i < sched->process_count; i++) {
        PCB* p = sched->processes[i];
        const char* state_str = "UNKNOWN";
        switch (p->state) {
            case PROC_NEW: state_str = "NEW"; break;
            case PROC_READY: state_str = "READY"; break;
            case PROC_RUNNING: state_str = "RUNNING"; break;
            case PROC_BLOCKED: state_str = "BLOCKED"; break;
            case PROC_TERMINATED: state_str = "TERMINATED"; break;
        }
        printf("  PID %u: %s, Priority %d, CPU time %lu\n",
               p->pid, state_str, p->priority, p->cpu_time);
    }
}