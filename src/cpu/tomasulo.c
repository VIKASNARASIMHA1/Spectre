#include "cpu.h"

// Reservation station
typedef struct {
    bool busy;
    InstructionType op;
    uint64_t vj, vk;
    uint64_t qj, qk;  // Source reservation stations
    uint64_t dest;
    uint64_t address;
    uint64_t result;
    bool result_ready;
} ReservationStation;

// Reorder buffer entry
typedef struct {
    bool busy;
    InstructionType op;
    uint64_t result;
    uint64_t dest;
    bool ready;
    bool exception;
} ROBEntry;

// Tomasulo CPU implementation
typedef struct {
    ReservationStation* rs;  // Reservation stations
    ROBEntry* rob;           // Reorder buffer
    uint64_t* registers;     // Register file
    uint64_t* reg_status;    // Which RS produces register value
    
    int rs_count;
    int rob_size;
    
    int rs_head;
    int rs_tail;
    int rob_head;
    int rob_tail;
    
    uint64_t clock;
    uint64_t instructions_issued;
    uint64_t instructions_completed;
    uint64_t instructions_committed;
} TomasuloCPU;

TomasuloCPU* tomasulo_create(int rs_count, int rob_size) {
    TomasuloCPU* cpu = (TomasuloCPU*)malloc(sizeof(TomasuloCPU));
    if (!cpu) return NULL;
    
    cpu->rs_count = rs_count;
    cpu->rob_size = rob_size;
    
    cpu->rs = (ReservationStation*)calloc(rs_count, sizeof(ReservationStation));
    cpu->rob = (ROBEntry*)calloc(rob_size, sizeof(ROBEntry));
    cpu->registers = (uint64_t*)calloc(32, sizeof(uint64_t));
    cpu->reg_status = (uint64_t*)calloc(32, sizeof(uint64_t));
    
    if (!cpu->rs || !cpu->rob || !cpu->registers || !cpu->reg_status) {
        free(cpu->rs);
        free(cpu->rob);
        free(cpu->registers);
        free(cpu->reg_status);
        free(cpu);
        return NULL;
    }
    
    cpu->rs_head = 0;
    cpu->rs_tail = 0;
    cpu->rob_head = 0;
    cpu->rob_tail = 0;
    cpu->clock = 0;
    
    cpu->instructions_issued = 0;
    cpu->instructions_completed = 0;
    cpu->instructions_committed = 0;
    
    return cpu;
}

void tomasulo_destroy(TomasuloCPU* cpu) {
    if (cpu) {
        free(cpu->rs);
        free(cpu->rob);
        free(cpu->registers);
        free(cpu->reg_status);
        free(cpu);
    }
}

// Issue instruction to reservation station
bool tomasulo_issue(TomasuloCPU* cpu, DecodedInstruction inst) {
    // Find free reservation station
    for (int i = 0; i < cpu->rs_count; i++) {
        if (!cpu->rs[i].busy) {
            cpu->rs[i].busy = true;
            cpu->rs[i].op = inst.type;
            cpu->rs[i].dest = inst.rd;
            
            // Check if source operands are ready
            if (cpu->reg_status[inst.rs1] == 0) {
                cpu->rs[i].vj = cpu->registers[inst.rs1];
                cpu->rs[i].qj = 0;
            } else {
                cpu->rs[i].qj = cpu->reg_status[inst.rs1];
            }
            
            if (cpu->reg_status[inst.rs2] == 0) {
                cpu->rs[i].vk = cpu->registers[inst.rs2];
                cpu->rs[i].qk = 0;
            } else {
                cpu->rs[i].qk = cpu->reg_status[inst.rs2];
            }
            
            // Allocate ROB entry
            int rob_idx = cpu->rob_tail;
            cpu->rob[rob_idx].busy = true;
            cpu->rob[rob_idx].op = inst.type;
            cpu->rob[rob_idx].dest = inst.rd;
            cpu->rob[rob_idx].ready = false;
            cpu->rob[rob_idx].exception = false;
            
            // Update register status
            cpu->reg_status[inst.rd] = rob_idx + 1;  // +1 because 0 means no producer
            
            cpu->rob_tail = (cpu->rob_tail + 1) % cpu->rob_size;
            cpu->instructions_issued++;
            
            return true;
        }
    }
    return false;  // No free reservation station
}

