#include "cpu.h"

Cache* cache_create(CacheType type, int size, int line_size, int associativity) {
    Cache* cache = (Cache*)malloc(sizeof(Cache));
    if (!cache) return NULL;
    
    cache->type = type;
    cache->size = size;
    cache->line_size = line_size;
    cache->associativity = associativity;
    cache->hit_time = 1;
    cache->miss_penalty = 10;
    
    cache->hits = 0;
    cache->misses = 0;
    cache->accesses = 0;
    
    // Calculate number of sets
    int lines = size / line_size;
    cache->num_sets = lines / associativity;
    
    // Allocate tag arrays
    cache->tags = (uint64_t**)malloc(cache->num_sets * sizeof(uint64_t*));
    cache->valid = (bool**)malloc(cache->num_sets * sizeof(bool*));
    
    for (int i = 0; i < cache->num_sets; i++) {
        cache->tags[i] = (uint64_t*)malloc(associativity * sizeof(uint64_t));
        cache->valid[i] = (bool*)malloc(associativity * sizeof(bool));
        
        for (int j = 0; j < associativity; j++) {
            cache->tags[i][j] = 0;
            cache->valid[i][j] = false;
        }
    }
    
    // Allocate LRU counters for set-associative
    if (type == CACHE_SET_ASSOC) {
        cache->lru_counters = (uint64_t*)malloc(cache->num_sets * associativity * sizeof(uint64_t));
        memset(cache->lru_counters, 0, cache->num_sets * associativity * sizeof(uint64_t));
    } else {
        cache->lru_counters = NULL;
    }
    
    return cache;
}

void cache_destroy(Cache* cache) {
    if (cache) {
        for (int i = 0; i < cache->num_sets; i++) {
            free(cache->tags[i]);
            free(cache->valid[i]);
        }
        free(cache->tags);
        free(cache->valid);
        free(cache->lru_counters);
        free(cache);
    }
}

int cache_access(Cache* cache, uint64_t addr, bool is_write) {
    cache->accesses++;
    
    // Calculate set index and tag
    uint64_t line_addr = addr / cache->line_size;
    int set_index = line_addr % cache->num_sets;
    uint64_t tag = line_addr / cache->num_sets;
    
    // Search for tag in the set
    for (int i = 0; i < cache->associativity; i++) {
        if (cache->valid[set_index][i] && cache->tags[set_index][i] == tag) {
            // Hit
            cache->hits++;
            
            // Update LRU
            if (cache->type == CACHE_SET_ASSOC) {
                cache->lru_counters[set_index * cache->associativity + i] = cache->accesses;
            }
            
            return cache->hit_time;
        }
    }
    
    // Miss
    cache->misses++;
    
    // Find victim for replacement
    int victim = 0;
    if (cache->type == CACHE_SET_ASSOC) {
        // LRU replacement
        uint64_t min_lru = UINT64_MAX;
        for (int i = 0; i < cache->associativity; i++) {
            if (!cache->valid[set_index][i]) {
                victim = i;
                break;
            }
            uint64_t lru = cache->lru_counters[set_index * cache->associativity + i];
            if (lru < min_lru) {
                min_lru = lru;
                victim = i;
            }
        }
    }
    
    // Install new line
    cache->valid[set_index][victim] = true;
    cache->tags[set_index][victim] = tag;
    
    if (cache->type == CACHE_SET_ASSOC) {
        cache->lru_counters[set_index * cache->associativity + victim] = cache->accesses;
    }
    
    return cache->miss_penalty;
}

void cache_print_stats(Cache* cache) {
    printf("\n=== Cache Stats ===\n");
    printf("Size: %d KB\n", cache->size / KiB);
    printf("Line size: %d bytes\n", cache->line_size);
    printf("Associativity: %d\n", cache->associativity);
    printf("Accesses: %lu\n", cache->accesses);
    printf("Hits: %lu\n", cache->hits);
    printf("Misses: %lu\n", cache->misses);
    printf("Hit rate: %.2f%%\n", 
           cache->accesses > 0 ? (100.0 * cache->hits / cache->accesses) : 0.0);
}