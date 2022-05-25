#ifndef _OPCODES_CLEVER_H_
#define _OPCODES_CLEVER_H_

#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

typedef enum{
    r0,
    r1,
    r2,
    r3,
    r4,
    r5,
    r6,
    r7,
    r8,
    r9,
    r10,
    r11,
    r12,
    r13,
    r14,
    r15,
    ip,
    flags,
    mode,
    fpcw,
    /* no registers between 19 and 24 exclusive */
    f0 = 24,
    f1, 
    f2,
    f3,
    f4,
    f5,
    f6,
    f7,

    reserved63 = 63,
    /* vector registers start at 64-127 */
    v0l = 64,
    v0h,
    v1l,
    v1h,
    v2l,
    v2h,
    v3l,
    v3h,
    v4l,
    v4h,
    v5l,
    v5h,
    v6l,
    v6h,
    v7l,
    v7h,
    v8l,
    v8h,
    v9l,
    v9h,
    v10l,
    v10h,
    v11l,
    v11h,
    v12l,
    v12h,
    v13l,
    v13h,
    v14l,
    v14h,
    v15l,
    v15h,
    cr0 = 128,
    cr1,
    cr2, 
    cr3, 
    cr4,
    cr5,
    cr6,
    cr7,
    cpuidlo,
    cpuidhi,
    cpuex2,
    cpuex3,
    cpuex4,
    cpuex5,
    cpuex6,
    mscpuex,

    msr0 = 148,
    msr1,
    msr2,
    msr3,
    msr4,
    msr5,
    msr6,
    randinfo = 156,
    reserved255 = 255,
} clever_reg;


#define CLEVER_OPC(full_op) ((full_op)>>4)
#define CLEVER_OP_H(full_op) ((full_op)&0xf)

enum clever_operand_kind{
    direct = 0,
    indirect = 1,
    short_imm    = 2,
    long_imm     = 3,
    size_only    = 4,
};

enum clever_condition{
    parity = 0,
    carry = 1,
    overflow = 2,
    zero = 3,
    less_than = 4,
    less_equal = 5,
    below_equal = 6,
    minus = 7,
    plus = 8,
    above = 9,
    greater_than = 10,
    greater_equal = 11,
    nonzero = 12,
    no_overflow = 13,
    no_carry = 14,
    no_parity = 15
};

// extract the kind of a clever operand
#define CLEVER_OPR_KIND(operand) ((operand)>>14)

#define CLEVER_SS_TO_SIZE(ss) (1<<(ss))

// Bitfield extractions from register operand
#define CLEVER_REG_IS_VECTOR(operand) (((operand)>>13)&0x1)
#define CLEVER_REG_OPERAND(operand) ((operand)&0xff)
#define CLEVER_REG_SIZE_SS(operand) (((operand)>>8)&0x7)


// Bitfield extractions from indirect
#define CLEVER_INDIRECT_OFFSET(operand) (((operand)>>10)&0xf)
#define CLEVER_INDIRECT_BASE(operand) ((operand)&0xf)
#define CLEVER_INDIRECT_SCALE(operand) (((operand)>>7)&0x7)
#define CLEVER_INDIRECT_IS_STATIC_OFF(operand) (((operand)>>6)&0x1)
#define CLEVER_INDIRECT_SIZE_SS(operand) (((operand)>>4)&0x3)

// Bitfield extractions from short immediate
#define CLEVER_SHORT_VAL(operand) ((operand)&0xfff)
#define CLEVER_SHORT_REL(operand) (((operand)>>12)&0x1)

// Bitfield extractions from long immediate
#define CLEVER_LONG_IMM_SIZE(operand) ((((operand)>>8)&0x3)+1)
#define CLEVER_LONG_MEM_SIZE(operand) (((operand)>>4)&0xf)
#define CLEVER_LONG_IMM_REL(operand) (((operand)>>10)&0x1)
#define CLEVER_LONG_IS_MEM(operand) (((operand)>>13)&0x1)


