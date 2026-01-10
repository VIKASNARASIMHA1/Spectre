#include "cpu.h"

#define MEM_SIZE (64 * KiB)

// Helper functions
static uint64_t sign_extend(uint64_t value, int bits) {
    uint64_t mask = 1ULL << (bits - 1);
    return (value ^ mask) - mask;
}

static bool check_hazard(CPU* cpu, PipelineStage stage, uint64_t reg) {
    for (int i = stage + 1; i <= STAGE_COMMIT; i++) {
        if (cpu->pipeline[i].dest == reg && cpu->pipeline[i].type != INST_NOP) {
            return true;
        }
    }
    return false;
}

// Create CPU
CPU* cpu_create(uint64_t mem_size) {
    CPU* cpu = (CPU*)malloc(sizeof(CPU));
    if (!cpu) return NULL;
    
    cpu->mem_size = mem_size;
    cpu->memory = (uint8_t*)malloc(mem_size);
    if (!cpu->memory) {
        free(cpu);
        return NULL;
    }
    
    // Initialize caches
    cpu->l1_cache = cache_create(CACHE_SET_ASSOC, 32 * KiB, 64, 8);
    cpu->l2_cache = cache_create(CACHE_SET_ASSOC, 256 * KiB, 64, 16);
    
    // Initialize branch predictor
    cpu->bp = bp_create(PREDICTOR_BIMODAL, 12, 4096);
    
    cpu_reset(cpu);
    return cpu;
}

void cpu_destroy(CPU* cpu) {
    if (cpu) {
        free(cpu->memory);
        cache_destroy(cpu->l1_cache);
        cache_destroy(cpu->l2_cache);
        bp_destroy(cpu->bp);
        free(cpu);
    }
}

void cpu_reset(CPU* cpu) {
    memset(cpu->registers, 0, sizeof(cpu->registers));
    cpu->pc = 0x1000;
    cpu->sp = 0x8000;
    cpu->flags = 0;
    
    memset(cpu->pipeline, 0, sizeof(cpu->pipeline));
    for (int i = 0; i < 6; i++) {
        cpu->pipeline[i].type = INST_NOP;
    }
    cpu->current_stage = STAGE_FETCH;
    
    cpu->cycles = 0;
    cpu->instructions = 0;
    cpu->stalls = 0;
    cpu->bubbles = 0;
    cpu->start_time = get_time_ms();
}

int cpu_load_program(CPU* cpu, uint8_t* program, uint64_t size, uint64_t address) {
    if (address + size > cpu->mem_size) {
        ERROR("Program too large for memory\n");
        return -1;
    }
    memcpy(cpu->memory + address, program, size);
    cpu->pc = address;
    return 0;
}

// Fetch stage
static void stage_fetch(CPU* cpu) {
    PipelineRegister* pr = &cpu->pipeline[STAGE_FETCH];
    
    if (pr->stall) {
        cpu->stalls++;
        return;
    }
    
    // Check for branch misprediction
    if (cpu->pipeline[STAGE_EXECUTE].type == INST_JMP ||
        cpu->pipeline[STAGE_EXECUTE].type == INST_JZ ||
        cpu->pipeline[STAGE_EXECUTE].type == INST_JNZ) {
        
        bool taken = cpu->pipeline[STAGE_EXECUTE].result != 0;
        bool predicted = bp_predict(cpu->bp, cpu->pipeline[STAGE_EXECUTE].pc);
        
        if (taken != predicted) {
            // Misprediction - flush pipeline
            for (int i = STAGE_DECODE; i <= STAGE_COMMIT; i++) {
                cpu->pipeline[i].type = INST_NOP;
                cpu->pipeline[i].bubble = true;
            }
            cpu->pc = cpu->pipeline[STAGE_EXECUTE].result;
            cpu->bubbles += 3;  // 3 cycle penalty
        }
    }
    
    // Access instruction cache
    cache_access(cpu->l1_cache, cpu->pc, false);
    
    // Simple instruction fetch (in real implementation, decode opcode)
    uint8_t opcode = cpu->memory[cpu->pc];
    pr->pc = cpu->pc;
    pr->opcode = opcode;
    pr->cycle_entered = cpu->cycles;
    
    // Map to instruction type (simplified)
    if (opcode < 20) {
        pr->type = opcode;
    } else {
        pr->type = INST_NOP;
    }
    
    cpu->pc++;
}

