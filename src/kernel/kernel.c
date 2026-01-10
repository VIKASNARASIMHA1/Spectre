#include "kernel.h"

Microkernel* kernel_create(uint64_t mem_size) {
    Microkernel* kernel = (Microkernel*)malloc(sizeof(Microkernel));
    if (!kernel) return NULL;
    
    kernel->scheduler = (Scheduler*)malloc(sizeof(Scheduler));
    if (!kernel->scheduler) {
        free(kernel);
        return NULL;
    }
    scheduler_init(kernel->scheduler);
    
    kernel->mm = mm_create(mem_size);
    if (!kernel->mm) {
        free(kernel->scheduler);
        free(kernel);
        return NULL;
    }
    
    kernel->filesystem = vfs_create();
    if (!kernel->filesystem) {
        mm_destroy(kernel->mm);
        free(kernel->scheduler);
        free(kernel);
        return NULL;
    }
    
    kernel->queue_count = 0;
    kernel->running = false;
    
    // Initialize message queues
    for (int i = 0; i < MAX_QUEUES; i++) {
        kernel->queues[i] = NULL;
    }
    
    INFO("Microkernel created with %lu MB memory\n", mem_size / MiB);
    return kernel;
}

void kernel_destroy(Microkernel* kernel) {
    if (kernel) {
        // Destroy all processes
        if (kernel->scheduler) {
            for (int i = 0; i < kernel->scheduler->process_count; i++) {
                PCB* pcb = kernel->scheduler->processes[i];
                mm_free_pages(kernel->mm, pcb->pid);
                pcb_destroy(pcb);
            }
            free(kernel->scheduler);
        }
        
        // Destroy message queues
        for (int i = 0; i < kernel->queue_count; i++) {
            if (kernel->queues[i]) {
                mq_destroy(kernel->queues[i]);
            }
        }
        
        mm_destroy(kernel->mm);
        vfs_destroy(kernel->filesystem);
        free(kernel);
    }
}

uint32_t kernel_create_process(Microkernel* kernel, void* entry_point) {
    if (!kernel->scheduler) return 0;
    
    uint32_t pid = kernel->scheduler->next_pid++;
    PCB* pcb = pcb_create(pid, entry_point);
    if (!pcb) return 0;
    
    // Allocate initial pages for the process
    uint64_t phys_addr = mm_allocate_pages(kernel->mm, pid, 4);  // 4 pages
    if (phys_addr == 0) {
        pcb_destroy(pcb);
        return 0;
    }
    
    pcb->page_table = (uint64_t*)phys_addr;
    pcb->page_count = 4;
    
    scheduler_add_process(kernel->scheduler, pcb);
    INFO("Created process PID %u\n", pid);
    
    return pid;
}

void kernel_terminate_process(Microkernel* kernel, uint32_t pid) {
    if (!kernel->scheduler) return;
    
    for (int i = 0; i < kernel->scheduler->process_count; i++) {
        PCB* pcb = kernel->scheduler->processes[i];
        if (pcb->pid == pid) {
            pcb->state = PROC_TERMINATED;
            
            // Free process resources
            mm_free_pages(kernel->mm, pid);
            
            // Close all open files
            for (int j = 0; j < MAX_FILES; j++) {
                if (pcb->open_files[j] != -1) {
                    // File closing logic would go here
                    pcb->open_files[j] = -1;
                }
            }
            
            INFO("Terminated process PID %u\n", pid);
            break;
        }
    }
}

int kernel_create_queue(Microkernel* kernel) {
    if (kernel->queue_count >= MAX_QUEUES) {
        ERROR("Too many message queues\n");
        return -1;
    }
    
    MessageQueue* mq = mq_create(32);  // 32 message capacity
    if (!mq) return -1;
    
    int qid = kernel->queue_count++;
    kernel->queues[qid] = mq;
    
    INFO("Created message queue %d\n", qid);
    return qid;
}

int kernel_send_message(Microkernel* kernel, int qid, Message* msg) {
    if (qid < 0 || qid >= kernel->queue_count) {
        ERROR("Invalid queue ID\n");
        return -1;
    }
    
    if (!kernel->queues[qid]) {
        ERROR("Queue not initialized\n");
        return -1;
    }
    
    msg->timestamp = get_time_ms();
    return mq_send(kernel->queues[qid], msg, 0);
}

int kernel_receive_message(Microkernel* kernel, int qid, Message* msg, int timeout) {
    if (qid < 0 || qid >= kernel->queue_count) {
        ERROR("Invalid queue ID\n");
        return -1;
    }
    
    if (!kernel->queues[qid]) {
        ERROR("Queue not initialized\n");
        return -1;
    }
    
    return mq_receive(kernel->queues[qid], msg, timeout);
}

void kernel_destroy_queue(Microkernel* kernel, int qid) {
    if (qid >= 0 && qid < kernel->queue_count && kernel->queues[qid]) {
        mq_destroy(kernel->queues[qid]);
        kernel->queues[qid] = NULL;
        INFO("Destroyed message queue %d\n", qid);
    }
}

void kernel_run(Microkernel* kernel, uint64_t cycles) {
    if (!kernel->scheduler) return;
    
    kernel->running = true;
    INFO("Microkernel starting with %u processes\n", 
         kernel->scheduler->process_count);
    
    for (uint64_t i = 0; i < cycles && kernel->running; i++) {
        scheduler_tick(kernel->scheduler);
        
        // Simulate some work
        struct timespec ts = {0, 100000};  // 0.1ms
        nanosleep(&ts, NULL);
    }
    
    kernel->running = false;
    
    // Print statistics
    printf("\n=== Microkernel Statistics ===\n");
    printf("Total cycles: %lu\n", cycles);
    printf("Active processes: %d\n", kernel->scheduler->process_count);
    
    mm_print_stats(kernel->mm);
    vfs_list_files(kernel->filesystem);
}