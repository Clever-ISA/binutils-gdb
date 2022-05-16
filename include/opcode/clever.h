#ifndef _OPCODES_CLEVER_H_
#define _OPCODES_CLEVER_H_

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

    reserved255 = 255,
} clever_reg;

typedef enum {
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
} clever_sreg;

#define CLEVER_OPC(full_op) ((full_op)>>4)
#define CLEVER_OP_H(full_op) ((full_op)&0xf)

enum clever_operand_kind{
    direct = 0,
    indirect = 1,
    short_imm    = 2,
    long_imm     = 3,
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



#endif 