#ifndef KERNEL_H
#define KERNEL_H

#include "common.h"

#define MAX_PROCESSES 64
#define MAX_PAGES 1024
#define MAX_QUEUES 32
#define MAX_FILES 128
#define MAX_NAME_LEN 32

// Process states
typedef enum {
    PROC_NEW,
    PROC_READY,
    PROC_RUNNING,
    PROC_BLOCKED,
    PROC_TERMINATED
} ProcessState;

// Message structure
typedef struct {
    uint32_t src_pid;
    uint32_t dst_pid;
    uint32_t msg_id;
    uint64_t timestamp;
    void* data;
    size_t size;
} Message;

// Message queue
typedef struct {
    Message* messages;
    int capacity;
    int head;
    int tail;
    int count;
    sem_t mutex;
    sem_t slots;
    sem_t items;
} MessageQueue;

// Virtual file
typedef struct {
    char name[MAX_NAME_LEN];
    uint8_t* data;
    size_t size;
    size_t capacity;
    uint64_t timestamp;
    bool is_open;
} VFile;

// Virtual filesystem
typedef struct {
    VFile files[MAX_FILES];
    int file_count;
    char current_dir[MAX_NAME_LEN];
} VFS;

// Process Control Block
typedef struct {
    uint32_t pid;
    ProcessState state;
    uint8_t priority;
    uint64_t quantum;
    uint64_t quantum_remaining;
    
    // CPU context
    uint64_t registers[16];
    uint64_t pc;
    uint64_t sp;
    uint64_t flags;
    
    // Memory
    uint64_t* page_table;
    uint32_t page_count;
    uint64_t heap_start;
    uint64_t heap_end;
    
    // Resources
    int open_files[MAX_FILES];
    int message_queues[MAX_QUEUES];
    
    // Statistics
    uint64_t start_time;
    uint64_t cpu_time;
    uint64_t wakeups;
} PCB;

// Page table entry
typedef struct {
    uint64_t virtual_addr;
    uint64_t physical_addr;
    bool present;
    bool writable;
    bool accessed;
    bool dirty;
    uint64_t timestamp;
} PageTableEntry;

// Scheduler
typedef struct {
    PCB* processes[MAX_PROCESSES];
    PCB* current_process;
    int process_count;
    
    // Ready queues for each priority level (0 = highest)
    PCB* ready_queues[16][MAX_PROCESSES];
    int queue_counts[16];
    
    uint64_t system_time;
    uint64_t next_pid;
} Scheduler;

// Memory Manager
typedef struct {
    uint8_t* physical_memory;
    uint64_t mem_size;
    
    PageTableEntry* page_tables[MAX_PROCESSES];
    uint32_t page_table_sizes[MAX_PROCESSES];
    
    uint64_t free_pages;
    uint64_t total_pages;
    bool* page_bitmap;
    
    uint64_t page_faults;
    uint64_t tlb_hits;
    uint64_t tlb_misses;
} MemoryManager;

// Microkernel
typedef struct {
    Scheduler* scheduler;
    MemoryManager* mm;
    MessageQueue* queues[MAX_QUEUES];
    VFS* filesystem;
    
    int queue_count;
    bool running;
} Microkernel;

// Function prototypes
Microkernel* kernel_create(uint64_t mem_size);
void kernel_destroy(Microkernel* kernel);
void kernel_init(Microkernel* kernel);
void kernel_run(Microkernel* kernel, uint64_t cycles);

// Process management
uint32_t kernel_create_process(Microkernel* kernel, void* entry_point);
void kernel_terminate_process(Microkernel* kernel, uint32_t pid);
void kernel_suspend_process(Microkernel* kernel, uint32_t pid);
void kernel_resume_process(Microkernel* kernel, uint32_t pid);

// Scheduling
void scheduler_init(Scheduler* sched);
void scheduler_add_process(Scheduler* sched, PCB* pcb);
PCB* scheduler_next_process(Scheduler* sched);
void scheduler_tick(Scheduler* sched);
void scheduler_print(Scheduler* sched);

// Memory management
MemoryManager* mm_create(uint64_t mem_size);
void mm_destroy(MemoryManager* mm);
uint64_t mm_allocate_pages(MemoryManager* mm, uint32_t pid, uint32_t pages);
void mm_free_pages(MemoryManager* mm, uint32_t pid);
uint64_t mm_translate_address(MemoryManager* mm, uint32_t pid, uint64_t vaddr);
void mm_print_stats(MemoryManager* mm);

// IPC
int kernel_create_queue(Microkernel* kernel);
int kernel_send_message(Microkernel* kernel, int qid, Message* msg);
int kernel_receive_message(Microkernel* kernel, int qid, Message* msg, int timeout);
void kernel_destroy_queue(Microkernel* kernel, int qid);

// Filesystem
VFS* vfs_create();
void vfs_destroy(VFS* vfs);
int vfs_create_file(VFS* vfs, const char* name, size_t size);
int vfs_open_file(VFS* vfs, const char* name);
int vfs_read_file(VFS* vfs, int fd, void* buffer, size_t size);
int vfs_write_file(VFS* vfs, int fd, void* data, size_t size);
void vfs_list_files(VFS* vfs);

#endif