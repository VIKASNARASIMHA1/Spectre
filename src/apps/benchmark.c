#include "cpu.h"
#include "kernel.h"
#include "embedded.h"

// Benchmark programs
uint8_t fibonacci_program[] = {
    // Calculate fibonacci(10)
    INST_MOV, 10,    // R0 = 10 (n)
    INST_MOV, 0,     // R1 = 0 (a)
    INST_MOV, 1,     // R2 = 1 (b)
    0, 0, 0, 0,      // Padding
};

uint8_t matrix_multiply[] = {
    // Simple 2x2 matrix multiplication
    INST_LD, 0, 0,   // Load matrix A[0][0]
    INST_LD, 1, 4,   // Load matrix A[0][1]
    INST_LD, 2, 8,   // Load matrix B[0][0]
    INST_LD, 3, 12,  // Load matrix B[1][0]
    INST_MUL, 0, 2,  // R4 = A00 * B00
    INST_MUL, 1, 3,  // R5 = A01 * B10
    INST_ADD, 4, 5,  // R6 = R4 + R5
    INST_ST, 6, 16,  // Store result
    INST_HLT
};

void benchmark_cpu() {
    printf("\n=== CPU Benchmark ===\n");
    
    CPU* cpu = cpu_create(64 * KiB);
    
    // Test 1: Fibonacci calculation
    printf("Test 1: Fibonacci calculation\n");
    cpu_reset(cpu);
    cpu_load_program(cpu, fibonacci_program, sizeof(fibonacci_program), 0x1000);
    
    uint64_t start = get_time_ms();
    cpu_run(cpu, 1000);
    uint64_t end = get_time_ms();
    
    cpu_print_stats(cpu);
    printf("Execution time: %lu ms\n", end - start);
    printf("Performance: %.2f instructions/ms\n", 
           (float)cpu->instructions / (end - start));
    
    // Test 2: Matrix multiplication
    printf("\nTest 2: Matrix multiplication\n");
    cpu_reset(cpu);
    cpu_load_program(cpu, matrix_multiply, sizeof(matrix_multiply), 0x1000);
    
    // Initialize matrices in memory
    uint32_t* mem = (uint32_t*)cpu->memory;
    mem[0] = 1; mem[1] = 2;  // A = [1 2; 3 4]
    mem[2] = 3; mem[3] = 4;
    mem[4] = 5; mem[5] = 6;  // B = [5 6; 7 8]
    mem[6] = 7; mem[7] = 8;
    
    start = get_time_ms();
    cpu_run(cpu, 500);
    end = get_time_ms();
    
    cpu_print_stats(cpu);
    printf("Result: %u (expected: 19)\n", mem[8]);
    
    cpu_destroy(cpu);
}

void benchmark_cache() {
    printf("\n=== Cache Benchmark ===\n");
    
    // Test different cache configurations
    Cache* caches[] = {
        cache_create(CACHE_DIRECT_MAPPED, 8 * KiB, 64, 1),
        cache_create(CACHE_SET_ASSOC, 8 * KiB, 64, 4),
        cache_create(CACHE_SET_ASSOC, 8 * KiB, 64, 8),
        cache_create(CACHE_FULL_ASSOC, 8 * KiB, 64, 128),
    };
    
    const char* names[] = {
        "Direct Mapped 8KB",
        "4-way Set Assoc 8KB",
        "8-way Set Assoc 8KB",
        "Full Assoc 8KB"
    };
    
    // Sequential access pattern
    printf("Sequential access pattern:\n");
    for (int c = 0; c < 4; c++) {
        Cache* cache = caches[c];
        
        uint64_t addr = 0;
        for (int i = 0; i < 10000; i++) {
            cache_access(cache, addr, false);
            addr += 64;  // Next cache line
        }
        
        printf("  %s: Hit rate = %.2f%%\n", 
               names[c], 100.0 * cache->hits / cache->accesses);
        
        cache_destroy(cache);
    }
}

void benchmark_scheduler() {
    printf("\n=== Scheduler Benchmark ===\n");
    
    Scheduler sched;
    scheduler_init(&sched);
    
    // Create some test processes
    for (int i = 0; i < 10; i++) {
        PCB* pcb = pcb_create(sched.next_pid++, NULL);
        pcb->priority = i % 4;
        pcb->quantum = 50 + (i * 10);
        scheduler_add_process(&sched, pcb);
    }
    
    printf("Running scheduler for 1000 ticks...\n");
    
    uint64_t start = get_time_ms();
    for (int i = 0; i < 1000; i++) {
        scheduler_tick(&sched);
    }
    uint64_t end = get_time_ms();
    
    scheduler_print(&sched);
    printf("Scheduling time: %lu ms\n", end - start);
    
    // Cleanup
    for (int i = 0; i < sched.process_count; i++) {
        pcb_destroy(sched.processes[i]);
    }
}

int main() {
    printf("=== Spectre Simulator Benchmark Suite ===\n");
    
    benchmark_cpu();
    benchmark_cache();
    benchmark_scheduler();
    
    printf("\n=== Demo: Traffic Light Controller ===\n");
    demo_traffic_light();
    
    return 0;
}