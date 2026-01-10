#include "cpu.h"
#include "kernel.h"
#include "embedded.h"
#include <assert.h>

void test_integration_cpu_kernel(void) {
    printf("Testing CPU-Kernel Integration...\n");
    
    // Create CPU and kernel
    CPU* cpu = cpu_create(64 * KiB);
    Microkernel* kernel = kernel_create(16 * MiB);
    
    assert(cpu != NULL);
    assert(kernel != NULL);
    
    // Create a process that runs CPU instructions
    uint32_t pid = kernel_create_process(kernel, (void*)0x1000);
    assert(pid > 0);
    
    // Load a simple program into CPU memory
    uint8_t program[] = {
        INST_MOV, 0, 10,    // R0 = 10
        INST_MOV, 1, 20,    // R1 = 20
        INST_ADD, 2, 0, 1,  // R2 = R0 + R1
        INST_HLT
    };
    
    cpu_load_program(cpu, program, sizeof(program), 0x1000);
    
    // Run CPU for some cycles
    cpu_run(cpu, 100);
    
    // Check result
    assert(cpu->registers[2] == 30);
    
    // Cleanup
    cpu_destroy(cpu);
    kernel_destroy(kernel);
    
    printf("CPU-Kernel integration test PASSED\n");
}

void test_integration_kernel_embedded(void) {
    printf("Testing Kernel-Embedded Integration...\n");
    
    // Create kernel and RTOS
    Microkernel* kernel = kernel_create(16 * MiB);
    RTOS* rtos = rtos_create();
    
    assert(kernel != NULL);
    assert(rtos != NULL);
    
    // Create a kernel process that uses RTOS-like features
    uint32_t pid = kernel_create_process(kernel, (void*)0x2000);
    
    // Create RTOS task
    uint32_t task_id = rtos_create_task(rtos, NULL, NULL,
                                       PRIO_NORMAL, 100, 10);
    
    assert(pid > 0);
    assert(task_id > 0);
    
    // Run both for a while
    for (int i = 0; i < 10; i++) {
        scheduler_tick(kernel->scheduler);
        rtos_schedule(rtos);
    }
    
    // Verify both are functional
    assert(kernel->scheduler->process_count > 0);
    assert(rtos->task_count > 0);
    
    kernel_destroy(kernel);
    rtos_destroy(rtos);
    
    printf("Kernel-Embedded integration test PASSED\n");
}

void test_integration_complete_system(void) {
    printf("Testing Complete System Integration...\n");
    
    // Full system test
    CPU* cpu = cpu_create(64 * KiB);
    Microkernel* kernel = kernel_create(16 * MiB);
    RTOS* rtos = rtos_create();
    
    // 1. CPU executes instructions
    uint8_t program[] = {
        INST_MOV, 0, 42,
        INST_HLT
    };
    cpu_load_program(cpu, program, sizeof(program), 0x1000);
    cpu_run(cpu, 50);
    
    // 2. Kernel manages processes
    uint32_t pid1 = kernel_create_process(kernel, (void*)0x1000);
    uint32_t pid2 = kernel_create_process(kernel, (void*)0x2000);
    
    // 3. RTOS runs real-time tasks
    uint32_t task1 = rtos_create_task(rtos, NULL, NULL, PRIO_HIGH, 50, 5);
    uint32_t task2 = rtos_create_task(rtos, NULL, NULL, PRIO_NORMAL, 100, 10);
    
    // 4. Run integrated simulation
    for (int cycle = 0; cycle < 1000; cycle++) {
        // CPU cycle
        if (cycle % 10 == 0) {
            cpu_step(cpu);
        }
        
        // Kernel tick
        if (cycle % 5 == 0) {
            scheduler_tick(kernel->scheduler);
        }
        
        // RTOS schedule
        if (cycle % 2 == 0) {
            rtos_schedule(rtos);
        }
    }
    
    // Verify all components worked
    assert(cpu->instructions > 0);
    assert(kernel->scheduler->process_count == 2);
    assert(rtos->task_count == 2);
    
    printf("CPU executed %lu instructions\n", cpu->instructions);
    printf("Kernel has %d processes\n", kernel->scheduler->process_count);
    printf("RTOS has %u tasks\n", rtos->task_count);
    
    cpu_destroy(cpu);
    kernel_destroy(kernel);
    rtos_destroy(rtos);
    
    printf("Complete system integration test PASSED\n");
}

void test_performance_integration(void) {
    printf("Testing Performance Integration...\n");
    
    // Test that performance metrics work across all systems
    CPU* cpu = cpu_create(64 * KiB);
    
    // Run workload
    uint8_t workload[1024];
    for (int i = 0; i < sizeof(workload); i++) {
        workload[i] = INST_ADD + (i % 5);
    }
    
    cpu_load_program(cpu, workload, sizeof(workload), 0x1000);
    
    uint64_t start = get_time_ms();
    cpu_run(cpu, 10000);
    uint64_t end = get_time_ms();
    
    printf("Performance metrics:\n");
    printf("  Execution time: %lu ms\n", end - start);
    printf("  Instructions: %lu\n", cpu->instructions);
    printf("  IPC: %.3f\n", (float)cpu->instructions / cpu->cycles);
    printf("  Cache hit rate: %.1f%%\n",
           100.0 * cpu->l1_cache->hits / cpu->l1_cache->accesses);
    
    cpu_print_stats(cpu);
    cpu_destroy(cpu);
    
    printf("Performance integration test PASSED\n");
}

void run_integration_tests(void) {
    printf("=== Running Integration Tests ===\n");
    
    test_integration_cpu_kernel();
    test_integration_kernel_embedded();
    test_integration_complete_system();
    test_performance_integration();
    
    printf("\n=== All integration tests PASSED ===\n");
}

int main(void) {
    run_integration_tests();
    return 0;
}