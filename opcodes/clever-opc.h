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

union clever_extensions{
    struct{
        unsigned int x_main:1;
        unsigned int x_float:1;
        unsigned int x_vec:1;
        unsigned int x_float_ext:1;
        unsigned int x_rand:1;
        unsigned int x_virtualization:1;
        unsigned int unused_bits:(ExtensionNumBits-ExtensionUnused);
    };
    unsigned int flags[ExtensionNumUints];
};

#define CLEVER_HAS_EXTENSION(exts, xno) ((((exts)->flags)[(xno)/(sizeof (unsigned int)*CHAR_BIT)])&(0<<((xno)%(sizeof (unsigned int)*CHAR_BIT))))

enum{
    branch_addr = 16,
    prefix,
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
    first_hyper_branch = first_super_branch+16,
    hret = first_hyper_branch+6,
    hresume = first_hyper_branch+7,
    last_hyper_branch = hresume,
};

/// Given the entire opcode, obtains the unconditional branch operand
/// Returns 0 if no such 
typedef unsigned int(clever_branch_dest_size)(uint16_t);
/// Given the entire opcode, obtains the indirect branch target register if any
/// returns 63 (reserved63) if none
typedef clever_reg(clever_indirect_branch_register)(uint16_t);



enum{
    NullPrefix = 0x000, // not an actual prefix, but shares an opcode with und
    Vec = 0x200,
    Repbi = 0x028,
    Repbc = 0x029,
};

typedef bool(clever_validate_control_field)(uint16_t, union clever_extensions*);
typedef char*(clever_write_control_suffix)(uint16_t, char* c);

typedef struct{
    uint16_t size;
    union{char* symbol; uint64_t value_hi;};
    uint64_t value;
} clever_immediate;


typedef union clever_operand{
    struct{ enum clever_operand_kind kind; bool vec; uint16_t size; clever_reg reg;} direct;
    struct{ enum clever_operand_kind kind; uint16_t size; uint8_t scale; clever_reg base; uint8_t index_kind_and_val;} indirect;
    struct{ enum clever_operand_kind kind; bool rel; clever_immediate imm;} short_imm;
    struct{ enum clever_operand_kind kind; bool rel; bool mem; clever_immediate imm; uint16_t mem_size;} long_imm;
} clever_operand;


typedef size_t clever_insert_operands(uint16_t, clever_operand*, size_t);

typedef bool clever_validate_operands(uint16_t, clever_operand*);

struct clever_instruction_info{
    uint16_t opcode;
    uint16_t valid_prefix;
    uint16_t operand_count;
    uint16_t clever_extension;
    const char* mnemonic;
    clever_validate_control_field* validate_h;
    clever_write_control_suffix* print_h;
    clever_insert_operands* pre_insert_operands;
    clever_insert_operands* post_insert_operands;
    clever_validate_operands* validate_operands;
};




struct clever_branch_info{
    clever_branch_dest_size* dest_size;
    clever_indirect_branch_register* reg;
    bool allow_relative;
    const struct clever_instruction_info* insn;
};

extern const struct clever_branch_info clever_ubranches[];
extern const struct clever_branch_info clever_sbranches[];

extern const struct clever_branch_info clever_cond_branch_info;

extern const struct clever_instruction_info clever_insns[];

struct clever_instruction{
    uint16_t prefix_opc;
    uint16_t opc;
    const struct clever_instruction_info* prefix_insn;
    const struct clever_instruction_info* insn;
    union clever_operand ops[CLEVER_MAX_OPERANDS];
    size_t num_ops;
};

#define MAX_MNEMONIC_LENGTH 24

#ifdef __cplusplus
};
#endif

#endif