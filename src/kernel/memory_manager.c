#include "kernel.h"

MemoryManager* mm_create(uint64_t mem_size) {
    MemoryManager* mm = (MemoryManager*)malloc(sizeof(MemoryManager));
    if (!mm) return NULL;
    
    mm->mem_size = mem_size;
    mm->physical_memory = (uint8_t*)malloc(mem_size);
    if (!mm->physical_memory) {
        free(mm);
        return NULL;
    }
    
    // Initialize page tables
    memset(mm->page_tables, 0, sizeof(mm->page_tables));
    memset(mm->page_table_sizes, 0, sizeof(mm->page_table_sizes));
    
    // Initialize page bitmap
    mm->total_pages = mem_size / PAGE_SIZE;
    mm->free_pages = mm->total_pages;
    mm->page_bitmap = (bool*)malloc(mm->total_pages * sizeof(bool));
    if (!mm->page_bitmap) {
        free(mm->physical_memory);
        free(mm);
        return NULL;
    }
    
    // Mark all pages as free initially
    for (uint64_t i = 0; i < mm->total_pages; i++) {
        mm->page_bitmap[i] = false;  // false = free
    }
    
    mm->page_faults = 0;
    mm->tlb_hits = 0;
    mm->tlb_misses = 0;
    
    return mm;
}

void mm_destroy(MemoryManager* mm) {
    if (mm) {
        free(mm->physical_memory);
        free(mm->page_bitmap);
        
        // Free all page tables
        for (int i = 0; i < MAX_PROCESSES; i++) {
            if (mm->page_tables[i]) {
                free(mm->page_tables[i]);
            }
        }
        
        free(mm);
    }
}

uint64_t mm_allocate_pages(MemoryManager* mm, uint32_t pid, uint32_t pages) {
    if (pid >= MAX_PROCESSES) {
        ERROR("Invalid PID\n");
        return 0;
    }
    
    if (pages == 0) return 0;
    
    // Find contiguous free pages
    uint64_t start_page = 0;
    uint64_t consecutive = 0;
    
    for (uint64_t i = 0; i < mm->total_pages; i++) {
        if (!mm->page_bitmap[i]) {
            if (consecutive == 0) start_page = i;
            consecutive++;
            
            if (consecutive == pages) {
                // Found enough contiguous pages
                for (uint64_t j = start_page; j < start_page + pages; j++) {
                    mm->page_bitmap[j] = true;  // Mark as allocated
                }
                
                mm->free_pages -= pages;
                
                // Create or extend page table
                if (!mm->page_tables[pid]) {
                    mm->page_tables[pid] = (PageTableEntry*)malloc(MAX_PAGES * sizeof(PageTableEntry));
                    if (!mm->page_tables[pid]) {
                        return 0;
                    }
                    memset(mm->page_tables[pid], 0, MAX_PAGES * sizeof(PageTableEntry));
                    mm->page_table_sizes[pid] = 0;
                }
                
                // Setup page table entries
                for (uint32_t j = 0; j < pages; j++) {
                    PageTableEntry* pte = &mm->page_tables[pid][mm->page_table_sizes[pid] + j];
                    pte->virtual_addr = (mm->page_table_sizes[pid] + j) * PAGE_SIZE;
                    pte->physical_addr = (start_page + j) * PAGE_SIZE;
                    pte->present = true;
                    pte->writable = true;
                    pte->accessed = false;
                    pte->dirty = false;
                    pte->timestamp = 0;
                }
                
                mm->page_table_sizes[pid] += pages;
                
                return start_page * PAGE_SIZE;
            }
        } else {
            consecutive = 0;
        }
    }
    
    ERROR("Not enough contiguous pages available\n");
    return 0;
}

void mm_free_pages(MemoryManager* mm, uint32_t pid) {
    if (pid >= MAX_PROCESSES || !mm->page_tables[pid]) {
        return;
    }
    
    // Free all pages allocated to this process
    for (uint32_t i = 0; i < mm->page_table_sizes[pid]; i++) {
        PageTableEntry* pte = &mm->page_tables[pid][i];
        if (pte->present) {
            uint64_t page_num = pte->physical_addr / PAGE_SIZE;
            if (page_num < mm->total_pages) {
                mm->page_bitmap[page_num] = false;  // Mark as free
                mm->free_pages++;
            }
        }
    }
    
    // Free page table
    free(mm->page_tables[pid]);
    mm->page_tables[pid] = NULL;
    mm->page_table_sizes[pid] = 0;
}

uint64_t mm_translate_address(MemoryManager* mm, uint32_t pid, uint64_t vaddr) {
    if (pid >= MAX_PROCESSES || !mm->page_tables[pid]) {
        mm->page_faults++;
        return 0;
    }
    
    uint64_t vpage = vaddr / PAGE_SIZE;
    uint64_t offset = vaddr % PAGE_SIZE;
    
    // Simple linear search (in real system, use page table walk)
    for (uint32_t i = 0; i < mm->page_table_sizes[pid]; i++) {
        PageTableEntry* pte = &mm->page_tables[pid][i];
        if (pte->present && (pte->virtual_addr / PAGE_SIZE) == vpage) {
            pte->accessed = true;
            mm->tlb_hits++;
            return pte->physical_addr + offset;
        }
    }
    
    // Page fault
    mm->page_faults++;
    mm->tlb_misses++;
    
    // Try to allocate a new page
    uint64_t phys_addr = mm_allocate_pages(mm, pid, 1);
    if (phys_addr == 0) {
        ERROR("Page fault cannot be resolved\n");
        return 0;
    }
    
    // Setup new page table entry
    if (mm->page_table_sizes[pid] < MAX_PAGES) {
        PageTableEntry* pte = &mm->page_tables[pid][mm->page_table_sizes[pid] - 1];
        pte->virtual_addr = vpage * PAGE_SIZE;
        pte->physical_addr = phys_addr;
        pte->present = true;
        pte->writable = true;
        pte->accessed = true;
        pte->dirty = false;
        pte->timestamp = 0;
    }
    
    return phys_addr + offset;
}

void mm_print_stats(MemoryManager* mm) {
    printf("\n=== Memory Manager Stats ===\n");
    printf("Total memory: %lu MB\n", mm->mem_size / MiB);
    printf("Total pages: %lu\n", mm->total_pages);
    printf("Free pages: %lu\n", mm->free_pages);
    printf("Used pages: %lu\n", mm->total_pages - mm->free_pages);
    printf("Page faults: %lu\n", mm->page_faults);
    printf("TLB hits: %lu\n", mm->tlb_hits);
    printf("TLB misses: %lu\n", mm->tlb_misses);
    printf("Hit rate: %.2f%%\n", 
           (mm->tlb_hits + mm->tlb_misses) > 0 ? 
           (100.0 * mm->tlb_hits / (mm->tlb_hits + mm->tlb_misses)) : 0.0);
}