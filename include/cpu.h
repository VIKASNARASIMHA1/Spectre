#ifndef CPU_H
#define CPU_H

#include "common.h"

// Instruction definitions
typedef enum {
    INST_NOP,
    INST_ADD,
    INST_SUB,
    INST_MUL,
    INST_DIV,
    INST_AND,
    INST_OR,
    INST_XOR,
    INST_NOT,
    INST_SHL,
    INST_SHR,
    INST_LD,
    INST_ST,
    INST_JMP,
    INST_JZ,
    INST_JNZ,
    INST_CALL,
    INST_RET,
    INST_CMP,
    INST_MOV,
    INST_HLT
} InstructionType;

// Pipeline stages
typedef enum {
    STAGE_FETCH,
    STAGE_DECODE,
    STAGE_EXECUTE,
    STAGE_MEMORY,
    STAGE_WRITEBACK,
    STAGE_COMMIT
} PipelineStage;

// Cache types
typedef enum {
    CACHE_DIRECT_MAPPED,
    CACHE_SET_ASSOC,
    CACHE_FULL_ASSOC
} CacheType;

// Cache structure
typedef struct {
    CacheType type;
    int size;           // in bytes
    int line_size;
    int associativity;
    int num_sets;
    int hit_time;       // cycles
    int miss_penalty;   // cycles
    
    uint64_t** tags;
    bool** valid;
    uint64_t* lru_counters;
    
    uint64_t hits;
    uint64_t misses;
    uint64_t accesses;
} Cache;

// Branch predictor types
typedef enum {
    PREDICTOR_ALWAYS_TAKEN,
    PREDICTOR_ALWAYS_NOT_TAKEN,
    PREDICTOR_BIMODAL,
    PREDICTOR_GSHARE
} PredictorType;

// Branch predictor
typedef struct {
    PredictorType type;
    int bhr_size;           // Branch history register size
    int pht_size;           // Pattern history table size
    uint32_t bhr;           // Branch history register
    uint8_t* pht;           // Pattern history table
    uint64_t correct;
    uint64_t total;
} BranchPredictor;

// Pipeline register
typedef struct {
    uint64_t pc;
    InstructionType type;
    uint8_t opcode;
    uint64_t src1;
    uint64_t src2;
    uint64_t dest;
    uint64_t immediate;
    uint64_t result;
    uint64_t mem_addr;
    uint64_t mem_data;
    bool stall;
    bool bubble;
    uint64_t cycle_entered;
} PipelineRegister;

// CPU core
typedef struct {
    // Registers
    uint64_t registers[16];     // R0-R15
    uint64_t pc;                // Program counter
    uint64_t sp;                // Stack pointer
    uint64_t flags;             // Status flags
    
    // Memory
    uint8_t* memory;
    uint64_t mem_size;
    
    // Caches
    Cache* l1_cache;
    Cache* l2_cache;
    
    // Branch predictor
    BranchPredictor* bp;
    
    // Pipeline
    PipelineRegister pipeline[6];  // One for each stage
    PipelineStage current_stage;
    
    // Performance counters
    uint64_t cycles;
    uint64_t instructions;
    uint64_t stalls;
    uint64_t bubbles;
    
    // Statistics
    uint64_t start_time;
} CPU;

// Function prototypes
CPU* cpu_create(uint64_t mem_size);
void cpu_destroy(CPU* cpu);
void cpu_reset(CPU* cpu);
int cpu_load_program(CPU* cpu, uint8_t* program, uint64_t size, uint64_t address);
void cpu_step(CPU* cpu);
void cpu_run(CPU* cpu, uint64_t cycles);
void cpu_print_stats(CPU* cpu);
void cpu_print_registers(CPU* cpu);
void cpu_print_pipeline(CPU* cpu);

// Cache functions
Cache* cache_create(CacheType type, int size, int line_size, int associativity);
void cache_destroy(Cache* cache);
int cache_access(Cache* cache, uint64_t addr, bool is_write);
void cache_print_stats(Cache* cache);

// Branch predictor functions
BranchPredictor* bp_create(PredictorType type, int bhr_size, int pht_size);
void bp_destroy(BranchPredictor* bp);
bool bp_predict(BranchPredictor* bp, uint64_t pc);
void bp_update(BranchPredictor* bp, uint64_t pc, bool taken, bool predicted);
void bp_print_stats(BranchPredictor* bp);

#endif