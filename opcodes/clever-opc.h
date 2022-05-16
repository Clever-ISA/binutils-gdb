#ifndef CLEVER_OPC_H
#define CLEVER_OPC_H

#include "opcode/clever.h"
#include <limits.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"{
#endif

// Extensions in Clever-ISA
enum CleverExtension{
    XMain, // All CPUs support X-main by definition
    XFloat,
    XVec,
    XFloatExt,
    XRand,
    XVirtualization,

    ExtMax = XVirtualization
};

#define ExtensionNumUints \
  (ExtMax / sizeof (unsigned int) / CHAR_BIT + 1)

#define ExtensionNumBits (ExtensionNumUints * sizeof(unsigned int) * CHAR_BIT)

#define ExtensionUnused (ExtMax + 1)

typedef union clever_extensions{
    struct{
        unsigned int x_main:1;
        unsigned int x_float:1;
        unsigned int x_vec:1;
        unsigned int x_vec:1;
        unsigned int x_rand:1;
        unsigned int x_virtualization:1;
        unsigned int unused_bits:(ExtensionNumBits-ExtensionUnused);
    };
    unsigned int flags[ExtensionNumUints];
};

enum{
    branch_addr = 16,
};

enum clever_unconditional_branches{
    jmp,
    call,
    fcall,
    ret,
    scall,
    intr,
    ijmp,
    icall,
    ifcall,
    jsm,
    callsm,
    retrsm,
    last_user_branch = retrsm,
    // supervisor branches
    first_super_branch = 32,
    scret = 32,
    reti,

    hcall = first_super_branch+11,
    last_supervisor_branch = hcall,
    hypervisor_branches = first_super_branch+16,
    hret = hypervisor_branches+16+6,
    hresume = first_super_branch+16+7,
    last_hyper_branch = hresume,
};

/// Given the entire opcode, obtains the unconditional branch operand
/// Returns 0 if no such 
typedef unsigned int(branch_dest_size)(uint16_t);
/// Given the entire opcode, obtains the indirect branch target register if any
/// returns 63 (reserved63) if none
typedef clever_reg(indirect_branch_register)(uint16_t);



enum{
    Vec = 0x200,
    Repbi = 0x028,
    Repbc = 0x029,
};

typedef bool(validate_control_field)(uint16_t, union clever_extensions*);
typedef char*(write_control_suffix)(uint16_t, char* c);

typedef struct{
    uint16_t size;
    union{char* symbol; uint64_t value_hi;};
    uint64_t value;
} clever_immediate;


typedef union clever_operand{
    struct{ enum clever_operand_kind kind; bool vec; uint16_t size; clever_reg reg;} direct;
    struct{ enum clever_operand_kind kind; uint8_t scale; clever_reg base; uint8_t index_kind_and_val;} indirect;
    struct{ enum clever_operand_kind kind; bool rel; clever_immedate imm;} short_imm;
    struct{ enum clever_operand_kind kind; bool rel; bool mem; clever_immediate im; uint16_t mem_size;} long_imm;
} clever_operand;

typedef size_t (insert_operands)(uint16_t, clever_operand*, size_t);

struct clever_instruction_info{
    uint16_t opcode;
    uint16_t valid_prefix;
    uint16_t operand_count;
    uint16_t clever_extension;
    const char* mnemonic;
    validate_control_field* validate_h;
    write_control_suffix* print_h;
    insert_operands* pre_insert_operands;
    insert_operands* post_insert_operands;
};




struct clever_branch_info{
    branch_dest_size* dest_size;
    indirect_branch_register* reg;
    bool allow_relative;
    const clever_instruction_info* insn;
};

extern const struct clever_branch_info ubranches[];
extern const struct clever_branch_info sbranches[];

extern const struct clever_branch_info cond_branch_info;

extern const struct clever_instruction_info insns[];

#define MAX_MNEMONIC_LENGTH 16

#ifdef __cplusplus
};
#endif

#endif