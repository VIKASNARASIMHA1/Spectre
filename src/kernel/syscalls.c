#include "kernel.h"

// System call numbers
typedef enum {
    SYS_EXIT = 0,
    SYS_FORK,
    SYS_READ,
    SYS_WRITE,
    SYS_OPEN,
    SYS_CLOSE,
    SYS_EXEC,
    SYS_WAIT,
    SYS_BRK,
    SYS_MMAP,
    SYS_MUNMAP,
    SYS_GETPID,
    SYS_GETTIME,
    SYS_SLEEP,
    SYS_YIELD,
    SYS_SEND,
    SYS_RECV,
    SYS_IOCTL,
    SYS_MAX
} SyscallNumber;

// System call handler
uint64_t syscall_handler(Microkernel* kernel, PCB* pcb, 
                        uint64_t syscall_num, uint64_t arg1, 
                        uint64_t arg2, uint64_t arg3, uint64_t arg4) {
    
    switch (syscall_num) {
        case SYS_EXIT:
            kernel_terminate_process(kernel, pcb->pid);
            return 0;
            
        case SYS_GETPID:
            return pcb->pid;
            
        case SYS_GETTIME:
            return get_time_ms();
            
        case SYS_SLEEP:
            // Simple sleep implementation
            {
                uint64_t end = get_time_ms() + arg1;
                while (get_time_ms() < end) {
                    // Yield CPU
                    pcb->state = PROC_BLOCKED;
                    scheduler_tick(kernel->scheduler);
                }
            }
            return 0;
            
        case SYS_YIELD:
            pcb->state = PROC_READY;
            pcb->quantum_remaining = 0;  // Force reschedule
            return 0;
            
        case SYS_BRK:
            // Simple heap expansion
            {
                uint64_t new_brk = arg1;
                if (new_brk > pcb->heap_end) {
                    uint64_t pages_needed = ALIGN_UP(new_brk - pcb->heap_end, PAGE_SIZE) / PAGE_SIZE;
                    mm_allocate_pages(kernel->mm, pcb->pid, pages_needed);
                }
                pcb->heap_end = new_brk;
                return new_brk;
            }
            
        case SYS_OPEN:
            return vfs_open_file(kernel->filesystem, (const char*)arg1);
            
        case SYS_CLOSE:
            // Find and close file
            for (int i = 0; i < MAX_FILES; i++) {
                if (pcb->open_files[i] == (int)arg1) {
                    pcb->open_files[i] = -1;
                    // In real implementation, would close file in VFS
                    return 0;
                }
            }
            return -1;  // EBADF
            
        case SYS_READ:
            if (arg2 < MAX_FILES && pcb->open_files[arg2] != -1) {
                return vfs_read_file(kernel->filesystem, pcb->open_files[arg2],
                                   (void*)arg1, arg3);
            }
            return -1;  // EBADF
            
        case SYS_WRITE:
            if (arg2 < MAX_FILES && pcb->open_files[arg2] != -1) {
                return vfs_write_file(kernel->filesystem, pcb->open_files[arg2],
                                    (void*)arg1, arg3);
            }
            return -1;  // EBADF
            
        case SYS_SEND:
            {
                Message msg;
                msg.src_pid = pcb->pid;
                msg.dst_pid = arg1;
                msg.msg_id = arg2;
                msg.data = (void*)arg3;
                msg.size = arg4;
                return kernel_send_message(kernel, arg1, &msg);
            }
            
        case SYS_RECV:
            {
                Message msg;
                int result = kernel_receive_message(kernel, arg1, &msg, arg2);
                if (result == 0) {
                    // Copy message to user buffer
                    if (msg.size <= arg4) {
                        memcpy((void*)arg3, msg.data, msg.size);
                        return msg.size;
                    } else {
                        return -1;  // EMSGSIZE
                    }
                }
                return result;
            }
            
        default:
            ERROR("Unknown syscall: %lu\n", syscall_num);
            return -1;  // ENOSYS
    }
}

// System call table
static uint64_t (*syscall_table[SYS_MAX])(Microkernel*, PCB*, uint64_t, uint64_t, uint64_t, uint64_t) = {
    [SYS_EXIT] = syscall_handler,
    [SYS_GETPID] = syscall_handler,
    [SYS_GETTIME] = syscall_handler,
    [SYS_SLEEP] = syscall_handler,
    [SYS_YIELD] = syscall_handler,
    [SYS_BRK] = syscall_handler,
    [SYS_OPEN] = syscall_handler,
    [SYS_CLOSE] = syscall_handler,
    [SYS_READ] = syscall_handler,
    [SYS_WRITE] = syscall_handler,
    [SYS_SEND] = syscall_handler,
    [SYS_RECV] = syscall_handler,
};

// Invoke system call from user process
uint64_t syscall_invoke(Microkernel* kernel, PCB* pcb, uint64_t num,
                       uint64_t arg1, uint64_t arg2, 
                       uint64_t arg3, uint64_t arg4) {
    
    if (num >= SYS_MAX || syscall_table[num] == NULL) {
        return -1;  // ENOSYS
    }
    
    return syscall_table[num](kernel, pcb, num, arg1, arg2, arg3, arg4);
}