// Bitfield extractions from branch instructions
#define CLEVER_IS_BRANCH(opcode) (((opcode)&0x720)==0x700)
#define CLEVER_IS_SUPER_BRANCH(opcode) (((opcode)&0xfe0)==0xfc0)
#define CLEVER_BRANCH_IS_CONDITIONAL(opcode) ((((opcode)>>6)&0x3)!=0x3)
#define CLEVER_BRANCH_IS_RELATIVE(opcode) (((opcode)>>4)&0x1)
#define CLEVER_BRANCH_CONDITIONAL_CC(opcode) ((opcode)&0xf)
#define CLEVER_BRANCH_CONDITIONAL_SIZE(opcode) ((((opcode)>>6)&0x3)+1)
#define CLEVER_BRANCH_UNCONDITIONAL_OP(opcode) ((opcode)&0xf)

#define CLEVER_BRANCH_UNCONDITIONAL_SIZE(opcode) ((CLEVER_OP_H(opcode)&0x3)+1)

#define CLEVER_MAX_OPERANDS 3

// Extensions in Clever-ISA
enum clever_extension{
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

#define CLEVER_ALL_EXTENSIONS {.x_main = 1, .x_float = 1, .x_vec = 1, .x_float_ext = 1, .x_rand = 1, .x_virtualization = 1}

// X-rand and X-virtualization are not part of the prepared 1.0 spec
#define CLEVER1_0_ALL_EXTENSIONS {.x_main = 1, .x_float = 1, .x_vec = 1, .x_float_ext = 1}

// X-main is mandatory, but other bits are unset.
#define CLEVER_NO_EXTENSIONS {.x_main = 1}

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
    user_branch_count,
    // supervisor branches
    first_super_branch = 32,
    scret = 32,
    reti,
    last_basic_super_branch = reti,

    next_super_branch = first_super_branch+11,
    hcall = first_super_branch+11,
    last_super_branch = hcall,
    hyper_branches = first_super_branch+16,
    first_hyper_branch = hyper_branches+6,
    hret = hyper_branches+6,
    hresume = hyper_branches+7,
    last_hyper_branch = hresume,
};

#define BASIC_SUPER_BRANCH_COUNT ((last_basic_super_branch+1)-first_super_branch)
#define VIRT_SUPER_BRANCH_COUNT ((last_super_branch+1)-next_super_branch)
#define SUPER_BRANCH_COUNT (BASIC_SUPER_BRANCH_COUNT+VIRT_SUPER_BRANCH_COUNT)
#define HYPER_BRANCH_COUNT ((last_hyper_branch+1)-first_hyper_branch)
#define SBRANCHES_SIZE (BASIC_SUPER_BRANCH_COUNT+VIRT_SUPER_BRANCH_COUNT+HYPER_BRANCH_COUNT)


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
    struct {enum clever_operand_kind kind; uint16_t size;} size_only;
} clever_operand;

struct clever_instruction;

typedef size_t clever_insert_operands(uint16_t, clever_operand*, size_t);

typedef bool clever_validate_operands(uint16_t, clever_operand*);

typedef const char* clever_parse_control_suffix(uint16_t*, const char*, const struct clever_instruction* insn);

struct clever_instruction_info{
    uint16_t opcode;
    uint16_t valid_prefix;
    uint16_t operand_count;
    uint16_t clever_extension;
    const char* mnemonic;
    clever_validate_control_field* validate_h;
    clever_write_control_suffix* print_h;
    clever_parse_control_suffix* parse_h;
    clever_insert_operands* pre_insert_operands;
    clever_insert_operands* post_insert_operands;
    clever_validate_operands* validate_operands;
    const struct clever_instruction_info* gpr_dest_spec;
    const struct clever_instruction_info* gpr_src_spec;
};


struct clever_branch_info{
    clever_branch_dest_size* dest_size;
    clever_indirect_branch_register* reg;
    bool allow_relative;
    const struct clever_instruction_info* insn;
};



struct clever_instruction{
    uint16_t prefix_opc;
    uint16_t opc;
    const struct clever_instruction_info* prefix_insn;
    const struct clever_instruction_info* insn;
    union clever_operand ops[CLEVER_MAX_OPERANDS];
    size_t num_ops;
};

#define MAX_MNEMONIC_LENGTH 24


#endif 