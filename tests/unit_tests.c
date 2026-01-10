#include "cpu.h"
#include "kernel.h"
#include "embedded.h"
#include <assert.h>

void test_cache(void) {
    printf("Testing cache...\n");
    
    Cache* cache = cache_create(CACHE_SET_ASSOC, 8 * KiB, 64, 4);
    assert(cache != NULL);
    
    // Test sequential access
    uint64_t addr = 0;
    for (int i = 0; i < 1000; i++) {
        cache_access(cache, addr, false);
        addr += 64;
    }
    
    assert(cache->accesses == 1000);
    assert(cache->hits > 0);  // Should have some hits after warmup
    
    cache_print_stats(cache);
    cache_destroy(cache);
    
    printf("Cache test PASSED\n");
}

void test_branch_predictor(void) {
    printf("Testing branch predictor...\n");
    
    BranchPredictor* bp = bp_create(PREDICTOR_BIMODAL, 12, 4096);
    assert(bp != NULL);
    
    // Test simple pattern
    uint64_t pc = 0x1000;
    bool pattern[] = {true, true, false, true};
    
    for (int i = 0; i < 100; i++) {
        bool taken = pattern[i % 4];
        bool predicted = bp_predict(bp, pc);
        bp_update(bp, pc, taken, predicted);
        pc += 4;
    }
    
    assert(bp->total == 100);
    assert(bp->correct > 50);  // Should be better than random
    
    bp_print_stats(bp);
    bp_destroy(bp);
    
    printf("Branch predictor test PASSED\n");
}

void test_scheduler(void) {
    printf("Testing scheduler...\n");
    
    Scheduler sched;
    scheduler_init(&sched);
    
    // Create test processes
    for (int i = 0; i < 5; i++) {
        PCB* pcb = pcb_create(sched.next_pid++, NULL);
        pcb->priority = i % 3;
        scheduler_add_process(&sched, pcb);
    }
    
    assert(sched.process_count == 5);
    
    // Run scheduler for some ticks
    for (int i = 0; i < 100; i++) {
        scheduler_tick(&sched);
    }
    
    // Check that all processes got CPU time
    for (int i = 0; i < sched.process_count; i++) {
        assert(sched.processes[i]->cpu_time > 0);
    }
    
    scheduler_print(&sched);
    
    // Cleanup
    for (int i = 0; i < sched.process_count; i++) {
        pcb_destroy(sched.processes[i]);
    }
    
    printf("Scheduler test PASSED\n");
}

void test_memory_manager(void) {
    printf("Testing memory manager...\n");
    
    MemoryManager* mm = mm_create(16 * MiB);
    assert(mm != NULL);
    
    // Allocate some pages
    uint64_t addr1 = mm_allocate_pages(mm, 1, 4);
    assert(addr1 != 0);
    
    uint64_t addr2 = mm_allocate_pages(mm, 2, 8);
    assert(addr2 != 0);
    
    // Test address translation
    uint64_t phys1 = mm_translate_address(mm, 1, 0x1000);
    assert(phys1 != 0);
    
    uint64_t phys2 = mm_translate_address(mm, 2, 0x2000);
    assert(phys2 != 0);
    
    // Free pages
    mm_free_pages(mm, 1);
    mm_free_pages(mm, 2);
    
    mm_print_stats(mm);
    mm_destroy(mm);
    
    printf("Memory manager test PASSED\n");
}

void test_vfs(void) {
    printf("Testing virtual filesystem...\n");
    
    VFS* vfs = vfs_create();
    assert(vfs != NULL);
    
    // Create a file
    int fd = vfs_create_file(vfs, "test.txt", 1024);
    assert(fd >= 0);
    
    // Open the file
    int open_fd = vfs_open_file(vfs, "test.txt");
    assert(open_fd == fd);
    
    // Write to file
    char data[] = "Hello, World!";
    int written = vfs_write_file(vfs, fd, data, strlen(data));
    assert(written == strlen(data));
    
    // Read from file
    char buffer[100];
    int read = vfs_read_file(vfs, fd, buffer, sizeof(buffer));
    assert(read == strlen(data));
    assert(strcmp(buffer, data) == 0);
    
    vfs_list_files(vfs);
    vfs_destroy(vfs);
    
    printf("VFS test PASSED\n");
}

void test_rtos(void) {
    printf("Testing RTOS...\n");
    
    RTOS* rtos = rtos_create();
    assert(rtos != NULL);
    
    // Create a simple task
    uint32_t task_id = rtos_create_task(rtos, NULL, NULL,
                                       PRIO_NORMAL, 100, 10);
    assert(task_id > 0);
    
    // Run scheduler for a while
    uint64_t start = get_time_ms();
    while (get_time_ms() - start < 1000) {
        rtos_schedule(rtos);
        usleep(1000);
    }
    
    rtos_print_stats(rtos);
    rtos_destroy(rtos);
    
    printf("RTOS test PASSED\n");
}

void test_power_management(void) {
    printf("Testing power management...\n");
    
    PowerManager* pm = pm_create();
    assert(pm != NULL);
    
    // Test state transitions
    pm_enter_state(pm, POWER_RUN);
    assert(pm->state == POWER_RUN);
    assert(pm->cpu_powered == true);
    
    pm_enter_state(pm, POWER_IDLE);
    assert(pm->state == POWER_IDLE);
    assert(pm->cpu_powered == false);
    
    pm_enter_state(pm, POWER_SLEEP);
    assert(pm->state == POWER_SLEEP);
    assert(pm->peripherals_powered == false);
    
    pm_update(pm);
    pm_print_stats(pm);
    
    pm_destroy(pm);
    printf("Power management test PASSED\n");
}

void run_all_tests(void) {
    printf("=== Running Unit Tests ===\n");
    
    test_cache();
    test_branch_predictor();
    test_scheduler();
    test_memory_manager();
    test_vfs();
    test_rtos();
    test_power_management();
    
    printf("\n=== All tests PASSED ===\n");
}

int main(void) {
    run_all_tests();
    return 0;
}