// Decode stage
static void stage_decode(CPU* cpu) {
    PipelineRegister* pr = &cpu->pipeline[STAGE_DECODE];
    PipelineRegister* prev = &cpu->pipeline[STAGE_FETCH];
    
    if (prev->bubble) {
        pr->type = INST_NOP;
        pr->bubble = true;
        return;
    }
    
    // Check for data hazards
    if (check_hazard(cpu, STAGE_DECODE, prev->src1) ||
        check_hazard(cpu, STAGE_DECODE, prev->src2)) {
        // Stall pipeline
        cpu->pipeline[STAGE_FETCH].stall = true;
        pr->type = INST_NOP;
        cpu->stalls++;
        return;
    }
    
    *pr = *prev;
    pr->cycle_entered = cpu->cycles;
    
    // Simple register read (in real implementation, read from register file)
    pr->src1 = cpu->registers[pr->opcode & 0x0F];
    pr->src2 = cpu->registers[(pr->opcode >> 4) & 0x0F];
}

// Execute stage
static void stage_execute(CPU* cpu) {
    PipelineRegister* pr = &cpu->pipeline[STAGE_EXECUTE];
    PipelineRegister* prev = &cpu->pipeline[STAGE_DECODE];
    
    if (prev->bubble) {
        pr->type = INST_NOP;
        pr->bubble = true;
        return;
    }
    
    *pr = *prev;
    pr->cycle_entered = cpu->cycles;
    
    // Execute based on instruction type
    switch (pr->type) {
        case INST_ADD:
            pr->result = pr->src1 + pr->src2;
            break;
        case INST_SUB:
            pr->result = pr->src1 - pr->src2;
            break;
        case INST_MUL:
            pr->result = pr->src1 * pr->src2;
            break;
        case INST_AND:
            pr->result = pr->src1 & pr->src2;
            break;
        case INST_OR:
            pr->result = pr->src1 | pr->src2;
            break;
        case INST_XOR:
            pr->result = pr->src1 ^ pr->src2;
            break;
        case INST_SHL:
            pr->result = pr->src1 << (pr->src2 & 0x3F);
            break;
        case INST_SHR:
            pr->result = pr->src1 >> (pr->src2 & 0x3F);
            break;
        case INST_JMP:
            pr->result = pr->immediate;
            break;
        case INST_JZ:
            pr->result = (pr->src1 == 0) ? pr->immediate : (pr->pc + 1);
            break;
        case INST_JNZ:
            pr->result = (pr->src1 != 0) ? pr->immediate : (pr->pc + 1);
            break;
        case INST_CMP:
            cpu->flags = pr->src1 - pr->src2;
            pr->result = cpu->flags;
            break;
        case INST_MOV:
            pr->result = pr->src1;
            break;
        default:
            pr->result = 0;
    }
    
    // Update branch predictor
    if (pr->type == INST_JMP || pr->type == INST_JZ || pr->type == INST_JNZ) {
        bool taken = pr->result != (pr->pc + 1);
        bool predicted = bp_predict(cpu->bp, pr->pc);
        bp_update(cpu->bp, pr->pc, taken, predicted);
    }
}

// Memory stage
static void stage_memory(CPU* cpu) {
    PipelineRegister* pr = &cpu->pipeline[STAGE_MEMORY];
    PipelineRegister* prev = &cpu->pipeline[STAGE_EXECUTE];
    
    if (prev->bubble) {
        pr->type = INST_NOP;
        pr->bubble = true;
        return;
    }
    
    *pr = *prev;
    pr->cycle_entered = cpu->cycles;
    
    // Handle memory operations
    if (pr->type == INST_LD) {
        // Load from memory
        cache_access(cpu->l1_cache, pr->mem_addr, false);
        uint64_t* data = (uint64_t*)(cpu->memory + pr->mem_addr);
        pr->result = *data;
    } else if (pr->type == INST_ST) {
        // Store to memory
        cache_access(cpu->l1_cache, pr->mem_addr, true);
        uint64_t* data = (uint64_t*)(cpu->memory + pr->mem_addr);
        *data = pr->mem_data;
    }
}

