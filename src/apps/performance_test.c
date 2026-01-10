#include "cpu.h"
#include "kernel.h"
#include "embedded.h"
#include <time.h>

#define TEST_ITERATIONS 1000

void performance_test_cpu() {
    printf("\n=== CPU Performance Test ===\n");
    
    // Test different cache configurations
    CacheConfig configs[] = {
        {CACHE_DIRECT_MAPPED, 4 * KiB, 64, 1},
        {CACHE_DIRECT_MAPPED, 8 * KiB, 64, 1},
        {CACHE_SET_ASSOC, 8 * KiB, 64, 4},
        {CACHE_SET_ASSOC, 16 * KiB, 64, 8},
        {CACHE_FULL_ASSOC, 32 * KiB, 64, 512},
    };
    
    const char* config_names[] = {
        "DM 4KB",
        "DM 8KB", 
        "4-way 8KB",
        "8-way 16KB",
        "FA 32KB"
    };
    
    // Test patterns
    typedef struct {
        const char* name;
        void (*pattern)(Cache*, uint64_t);
    } TestPattern;
    
    // Sequential access
    void seq_access(Cache* cache, uint64_t iterations) {
        uint64_t addr = 0;
        for (uint64_t i = 0; i < iterations; i++) {
            cache_access(cache, addr, false);
            addr = (addr + 64) % (cache->size * 4);  // Access 4x cache size
        }
    }
    
    // Random access
    void rand_access(Cache* cache, uint64_t iterations) {
        srand(time(NULL));
        for (uint64_t i = 0; i < iterations; i++) {
            uint64_t addr = rand() % (cache->size * 16);  // Large address space
            cache_access(cache, addr, false);
        }
    }
    
    // Strided access (cache thrashing)
    void strided_access(Cache* cache, uint64_t iterations) {
        uint64_t stride = cache->size * 2;  // Large stride to cause misses
        uint64_t addr = 0;
        for (uint64_t i = 0; i < iterations; i++) {
            cache_access(cache, addr, false);
            addr = (addr + stride) % (cache->size * 8);
        }
    }
    
    TestPattern patterns[] = {
        {"Sequential", seq_access},
        {"Random", rand_access},
        {"Strided", strided_access}
    };
    
    printf("\nCache Performance Comparison:\n");
    printf("%-15s %-10s %-12s %-10s %-10s\n", 
           "Config", "Pattern", "Accesses", "Hit Rate", "Time(ms)");
    printf("------------------------------------------------------------\n");
    
    for (int c = 0; c < 5; c++) {
        for (int p = 0; p < 3; p++) {
            Cache* cache = cache_create(configs[c].type, 
                                       configs[c].size,
                                       configs[c].line_size,
                                       configs[c].associativity);
            
            clock_t start = clock();
            patterns[p].pattern(cache, TEST_ITERATIONS * 100);
            clock_t end = clock();
            
            double hit_rate = 100.0 * cache->hits / cache->accesses;
            double time_ms = ((double)(end - start) / CLOCKS_PER_SEC) * 1000;
            
            printf("%-15s %-10s %-12lu %-10.2f %-10.2f\n",
                   config_names[c],
                   patterns[p].name,
                   cache->accesses,
                   hit_rate,
                   time_ms);
            
            cache_destroy(cache);
        }
        printf("\n");
    }
}

void performance_test_scheduler() {
    printf("\n=== Scheduler Performance Test ===\n");
    
    // Test with increasing number of processes
    for (int num_procs = 10; num_procs <= 100; num_procs += 10) {
        Scheduler sched;
        scheduler_init(&sched);
        
        // Create processes with different priorities
        for (int i = 0; i < num_procs; i++) {
            PCB* pcb = pcb_create(sched.next_pid++, NULL);
            pcb->priority = i % 5;  // Mix of priorities
            pcb->quantum = 10 + (i % 20);
            scheduler_add_process(&sched, pcb);
        }
        
        clock_t start = clock();
        
        // Run scheduler for fixed number of ticks
        for (int tick = 0; tick < 1000; tick++) {
            scheduler_tick(&sched);
        }
        
        clock_t end = clock();
        double time_ms = ((double)(end - start) / CLOCKS_PER_SEC) * 1000;
        
        printf("Processes: %3d, Time: %6.2f ms, Avg time per context switch: %.3f ms\n",
               num_procs, time_ms, time_ms / 1000);
        
        // Cleanup
        for (int i = 0; i < sched.process_count; i++) {
            pcb_destroy(sched.processes[i]);
        }
    }
}

void performance_test_memory() {
    printf("\n=== Memory Manager Performance Test ===\n");
    
    MemoryManager* mm = mm_create(64 * MiB);
    
    // Test allocation speed
    clock_t start = clock();
    
    for (int i = 0; i < 1000; i++) {
        mm_allocate_pages(mm, 0, 4);  // Allocate 4 pages (16KB)
    }
    
    clock_t end = clock();
    double alloc_time = ((double)(end - start) / CLOCKS_PER_SEC) * 1000;
    
    // Test translation speed
    start = clock();
    
    uint64_t total_translations = 0;
    for (int i = 0; i < 100000; i++) {
        mm_translate_address(mm, 0, i * PAGE_SIZE);
        total_translations++;
    }
    
    end = clock();
    double trans_time = ((double)(end - start) / CLOCKS_PER_SEC) * 1000;
    
    printf("Allocation test:\n");
    printf("  1000 allocations (4 pages each): %.2f ms\n", alloc_time);
    printf("  Average per allocation: %.3f ms\n", alloc_time / 1000);
    
    printf("\nTranslation test:\n");
    printf("  100,000 address translations: %.2f ms\n", trans_time);
    printf("  Average per translation: %.3f us\n", (trans_time * 1000) / 100000);
    printf("  Throughput: %.2f translations/ms\n", 100000 / trans_time);
    
    mm_print_stats(mm);
    mm_destroy(mm);
}

int main() {
    printf("=== Performance Test Suite ===\n");
    
    performance_test_cpu();
    performance_test_scheduler();
    performance_test_memory();
    
    return 0;
}