// Execute instructions (simplified)
void tomasulo_execute(TomasuloCPU* cpu) {
    for (int i = 0; i < cpu->rs_count; i++) {
        if (cpu->rs[i].busy && cpu->rs[i].qj == 0 && cpu->rs[i].qk == 0) {
            // Operands ready, execute
            switch (cpu->rs[i].op) {
                case INST_ADD:
                    cpu->rs[i].result = cpu->rs[i].vj + cpu->rs[i].vk;
                    break;
                case INST_SUB:
                    cpu->rs[i].result = cpu->rs[i].vj - cpu->rs[i].vk;
                    break;
                case INST_MUL:
                    cpu->rs[i].result = cpu->rs[i].vj * cpu->rs[i].vk;
                    break;
                // ... other operations
                default:
                    cpu->rs[i].result = 0;
            }
            cpu->rs[i].result_ready = true;
            cpu->instructions_completed++;
        }
    }
}

// Write results back
void tomasulo_writeback(TomasuloCPU* cpu) {
    for (int i = 0; i < cpu->rs_count; i++) {
        if (cpu->rs[i].busy && cpu->rs[i].result_ready) {
            // Find ROB entry for this instruction
            for (int j = 0; j < cpu->rob_size; j++) {
                if (cpu->rob[j].busy && !cpu->rob[j].ready) {
                    cpu->rob[j].result = cpu->rs[i].result;
                    cpu->rob[j].ready = true;
                    break;
                }
            }
            
            // Free reservation station
            cpu->rs[i].busy = false;
            cpu->rs[i].result_ready = false;
        }
    }
}

// Commit instructions in order
void tomasulo_commit(TomasuloCPU* cpu) {
    while (cpu->rob[cpu->rob_head].busy && cpu->rob[cpu->rob_head].ready) {
        ROBEntry* entry = &cpu->rob[cpu->rob_head];
        
        if (!entry->exception) {
            // Write result to register file
            cpu->registers[entry->dest] = entry->result;
            cpu->reg_status[entry->dest] = 0;  // Register now has value
        }
        
        // Free ROB entry
        entry->busy = false;
        cpu->rob_head = (cpu->rob_head + 1) % cpu->rob_size;
        cpu->instructions_committed++;
    }
}

void tomasulo_print_stats(TomasuloCPU* cpu) {
    printf("\n=== Tomasulo Out-of-Order Statistics ===\n");
    printf("Clock cycles: %lu\n", cpu->clock);
    printf("Instructions issued: %lu\n", cpu->instructions_issued);
    printf("Instructions completed: %lu\n", cpu->instructions_completed);
    printf("Instructions committed: %lu\n", cpu->instructions_committed);
    printf("IPC: %.2f\n", (float)cpu->instructions_committed / cpu->clock);
    
    printf("\nReservation stations:\n");
    for (int i = 0; i < cpu->rs_count; i++) {
        if (cpu->rs[i].busy) {
            printf("  RS%d: busy, dest=r%lu\n", i, cpu->rs[i].dest);
        }
    }
    
    printf("\nReorder buffer:\n");
    for (int i = 0; i < cpu->rob_size; i++) {
        if (cpu->rob[i].busy) {
            printf("  ROB%d: %s, dest=r%lu, ready=%s\n",
                   i,
                   cpu->rob[i].exception ? "EXCEPTION" : "normal",
                   cpu->rob[i].dest,
                   cpu->rob[i].ready ? "yes" : "no");
        }
    }
}