// Writeback stage
static void stage_writeback(CPU* cpu) {
    PipelineRegister* pr = &cpu->pipeline[STAGE_WRITEBACK];
    PipelineRegister* prev = &cpu->pipeline[STAGE_MEMORY];
    
    if (prev->bubble) {
        pr->type = INST_NOP;
        pr->bubble = true;
        return;
    }
    
    *pr = *prev;
    pr->cycle_entered = cpu->cycles;
    
    // Write back to register file
    if (pr->dest < 16) {
        cpu->registers[pr->dest] = pr->result;
    }
    
    cpu->instructions++;
}

// Commit stage
static void stage_commit(CPU* cpu) {
    PipelineRegister* pr = &cpu->pipeline[STAGE_COMMIT];
    PipelineRegister* prev = &cpu->pipeline[STAGE_WRITEBACK];
    
    if (prev->bubble) {
        pr->type = INST_NOP;
        pr->bubble = false;  // Clear bubble
    } else {
        *pr = *prev;
    }
    
    // Clear stall flag if set
    cpu->pipeline[STAGE_FETCH].stall = false;
}

// Single CPU step
void cpu_step(CPU* cpu) {
    // Advance pipeline in reverse order
    stage_commit(cpu);
    stage_writeback(cpu);
    stage_memory(cpu);
    stage_execute(cpu);
    stage_decode(cpu);
    stage_fetch(cpu);
    
    cpu->cycles++;
}

// Run for specified cycles
void cpu_run(CPU* cpu, uint64_t cycles) {
    for (uint64_t i = 0; i < cycles; i++) {
        cpu_step(cpu);
    }
}

void cpu_print_stats(CPU* cpu) {
    uint64_t elapsed = get_time_ms() - cpu->start_time;
    
    printf("\n=== CPU Statistics ===\n");
    printf("Cycles: %lu\n", cpu->cycles);
    printf("Instructions: %lu\n", cpu->instructions);
    printf("CPI: %.2f\n", (float)cpu->cycles / cpu->instructions);
    printf("Stalls: %lu\n", cpu->stalls);
    printf("Pipeline bubbles: %lu\n", cpu->bubbles);
    printf("Simulation time: %lu ms\n", elapsed);
    
    cache_print_stats(cpu->l1_cache);
    cache_print_stats(cpu->l2_cache);
    bp_print_stats(cpu->bp);
}

void cpu_print_registers(CPU* cpu) {
    printf("\n=== Registers ===\n");
    for (int i = 0; i < 16; i++) {
        printf("R%02d: 0x%016lx", i, cpu->registers[i]);
        if ((i + 1) % 4 == 0) printf("\n");
        else printf("\t");
    }
    printf("PC: 0x%016lx\n", cpu->pc);
    printf("SP: 0x%016lx\n", cpu->sp);
    printf("FLAGS: 0x%016lx\n", cpu->flags);
}

void cpu_print_pipeline(CPU* cpu) {
    const char* stage_names[] = {
        "FETCH", "DECODE", "EXECUTE", "MEMORY", "WRITEBACK", "COMMIT"
    };
    
    printf("\n=== Pipeline ===\n");
    for (int i = 0; i < 6; i++) {
        const char* inst_name = "NOP";
        if (cpu->pipeline[i].type != INST_NOP) {
            inst_name = "INST";
        }
        
        printf("%-10s: %s (PC: 0x%lx)", 
               stage_names[i], inst_name, cpu->pipeline[i].pc);
        
        if (cpu->pipeline[i].stall) printf(" [STALL]");
        if (cpu->pipeline[i].bubble) printf(" [BUBBLE]");
        printf("\n");
    }
}