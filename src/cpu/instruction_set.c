#include "cpu.h"

// Instruction formats
typedef enum {
    FORMAT_R,    // Register
    FORMAT_I,    // Immediate
    FORMAT_M,    // Memory
    FORMAT_J,    // Jump
    FORMAT_S     // System
} InstructionFormat;

// Instruction structure
typedef struct {
    InstructionType type;
    InstructionFormat format;
    uint8_t opcode;
    uint8_t rd;      // Destination register
    uint8_t rs1;     // Source register 1
    uint8_t rs2;     // Source register 2
    uint64_t imm;    // Immediate value
    uint64_t address;
} DecodedInstruction;

// Instruction table
static const struct {
    uint8_t opcode;
    InstructionType type;
    InstructionFormat format;
    const char* name;
} instruction_table[] = {
    {0x00, INST_NOP,   FORMAT_R, "nop"},
    {0x01, INST_ADD,   FORMAT_R, "add"},
    {0x02, INST_SUB,   FORMAT_R, "sub"},
    {0x03, INST_MUL,   FORMAT_R, "mul"},
    {0x04, INST_DIV,   FORMAT_R, "div"},
    {0x05, INST_AND,   FORMAT_R, "and"},
    {0x06, INST_OR,    FORMAT_R, "or"},
    {0x07, INST_XOR,   FORMAT_R, "xor"},
    {0x08, INST_NOT,   FORMAT_R, "not"},
    {0x09, INST_SHL,   FORMAT_R, "shl"},
    {0x0A, INST_SHR,   FORMAT_R, "shr"},
    {0x0B, INST_LD,    FORMAT_M, "ld"},
    {0x0C, INST_ST,    FORMAT_M, "st"},
    {0x0D, INST_JMP,   FORMAT_J, "jmp"},
    {0x0E, INST_JZ,    FORMAT_J, "jz"},
    {0x0F, INST_JNZ,   FORMAT_J, "jnz"},
    {0x10, INST_CALL,  FORMAT_J, "call"},
    {0x11, INST_RET,   FORMAT_J, "ret"},
    {0x12, INST_CMP,   FORMAT_R, "cmp"},
    {0x13, INST_MOV,   FORMAT_R, "mov"},
    {0x14, INST_HLT,   FORMAT_S, "hlt"},
};

// Decode instruction from memory
DecodedInstruction decode_instruction(uint8_t* mem, uint64_t pc) {
    DecodedInstruction inst = {0};
    uint8_t* ptr = mem + pc;
    
    // Simple decoding (in real x86-64, this is much more complex)
    inst.opcode = ptr[0];
    inst.type = INST_NOP;
    
    // Look up in instruction table
    for (size_t i = 0; i < ARRAY_SIZE(instruction_table); i++) {
        if (instruction_table[i].opcode == inst.opcode) {
            inst.type = instruction_table[i].type;
            inst.format = instruction_table[i].format;
            break;
        }
    }
    
    // Decode registers and immediate (simplified)
    if (inst.format == FORMAT_R || inst.format == FORMAT_M) {
        inst.rd = (ptr[1] >> 4) & 0x0F;
        inst.rs1 = ptr[1] & 0x0F;
        inst.rs2 = (ptr[2] >> 4) & 0x0F;
        
        if (inst.format == FORMAT_M) {
            // Memory instructions have address
            inst.address = *(uint64_t*)(ptr + 3);
        }
    } else if (inst.format == FORMAT_I || inst.format == FORMAT_J) {
        inst.rd = (ptr[1] >> 4) & 0x0F;
        inst.imm = *(uint64_t*)(ptr + 2);
    }
    
    return inst;
}

// Encode instruction to memory
uint32_t encode_instruction(DecodedInstruction inst, uint8_t* buffer) {
    uint32_t size = 1;  // Start with opcode
    
    // Find opcode
    for (size_t i = 0; i < ARRAY_SIZE(instruction_table); i++) {
        if (instruction_table[i].type == inst.type) {
            buffer[0] = instruction_table[i].opcode;
            break;
        }
    }
    
    // Encode based on format
    switch (inst.format) {
        case FORMAT_R:
        case FORMAT_M:
            buffer[1] = (inst.rd << 4) | (inst.rs1 & 0x0F);
            buffer[2] = (inst.rs2 << 4);
            size = 3;
            
            if (inst.format == FORMAT_M) {
                *(uint64_t*)(buffer + 3) = inst.address;
                size += 8;
            }
            break;
            
        case FORMAT_I:
        case FORMAT_J:
            buffer[1] = (inst.rd << 4);
            *(uint64_t*)(buffer + 2) = inst.imm;
            size = 10;
            break;
            
        case FORMAT_S:
            // System instructions are just opcode
            size = 1;
            break;
    }
    
    return size;
}

// Disassemble instruction to string
const char* disassemble_instruction(uint8_t* mem, uint64_t pc) {
    static char buffer[256];
    DecodedInstruction inst = decode_instruction(mem, pc);
    
    // Find instruction name
    const char* name = "unknown";
    for (size_t i = 0; i < ARRAY_SIZE(instruction_table); i++) {
        if (instruction_table[i].type == inst.type) {
            name = instruction_table[i].name;
            break;
        }
    }
    
    // Format based on instruction type
    switch (inst.format) {
        case FORMAT_R:
            snprintf(buffer, sizeof(buffer), "%s r%d, r%d, r%d",
                    name, inst.rd, inst.rs1, inst.rs2);
            break;
            
        case FORMAT_I:
            snprintf(buffer, sizeof(buffer), "%s r%d, %lu",
                    name, inst.rd, inst.imm);
            break;
            
        case FORMAT_M:
            if (inst.type == INST_LD) {
                snprintf(buffer, sizeof(buffer), "%s r%d, [%lu]",
                        name, inst.rd, inst.address);
            } else {
                snprintf(buffer, sizeof(buffer), "%s [%lu], r%d",
                        name, inst.address, inst.rd);
            }
            break;
            
        case FORMAT_J:
            snprintf(buffer, sizeof(buffer), "%s 0x%lx",
                    name, inst.imm);
            break;
            
        case FORMAT_S:
            snprintf(buffer, sizeof(buffer), "%s", name);
            break;
    }
    
    return buffer;
}

// Get instruction size in bytes
uint32_t get_instruction_size(uint8_t* mem, uint64_t pc) {
    DecodedInstruction inst = decode_instruction(mem, pc);
    
    switch (inst.format) {
        case FORMAT_R: return 3;
        case FORMAT_M: return 11;  // 3 + 8 for address
        case FORMAT_I: return 10;  // 2 + 8 for immediate
        case FORMAT_J: return 10;  // 2 + 8 for address
        case FORMAT_S: return 1;
        default: return 1;
    }
}