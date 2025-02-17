#include "clever-opc.h"

#include <string.h>
#include <stdlib.h>
#include <ctype.h>

static unsigned int extract_unconditional_branch_size(uint16_t s){
    return CLEVER_SS_TO_SIZE(CLEVER_BRANCH_UNCONDITIONAL_SIZE(s));
}

static unsigned int no_addr_operand(uint16_t s __attribute_maybe_unused__){
    return 0;
}

static bool validate_allow_ss(uint16_t s, union clever_extensions* cext __attribute_maybe_unused__){
    return (s&0xf)==(s&3);
}

static bool validate_allow_none(uint16_t s __attribute_maybe_unused__, union clever_extensions* cext __attribute_maybe_unused__){
    return false;
}

static bool validate_allow_any(uint16_t s __attribute_maybe_unused__, union clever_extensions* cext __attribute_maybe_unused__){
    return true;
}

static bool validate_callsm(uint16_t s, union clever_extensions* cext __attribute_maybe_unused__){
    return (s&0x7)==(s&3);
}

static char* noop_print_h(uint16_t s __attribute_maybe_unused__,  char* p){
    return p;
}

static clever_reg extract_indirect_branch_dest(uint16_t s){
    return CLEVER_OP_H(s);
}

static clever_reg no_indirect_branch(uint16_t s __attribute_maybe_unused__){
    return reserved63;
}

static char* print_ijmp_target(uint16_t s, char* p){
    if(CLEVER_OP_H(s)==14){
        p-=4;
        strcpy(p,"fret");
        return p+4;
    }else if(CLEVER_OP_H(s)==15){
        p-=4;
        strcpy(p,"ifjmp");
        return p+5;
    }else{
        return p;
    }
}

static const char* parse_ijmp_target(uint16_t* s, const char* p, const struct clever_instruction* insn){
    *s |= insn->ops[0].direct.reg;
    return p;
}

static char* print_vfield(uint16_t s, char* p){
    if(CLEVER_OP_H(s)==1){
        strcpy(p,".v");
        return p+2;
    }else{
        return p;
    }
}

static size_t add_none(uint16_t s __attribute_maybe_unused__, clever_operand* ops __attribute_maybe_unused__, size_t curr_size){
    return curr_size;
}

static const char* parse_none(uint16_t* s __attribute_maybe_unused__, const char* p, const struct clever_instruction* insn __attribute_maybe_unused__){
    return p;
}

static const char* parse_jmp_dest(uint16_t* s, const char* p, const struct clever_instruction* insn){
    size_t size = insn->ops[0].long_imm.imm.size;
    size_t size_ss = (size==16?0:size==32?1:2);

    if(insn->ops[0].long_imm.rel)
        *s |= 0x100;
    *s |= size_ss;

    return p;
}

static size_t add_ijmp_target(uint16_t s, clever_operand* ops, size_t curr_size){
    if(CLEVER_OP_H(s)<14){
        ops[curr_size].direct.kind = direct;
        ops[curr_size].direct.size = 3;
        ops[curr_size].direct.vec = false;
        ops[curr_size].direct.reg = CLEVER_OP_H(s);
        curr_size+=1;
    }
    return curr_size;
}

static size_t add_reg_op(uint16_t s, clever_operand* ops, size_t curr_size){
    ops[curr_size].direct.kind = direct;
    ops[curr_size].direct.size = 3;
    ops[curr_size].direct.vec = false;
    ops[curr_size].direct.reg = CLEVER_OP_H(s);
    curr_size+=1;
    return curr_size;
}

static size_t add_int(uint16_t s, clever_operand* ops, size_t curr_size){
    ops[curr_size].short_imm.kind = short_imm;
    ops[curr_size].short_imm.rel = false;
    ops[curr_size].short_imm.imm.size = 0;
    ops[curr_size].short_imm.imm.symbol = NULL;
    ops[curr_size].short_imm.imm.value = CLEVER_OP_H(s);
    return curr_size+1;
}

static bool validate_any_ops(uint16_t c __attribute_maybe_unused__, clever_operand* ops __attribute_maybe_unused__){
    return true;
}


const struct clever_instruction_info branch_jmp   = {0x7c0, 0x000, branch_addr, XMain, "jmp"  , validate_allow_ss,           noop_print_h,    parse_jmp_dest, add_none, add_none, validate_any_ops, NULL, NULL};
const struct clever_instruction_info branch_call  = {0x7c1, 0x000, branch_addr, XMain, "call" , validate_allow_ss,           noop_print_h,    parse_jmp_dest, add_none, add_none, validate_any_ops, NULL, NULL};
const struct clever_instruction_info branch_fcall = {0x7c2, 0x000, branch_addr, XMain, "fcall", validate_allow_ss,           noop_print_h,    parse_jmp_dest, add_none, add_none, validate_any_ops, NULL, NULL};
const struct clever_instruction_info branch_ret   = {0x7c3, 0x000, 0,           XMain, "ret"  , validate_allow_none,         noop_print_h,        parse_none, add_none, add_none, validate_any_ops, NULL, NULL};
const struct clever_instruction_info branch_scall = {0x7c4, 0x000, 0,           XMain, "scall", validate_allow_none,         noop_print_h,        parse_none, add_none, add_none, validate_any_ops, NULL, NULL};
const struct clever_instruction_info branch_int   = {0x7c5, 0x000, 0,           XMain, "int"  , validate_allow_any,          noop_print_h,        parse_none, add_int, add_none, validate_any_ops, NULL, NULL};
const struct clever_instruction_info branch_ijmp  = {0x7c6, 0x000, 0,           XMain, "ijmp" , validate_allow_any,     print_ijmp_target, parse_ijmp_target, add_ijmp_target,add_none, validate_any_ops, NULL, NULL};
const struct clever_instruction_info branch_icall = {0x7c7, 0x000, 0,           XMain, "icall", validate_allow_any,          noop_print_h, parse_ijmp_target, add_reg_op,add_none, validate_any_ops, NULL, NULL};
const struct clever_instruction_info branch_ifcall= {0x7c8, 0x000, 0,           XMain,"ifcall", validate_allow_none,         noop_print_h, parse_ijmp_target, add_reg_op,add_none, validate_any_ops, NULL, NULL};
const struct clever_instruction_info branch_jsm   = {0x7c9, 0x000, branch_addr, XMain, "jsm"  , validate_allow_ss,           noop_print_h,    parse_jmp_dest, add_none,  add_none, validate_any_ops, NULL, NULL};
const struct clever_instruction_info branch_callsm= {0x7ca, 0x000, branch_addr, XMain,"callsm", validate_callsm,             print_vfield,    parse_jmp_dest, add_none,  add_none, validate_any_ops, NULL, NULL};
const struct clever_instruction_info branch_retrsm= {0x7cb, 0x000, 0,           XMain,"retrsm", validate_allow_none,         noop_print_h,        parse_none, add_none,  add_none, validate_any_ops, NULL, NULL};

const struct clever_instruction_info branch_scret  = {0xfc6, 0x000, 0,  XMain, "scret", validate_allow_none, noop_print_h, parse_none, add_none,add_none, validate_any_ops, NULL, NULL};
const struct clever_instruction_info branch_iret   = {0xfc7, 0x000, 0,  XMain, "iret" , validate_allow_none, noop_print_h, parse_none, add_none,add_none, validate_any_ops, NULL, NULL};

const struct clever_instruction_info branch_hcall  = {0xfcb, 0x000, 0, XVirtualization, "hcall"  , validate_allow_none, noop_print_h, parse_none, add_none, add_none, validate_any_ops, NULL, NULL};
const struct clever_instruction_info branch_hret   = {0xfd6, 0x000, 0, XVirtualization, "hret"   , validate_allow_none, noop_print_h, parse_none, add_none,add_none, validate_any_ops, NULL, NULL};
const struct clever_instruction_info branch_hresume= {0xfd7, 0x000, 0, XVirtualization, "hresume", validate_allow_none, noop_print_h, parse_none, add_none,add_none, validate_any_ops, NULL, NULL};


const struct clever_branch_info clever_ubranches[user_branch_count] = {
    {extract_unconditional_branch_size, no_indirect_branch, true, &branch_jmp},
    {extract_unconditional_branch_size, no_indirect_branch, true, &branch_call},
    {extract_unconditional_branch_size, no_indirect_branch, true, &branch_fcall},
    {no_addr_operand, no_indirect_branch, false, &branch_ret},
    {no_addr_operand, no_indirect_branch, false, &branch_scall},
    {no_addr_operand, no_indirect_branch, false, &branch_int},
    {no_addr_operand, extract_indirect_branch_dest, false, &branch_ijmp},
    {no_addr_operand,extract_indirect_branch_dest, false, &branch_icall},
    {no_addr_operand, extract_indirect_branch_dest, false, &branch_ifcall},
    {extract_unconditional_branch_size, no_indirect_branch, true, &branch_jsm},
    {extract_unconditional_branch_size, no_indirect_branch, true, &branch_callsm},
    {no_addr_operand, no_indirect_branch, false, &branch_retrsm}
};

const struct clever_branch_info clever_sbranches[SBRANCHES_SIZE] = {
    {no_addr_operand, no_indirect_branch, false, &branch_scret},
    {no_addr_operand, no_indirect_branch, false, &branch_iret},

    {no_addr_operand, no_indirect_branch, false, &branch_hcall},
    /* hypervisor branches */
    {no_addr_operand, no_indirect_branch, false, &branch_hret},
    {no_addr_operand, no_indirect_branch, false, &branch_hresume}
};

/*

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
*/

static const char* const ccsuffix[] = {
    "p", "c", "v", "z", "lt", "le", "be", "mi",
    "pl", "a", "gt", "ge", "nz", "nv", "nc", "np"
};

static char* add_jump_suffix(uint16_t opc, char* op){
    strcpy(op,ccsuffix[CLEVER_OPC(opc)&0xf]);
    op += strlen(ccsuffix[CLEVER_OPC(opc)&0xf]);
    
    int8_t val = CLEVER_OP_H(opc);

    val = (val<<4)>>4; // sign extend

    if(val!=0){
        *op++ = '.';
        if(val<0){
            *op++ = '-';
            *op++ = '0'-val;
        }else
            *op++ = '0'+val;
    }
    return op;
}


static const char* parse_cond_jump_suffix(uint16_t* opc, const char* op, const struct clever_instruction* insn){
    size_t size = insn->ops[0].long_imm.imm.size;
    size_t size_ss = (size==16?0:size==32?1:2);
    *opc |= size_ss<<10;
    for (const char* const* p = &16[ccsuffix];p-->ccsuffix;){
        if (strncmp(op,*p,strlen(*p))==0){
            op += strlen(*p);
            *opc |= (p-ccsuffix)<<4;
            break;
        }
    }

    if(*op=='.'){
        op++;
        int16_t val;
        if(*op=='-'){
            op++;
            val = -(*op++-'0');
        }else{
            val = (*op++-'0');
        }
        if(val<-8||val>7)
            return NULL;
        *opc |= val&0xf;
    }
    return op;
}


const struct clever_instruction_info branch_cond = {0x700, 0x000, 0, XMain, "j", validate_allow_any, add_jump_suffix, parse_cond_jump_suffix, add_none, add_none, validate_any_ops, NULL, NULL};

static unsigned int cond_branch_size(uint16_t s){
    return CLEVER_SS_TO_SIZE(CLEVER_BRANCH_CONDITIONAL_SIZE(CLEVER_OPC(s)));
}

const struct clever_branch_info clever_cond_branch_info = {cond_branch_size, no_indirect_branch, true, &branch_cond};

/*
struct clever_instruction_info{
    uint16_t opcode;
    uint16_t valid_prefix;
    uint16_t operand_count;
    uint16_t clever_extension;
    const char* mnemonic;
    validate_control_field* validate_h;
    write_control_suffix* print_h;
};
*/

static bool validate_arith_mem_h(uint16_t s, union clever_extensions* ext);

static char* add_arith_lf_h(uint16_t s, char* p);

static bool validate_mul_insn(uint16_t s, union clever_extensions* ext);

static char* add_mul_ssf(uint16_t s, char* p);
static char* add_div_sswf(uint16_t s, char* p);

static size_t add_size_spec(uint16_t s, union clever_operand* ops, size_t curr_size);

static size_t add_implied_gpr(uint16_t s, union clever_operand* ops, size_t curr_size);

static bool validate_arith_insn(uint16_t s, union clever_operand* ops);
static bool validate_unary_arith_insn(uint16_t s, union clever_operand* ops);
static bool validate_cmp_insn(uint16_t s, union clever_operand* ops);

static bool validate_mov(uint16_t s, union clever_operand* op);
static bool validate_lea(uint16_t s, union clever_operand* op);
static bool validate_pop(uint16_t s, union clever_operand* op);

static bool validate_needs_mem(uint16_t s, union clever_operand* op);

static bool validate_flags_only(uint16_t s, union clever_extensions* ext);

char* print_flags_h(uint16_t s, char* p);
char* print_fixed_h(uint16_t s, char* p);

static bool validate_float2int(uint16_t s, union clever_operand* op);
static bool validate_int2float(uint16_t s, union clever_operand* op);
static bool validate_float_unop(uint16_t s, union clever_operand* op);
static bool validate_float_binop(uint16_t s, union clever_operand* op);
static bool validate_float_top(uint16_t s, union clever_operand* op);
static bool validate_float_cmpz(uint16_t s, union clever_operand* op);
static bool validate_float_cmp(uint16_t s, union clever_operand* op);

char* print_cc_h(uint16_t s, char* p);

static size_t add_block_mem_refs(uint16_t s, union clever_operand* op, size_t curr_size);
static bool validate_block_mem_refs(uint16_t s, union clever_operand* op);

static bool validate_xhcg(uint16_t s, union clever_operand* op);
static bool validate_cmpxchg(uint16_t s, union clever_operand* op);


static bool validate_vmov(uint16_t s, union clever_operand* op);

// Make sure when adding new instruction holes, to update these constants

#define ARITH_GPR_DEST_SPECS (66)
#define ARITH_GPR_SRC_SPECS (74)
#define CMP_GPR_SPEC (ARITH_GPR_SRC_SPECS+8+2)

static const char* parse_arith_lf(uint16_t* opc, const char* op, const struct clever_instruction* insn);
static const char* parse_mul_ssf(uint16_t* opc, const char* op,const struct clever_instruction* insn);
static const char* parse_div_sswf(uint16_t* opc, const char* op, const struct clever_instruction* insn);
static const char* parse_flags(uint16_t* opc, const char* op, const struct clever_instruction* insn);
static const char* parse_fixed(uint16_t* opc, const char* op, const struct clever_instruction* insn);
static const char* parse_cc(uint16_t* opc, const char* op, const struct clever_instruction* insn);
static const char* parse_block_mem_refs(uint16_t* opc, const char* op, const struct clever_instruction* insn);

const struct clever_instruction_info clever_insns[] = {
    {0x000, 0x000, 0, XMain, "und",  validate_allow_any, noop_print_h, parse_none, add_none, add_none,validate_any_ops, NULL, NULL},
    {0x001, 0x400, 2, XMain, "add",  validate_arith_mem_h, add_arith_lf_h, parse_arith_lf, add_none, add_none, validate_arith_insn, &clever_insns[ARITH_GPR_DEST_SPECS+1],&clever_insns[ARITH_GPR_SRC_SPECS+1]},
    {0x002, 0x400, 2, XMain, "sub",  validate_arith_mem_h, add_arith_lf_h, parse_arith_lf, add_none, add_none, validate_arith_insn, &clever_insns[ARITH_GPR_DEST_SPECS+1],&clever_insns[ARITH_GPR_SRC_SPECS+1]},
    {0x003, 0x400, 2, XMain, "and",  validate_arith_mem_h, add_arith_lf_h, parse_arith_lf, add_none, add_none, validate_arith_insn, &clever_insns[ARITH_GPR_DEST_SPECS+1],&clever_insns[ARITH_GPR_SRC_SPECS+1]},
    {0x004, 0x400, 2, XMain, "or" ,  validate_arith_mem_h, add_arith_lf_h, parse_arith_lf, add_none, add_none, validate_arith_insn, &clever_insns[ARITH_GPR_DEST_SPECS+1],&clever_insns[ARITH_GPR_SRC_SPECS+1]},
    {0x005, 0x400, 2, XMain, "xor",  validate_arith_mem_h, add_arith_lf_h, parse_arith_lf, add_none, add_none, validate_arith_insn, &clever_insns[ARITH_GPR_DEST_SPECS+1],&clever_insns[ARITH_GPR_SRC_SPECS+1]},

    {0x006, 0x000, 0, XMain, "mul",  validate_mul_insn, add_mul_ssf, parse_mul_ssf, add_size_spec, add_none, validate_any_ops, NULL, NULL}, 
    {0x007, 0x000, 0, XMain, "div",  validate_allow_any, add_div_sswf, parse_div_sswf, add_size_spec, add_none, validate_any_ops, NULL, NULL},

    {0x008, 0x400, 2, XMain, "mov",  validate_allow_none, noop_print_h, parse_none, add_none, add_none, validate_mov, &clever_insns[10], &clever_insns[11]},
    {0x009, 0x000, 2, XMain, "lea",  validate_allow_none, noop_print_h, parse_none, add_none, add_none, validate_lea, &clever_insns[12], NULL},
    {0x00A, 0x400, 1, XMain, "mov",  validate_allow_any, noop_print_h, parse_none, add_implied_gpr, add_none, validate_mov, NULL, NULL},
    {0x00B, 0x400, 1, XMain, "mov",  validate_allow_any, noop_print_h, parse_none, add_none, add_implied_gpr, validate_mov, NULL, NULL},
    {0x00C, 0x000, 1, XMain, "lea",  validate_allow_any, noop_print_h, parse_none, add_implied_gpr, add_none, validate_lea, NULL, NULL},

    {0x010, 0x000, 0, XMain, "nop",  validate_allow_any, noop_print_h, parse_none, add_none, add_none, validate_any_ops, NULL, NULL},
    {0x011, 0x000, 1, XMain, "nop",  validate_allow_any, noop_print_h, parse_none, add_none, add_none, validate_any_ops, NULL, NULL},
    {0x012, 0x000, 2, XMain, "nop",  validate_allow_any, noop_print_h, parse_none, add_none, add_none, validate_any_ops, NULL, NULL},
    {0x013, 0x000, 3, XMain, "nop",  validate_allow_any, noop_print_h, parse_none, add_none, add_none, validate_any_ops, NULL, NULL},

    {0x014, 0x000, 1, XMain, "push",  validate_allow_none, noop_print_h, parse_none, add_none, add_none, validate_any_ops, &clever_insns[18], NULL},
    {0x015, 0x000, 1, XMain, "pop" ,  validate_allow_none, noop_print_h, parse_none, add_none, add_none, validate_pop, &clever_insns[19], NULL},
    {0x016, 0x000, 0, XMain, "push",  validate_allow_any, noop_print_h, parse_none, add_implied_gpr, add_none, validate_any_ops, NULL, NULL},
    {0x017, 0x000, 0, XMain, "pop" ,  validate_allow_any, noop_print_h, parse_none, add_implied_gpr, add_none, validate_pop, NULL, NULL},

    {0x018, 0x000, 1, XMain, "stogpr",  validate_allow_none, noop_print_h, parse_none, add_none, add_none, validate_needs_mem, NULL, NULL},
    {0x019, 0x000, 1, XMain, "stoar",  validate_allow_none, noop_print_h, parse_none, add_none, add_none, validate_needs_mem, NULL, NULL},
    {0x01A, 0x000, 1, XMain, "rstogpr",  validate_allow_none, noop_print_h, parse_none, add_none, add_none, validate_needs_mem, NULL, NULL},
    {0x01B, 0x000, 1, XMain, "rstoar",  validate_allow_none, noop_print_h, parse_none, add_none, add_none, validate_needs_mem, NULL, NULL},
    {0x01C, 0x000, 0, XMain, "pushgpr",  validate_allow_none, noop_print_h, parse_none, add_none, add_none, validate_any_ops, NULL, NULL},
    {0x01D, 0x000, 0, XMain, "pushar",  validate_allow_none, noop_print_h, parse_none, add_none, add_none, validate_any_ops, NULL, NULL},
    {0x01E, 0x000, 0, XMain, "popgpr",  validate_allow_none, noop_print_h, parse_none, add_none, add_none, validate_any_ops, NULL, NULL},
    {0x01F, 0x000, 0, XMain, "popar",  validate_allow_none, noop_print_h, parse_none, add_none, add_none, validate_any_ops, NULL, NULL},

    {0x020, 0x000, 0, XMain, "movsx", validate_flags_only, print_flags_h, parse_flags, add_none, add_none, validate_arith_insn, NULL, NULL},
    {0x021, 0x000, 0, XMain, "bswap", validate_flags_only, print_flags_h, parse_flags, add_none, add_none, validate_arith_insn, NULL, NULL},
    {0x022, 0x000, 0, XFloat, "movsif", validate_flags_only, print_flags_h, parse_flags, add_none, add_none, validate_int2float, NULL, NULL},
    {0x023, 0x000, 0, XFloat, "movxf", validate_allow_any, print_fixed_h, parse_fixed, add_none, add_none, validate_int2float, NULL, NULL},
    {0x024, 0x000, 0, XFloat, "movfsi", validate_flags_only, print_flags_h, parse_flags, add_none, add_none, validate_float2int, NULL, NULL},
    {0x025, 0x000, 0, XFloat, "movfx", validate_allow_any, print_fixed_h, parse_fixed, add_none, add_none, validate_float2int, NULL, NULL},
    {0x026, 0x000, 0, XFloat, "cvtf", validate_flags_only, print_flags_h, parse_flags, add_none, add_none, validate_float_binop, NULL, NULL},

    {0x028, 0x000, prefix, XMain, "repbi", validate_allow_none, noop_print_h,parse_none, add_none, add_none, validate_any_ops, NULL, NULL},
    {0x029, 0x000, prefix, XMain, "repbc", validate_allow_any, print_cc_h, parse_cc, add_none, add_none, validate_any_ops, NULL, NULL},
    {0x02A, 0x028, 0, XMain, "bcpy", validate_allow_ss, noop_print_h, parse_block_mem_refs, add_block_mem_refs, add_none, validate_block_mem_refs, NULL, NULL},
    {0x02B, 0x028, 0, XMain, "bsto", validate_allow_ss, noop_print_h, parse_block_mem_refs, add_block_mem_refs, add_none, validate_block_mem_refs, NULL, NULL},
    {0x02C, 0x028, 0, XMain, "bsca", validate_allow_ss, noop_print_h, parse_block_mem_refs, add_block_mem_refs, add_none, validate_block_mem_refs, NULL, NULL},
    {0x02D, 0x028, 0, XMain, "bcmp", validate_allow_ss, noop_print_h, parse_block_mem_refs, add_block_mem_refs, add_none, validate_block_mem_refs, NULL, NULL},
    {0x02E, 0x028, 0, XMain, "btst", validate_allow_ss, noop_print_h, parse_block_mem_refs, add_block_mem_refs, add_none, validate_block_mem_refs, NULL, NULL},

    {0x030, 0x400, 2, XMain, "lsh",  validate_arith_mem_h, add_arith_lf_h, parse_arith_lf, add_none, add_none, validate_arith_insn, &clever_insns[ARITH_GPR_DEST_SPECS-7], NULL},
    {0x031, 0x400, 2, XMain, "rsh",  validate_arith_mem_h, add_arith_lf_h, parse_arith_lf, add_none, add_none, validate_arith_insn, &clever_insns[ARITH_GPR_DEST_SPECS-6], NULL},
    {0x032, 0x400, 2, XMain, "arsh", validate_arith_mem_h, add_arith_lf_h, parse_arith_lf, add_none, add_none, validate_arith_insn, &clever_insns[ARITH_GPR_DEST_SPECS-5], NULL},
    {0x033, 0x400, 2, XMain, "lshc", validate_arith_mem_h, add_arith_lf_h, parse_arith_lf, add_none, add_none, validate_arith_insn, &clever_insns[ARITH_GPR_DEST_SPECS-4], NULL},
    {0x034, 0x400, 2, XMain, "rshc", validate_arith_mem_h, add_arith_lf_h, parse_arith_lf, add_none, add_none, validate_arith_insn, &clever_insns[ARITH_GPR_DEST_SPECS-3], NULL},
    {0x035, 0x400, 2, XMain, "lrot", validate_arith_mem_h, add_arith_lf_h, parse_arith_lf, add_none, add_none, validate_arith_insn, &clever_insns[ARITH_GPR_DEST_SPECS-2], NULL},
    {0x036, 0x400, 2, XMain, "rrot", validate_arith_mem_h, add_arith_lf_h, parse_arith_lf, add_none, add_none, validate_arith_insn, &clever_insns[ARITH_GPR_DEST_SPECS-1], NULL},
    {0x038, 0x400, 1, XMain, "lsh",  validate_allow_any, noop_print_h, parse_none, add_implied_gpr, add_none, validate_arith_insn, NULL, NULL},
    {0x039, 0x400, 1, XMain, "rsh",  validate_allow_any, noop_print_h, parse_none, add_implied_gpr, add_none, validate_arith_insn, NULL, NULL},
    {0x03A, 0x400, 1, XMain, "arsh", validate_allow_any, noop_print_h, parse_none, add_implied_gpr, add_none, validate_arith_insn, NULL, NULL},
    {0x03B, 0x400, 1, XMain, "lshc", validate_allow_any, noop_print_h, parse_none, add_implied_gpr, add_none, validate_arith_insn, NULL, NULL},
    {0x03C, 0x400, 1, XMain, "rshc", validate_allow_any, noop_print_h, parse_none, add_implied_gpr, add_none, validate_arith_insn, NULL, NULL},
    {0x03D, 0x400, 1, XMain, "lrot", validate_allow_any, noop_print_h, parse_none, add_implied_gpr, add_none, validate_arith_insn, NULL, NULL},
    {0x03E, 0x400, 1, XMain, "rrot", validate_allow_any, noop_print_h, parse_none, add_implied_gpr, add_none, validate_arith_insn, NULL, NULL},

    {0x040, 0x000, 0, XMain, "imul", validate_mul_insn, add_mul_ssf, parse_mul_ssf, add_size_spec, add_none, validate_any_ops, NULL, NULL},
    {0x041, 0x400, 1, XMain, "add", validate_allow_any, noop_print_h, parse_none, add_implied_gpr, add_none, validate_arith_insn, NULL, NULL},
    {0x042, 0x400, 1, XMain, "sub", validate_allow_any, noop_print_h, parse_none, add_implied_gpr, add_none, validate_arith_insn, NULL, NULL},
    {0x043, 0x400, 1, XMain, "and", validate_allow_any, noop_print_h, parse_none, add_implied_gpr, add_none, validate_arith_insn, NULL, NULL},
    {0x044, 0x400, 1, XMain, "or" , validate_allow_any, noop_print_h, parse_none, add_implied_gpr, add_none, validate_arith_insn, NULL, NULL},
    {0x045, 0x400, 1, XMain, "xor", validate_allow_any, noop_print_h, parse_none, add_implied_gpr, add_none, validate_arith_insn, NULL, NULL},
    {0x046, 0x400, 1, XMain, "bnot",validate_arith_mem_h, add_arith_lf_h, parse_arith_lf, add_none, add_none, validate_unary_arith_insn, &clever_insns[ARITH_GPR_SRC_SPECS+6], NULL},
    {0x047, 0x400, 1, XMain, "neg", validate_arith_mem_h, add_arith_lf_h, parse_arith_lf, add_none, add_none, validate_unary_arith_insn, &clever_insns[ARITH_GPR_SRC_SPECS+7], NULL},
    {0x048, 0x400, 0, XMain, "idiv", validate_allow_any, add_div_sswf, parse_div_sswf, add_size_spec, add_none, validate_any_ops, NULL, NULL},
    {0x049, 0x400, 1, XMain, "add", validate_allow_any, noop_print_h, parse_none, add_none, add_implied_gpr, validate_arith_insn, NULL, NULL},
    {0x04A, 0x400, 1, XMain, "sub", validate_allow_any, noop_print_h, parse_none, add_none, add_implied_gpr, validate_arith_insn, NULL, NULL},
    {0x04B, 0x400, 1, XMain, "and", validate_allow_any, noop_print_h, parse_none, add_none, add_implied_gpr, validate_arith_insn, NULL, NULL},
    {0x04C, 0x400, 1, XMain, "or" , validate_allow_any, noop_print_h, parse_none, add_none, add_implied_gpr, validate_arith_insn, NULL, NULL},
    {0x04D, 0x400, 1, XMain, "xor", validate_allow_any, noop_print_h, parse_none, add_none, add_implied_gpr, validate_arith_insn, NULL, NULL},
    {0x04E, 0x400, 0, XMain, "bnot", validate_allow_any, noop_print_h, parse_none, add_implied_gpr, add_none, validate_unary_arith_insn, NULL, NULL},
    {0x04F, 0x400, 0, XMain, "neg", validate_allow_any, noop_print_h, parse_none, add_implied_gpr, add_none, validate_unary_arith_insn, NULL, NULL},
    
    {0x06c, 0x000, 2, XMain, "cmp", validate_allow_none, noop_print_h, parse_none, add_none, add_none, validate_cmp_insn, &clever_insns[CMP_GPR_SPEC], NULL},
    {0x06d, 0x000, 2, XMain, "test", validate_allow_none, noop_print_h, parse_none, add_none, add_none, validate_cmp_insn, &clever_insns[CMP_GPR_SPEC], NULL},
    {0x06e, 0x000, 1, XMain, "cmp", validate_allow_any, noop_print_h, parse_none, add_implied_gpr, add_none, validate_cmp_insn, &clever_insns[CMP_GPR_SPEC], NULL},
    {0x06f, 0x000, 1, XMain, "test", validate_allow_any, noop_print_h, parse_none, add_implied_gpr, add_none, validate_cmp_insn, &clever_insns[CMP_GPR_SPEC], NULL},

    {0x100, 0x400, 1, XFloat, "round", validate_flags_only, print_flags_h, parse_flags, add_none, add_none, validate_float_unop, NULL, NULL},
    {0x101, 0x400, 1, XFloat, "ceil", validate_flags_only, print_flags_h, parse_flags, add_none, add_none, validate_float_unop, NULL, NULL},
    {0x102, 0x400, 1, XFloat, "floor", validate_flags_only, print_flags_h, parse_flags, add_none, add_none, validate_float_unop, NULL, NULL},
    {0x103, 0x400, 1, XFloat, "fabs", validate_flags_only, print_flags_h, parse_flags, add_none, add_none, validate_float_unop, NULL, NULL},
    {0x104, 0x400, 1, XFloat, "fneg", validate_flags_only, print_flags_h, parse_flags, add_none, add_none, validate_float_unop, NULL, NULL},
    {0x105, 0x400, 1, XFloat, "finv", validate_flags_only, print_flags_h, parse_flags, add_none, add_none, validate_float_unop, NULL, NULL},
    {0x106, 0x400, 2, XFloat, "fadd", validate_flags_only, print_flags_h, parse_flags, add_none, add_none, validate_float_binop, NULL, NULL},
    {0x107, 0x400, 2, XFloat, "fsub", validate_flags_only, print_flags_h, parse_flags, add_none, add_none, validate_float_binop, NULL, NULL},
    {0x108, 0x400, 2, XFloat, "fmul", validate_flags_only, print_flags_h, parse_flags, add_none, add_none, validate_float_binop, NULL, NULL},
    {0x109, 0x400, 2, XFloat, "fdiv", validate_flags_only, print_flags_h, parse_flags, add_none, add_none, validate_float_binop, NULL, NULL},
    {0x10A, 0x400, 2, XFloat, "frem", validate_flags_only, print_flags_h, parse_flags, add_none, add_none, validate_float_binop, NULL, NULL},
    {0x10B, 0x400, 3, XFloat, "ffma", validate_flags_only, print_flags_h, parse_flags, add_none, add_none, validate_float_top, NULL, NULL},

    {0x118, 0x000, 1, XFloat, "fcmpz", validate_allow_none, noop_print_h, parse_none, add_none, add_none, validate_float_cmpz, NULL, NULL},
    {0x119, 0x000, 2, XFloat, "fcmp" , validate_allow_none, noop_print_h, parse_none, add_none, add_none, validate_float_cmp, NULL, NULL},
    
    {0x120, 0x400, 1, XFloatExt, "exp", validate_flags_only, print_flags_h, parse_flags, add_none, add_none, validate_float_unop, NULL, NULL},
    {0x121, 0x400, 1, XFloatExt, "ln" , validate_flags_only, print_flags_h, parse_flags, add_none, add_none, validate_float_unop, NULL, NULL},
    {0x122, 0x400, 1, XFloatExt, "lg" , validate_flags_only, print_flags_h, parse_flags, add_none, add_none, validate_float_unop, NULL, NULL},
    {0x123, 0x400, 1, XFloatExt, "sin", validate_flags_only, print_flags_h, parse_flags, add_none, add_none, validate_float_unop, NULL, NULL},
    {0x124, 0x400, 1, XFloatExt, "cos", validate_flags_only, print_flags_h, parse_flags, add_none, add_none, validate_float_unop, NULL, NULL},
    {0x125, 0x400, 1, XFloatExt, "tan", validate_flags_only, print_flags_h, parse_flags, add_none, add_none, validate_float_unop, NULL, NULL},
    {0x126, 0x400, 1, XFloatExt, "asin", validate_flags_only, print_flags_h, parse_flags, add_none, add_none, validate_float_unop, NULL, NULL},
    {0x127, 0x400, 1, XFloatExt, "acos", validate_flags_only, print_flags_h, parse_flags, add_none, add_none, validate_float_unop, NULL, NULL},
    {0x128, 0x400, 1, XFloatExt, "atan", validate_flags_only, print_flags_h, parse_flags, add_none, add_none, validate_float_unop, NULL, NULL},
    {0x129, 0x400, 1, XFloatExt, "exp2", validate_flags_only, print_flags_h, parse_flags, add_none, add_none, validate_float_unop, NULL, NULL},
    {0x12A, 0x400, 1, XFloatExt, "log10", validate_flags_only, print_flags_h, parse_flags, add_none, add_none, validate_float_unop, NULL, NULL},
    {0x12B, 0x400, 1, XFloatExt, "lnp1", validate_flags_only, print_flags_h, parse_flags, add_none, add_none, validate_float_unop, NULL, NULL},
    {0x12C, 0x400, 1, XFloatExt, "expm1", validate_flags_only, print_flags_h, parse_flags, add_none, add_none, validate_float_unop, NULL, NULL},
    {0x12D, 0x400, 1, XFloatExt, "sqrt", validate_flags_only, print_flags_h, parse_flags, add_none, add_none, validate_float_unop, NULL, NULL},

    {0x130, 0x000, 0, XFloat, "fraiseexcept", validate_allow_none, noop_print_h, parse_none, add_none, add_none, validate_any_ops, NULL, NULL},
    {0x131, 0x000, 0, XFloat, "fraiseexcept", validate_allow_none, noop_print_h, parse_none, add_none, add_none, validate_any_ops, NULL, NULL},
    
    {0x200, 0x000, 2, XMain, "xchg",validate_allow_none, noop_print_h, parse_none, add_none, add_none, validate_xhcg, NULL, NULL},
    {0x201, 0x000, 3, XMain, "cmpxchg", validate_allow_none, noop_print_h, parse_none, add_none, add_none, validate_cmpxchg, NULL, NULL},
    {0x202, 0x000, 3, XMain, "wcmpxchg", validate_allow_none, noop_print_h, parse_none, add_none, add_none, validate_cmpxchg, NULL, NULL},
    {0x203, 0x000, 0, XMain, "fence", validate_allow_none, noop_print_h, parse_none, add_none, add_none, validate_any_ops, NULL, NULL},
    
    {0x230, 0x000, 0, XRand, "rdpoll", validate_allow_none, noop_print_h, parse_ijmp_target, add_implied_gpr, add_none, validate_any_ops, NULL, NULL},

    {0x400, 0x000, prefix, XVec, "vec", validate_allow_ss, noop_print_h, parse_none, add_size_spec, add_none, validate_any_ops, NULL, NULL},
    {0x401, 0x000, 2, XVec, "vmov", validate_allow_none, noop_print_h, parse_none, add_none, add_none, validate_vmov, NULL, NULL},

    {0x801, 0x000, 0, XMain, "halt", validate_allow_none, noop_print_h, parse_none, add_none, add_none, validate_any_ops, NULL, NULL},
    {0x802, 0x000, 0, XMain, "pcfl", validate_allow_none, noop_print_h, parse_none, add_none, add_none, validate_any_ops, NULL, NULL},
    {0x803, 0x000, 0, XMain, "flall", validate_allow_none, noop_print_h, parse_none, add_none, add_none, validate_any_ops, NULL, NULL},
    {0x804, 0x000, 1, XMain, "dflush", validate_allow_none, noop_print_h, parse_none, add_none, add_none, validate_needs_mem, NULL, NULL},
    {0x805, 0x000, 1, XMain, "iflush", validate_allow_none, noop_print_h, parse_none, add_none, add_none, validate_needs_mem, NULL, NULL},
    {0x806, 0x028, 0, XMain, "in", validate_allow_ss, noop_print_h, parse_none, add_size_spec, add_none, validate_any_ops, NULL, NULL},
    {0x807, 0x028, 0, XMain, "out", validate_allow_ss, noop_print_h, parse_none, add_size_spec, add_none, validate_any_ops, NULL, NULL},
    {0x808, 0x000, 1, XMain, "storegf", validate_allow_none, noop_print_h, parse_none, add_none, add_none, validate_needs_mem, NULL, NULL},
    {0x809, 0x000, 1, XMain, "rstregf", validate_allow_none, noop_print_h, parse_none, add_none, add_none, validate_needs_mem, NULL, NULL},
    {0xe00, 0x000, 0, XVirtualization, "vmcreate", validate_allow_none, noop_print_h, parse_none, add_none, add_none, validate_any_ops, NULL, NULL},
    {0xe01, 0x000, 0, XVirtualization, "vmdestroy", validate_allow_none, noop_print_h, parse_none, add_none, add_none, validate_any_ops, NULL, NULL},
    {0xfff, 0x000, 0, XMain, "und", validate_allow_any, noop_print_h, parse_none, add_none, add_none, validate_any_ops, NULL, NULL},
    {0, 0, 0, 0, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL}
};

static bool validate_arith_mem_h(uint16_t s, union clever_extensions* ext __attribute_maybe_unused__){
    return (s&0x6)==0;
}

static char* add_arith_lf_h(uint16_t s, char* p){
    if(CLEVER_OP_H(s)==0)
        return p;
    *p++ = '.';

    if(s&0x8)
        *p++ = 'l';
    if(s&0x1)
        *p++ = 'f';
    return p;
}

static bool validate_mul_insn(uint16_t s, union clever_extensions* ext __attribute_maybe_unused__){
    return (s&0x2)==0;
}


static char* add_mul_ssf(uint16_t s, char* p){
    if(s&0x1){
        *p++ = '.';
        *p++ = 'f';
    }
    return p;   
}
static char* add_div_sswf(uint16_t s, char* p){
    if(s&0x3){
        *p++ = '.';
    }
    if(s&0x2){
        *p++ = 'w';
    }
    if(s&0x1){
        *p++ = 'f';
    }
    return p;  
}

static size_t add_size_spec(uint16_t s, union clever_operand* ops, size_t curr_size){
    uint16_t size = CLEVER_SS_TO_SIZE(s&3);
    ops[curr_size].size_only.kind = size_only;
    ops[curr_size].size_only.size = size;
    return size;
}

static bool is_writable(union clever_operand* op){
    switch(op->direct.kind){
        case direct:
            return op->direct.reg!=16; // `ip` is not writable at all
        case indirect:
            return true;
        case long_imm:
            return op->long_imm.mem;
        case short_imm:
            return false;
        case size_only:
            return false;
    }
    return false; // unreachable code
}

static bool is_simple(union clever_operand* op){
    switch(op->direct.kind){
        case direct:
            return op->direct.reg<16||op->direct.vec;
        case short_imm:
            return true;
        case long_imm:
            return !(op->long_imm.mem);
        case indirect:
            false;
        case size_only:
            return false;
    }
    return false; // unreachable code
}

static bool is_gpr_or_not_reg(union clever_operand* op){
    switch(op->direct.kind){
        case direct:
            return op->direct.reg<16;
        default:
            return true;
    }
    return false; // unreachable code
}

static bool is_mov_simple(union clever_operand* op){
    switch(op->direct.kind){
        case direct:
            return true;
        case short_imm:
            return true;
        case long_imm:
            return !(op->long_imm.mem);
        case indirect:
            false;
        case size_only:
            return false;
    }
    return false; // unreachable code
}

static bool is_reg(union clever_operand* op){
    return op->direct.kind==direct;
}

static bool is_mem(union clever_operand* op){
    switch(op->direct.kind){
        case indirect:
            return true;
        case long_imm:
            return op->long_imm.mem;
        default:
            return false;
    }
    return false; // unreachable code
}

static bool is_float(union clever_operand* op){
    switch(op->direct.kind){
        case direct:
            return ((24<=op->direct.reg)&&(op->direct.reg<32))||op->direct.vec;
        default:
            return true;
    }
}

static bool is_float_simple(union clever_operand* op){
    switch(op->direct.kind){
        case direct:
            return ((24<=op->direct.reg)&&(op->direct.reg<32))||op->direct.vec;
        case short_imm:
            return true;
        case long_imm:
            return !op->long_imm.mem;
        case indirect:
            return false;
        case size_only:
            return false;
    }
    return false; // unreachable code
}

static size_t add_implied_gpr(uint16_t s, union clever_operand* ops, size_t curr_size){
    ops[curr_size].direct.kind = direct;
    ops[curr_size].direct.reg = CLEVER_OP_H(s);
    ops[curr_size].direct.size = 3;
    ops[curr_size].direct.vec = false;
    return curr_size+1;
}

static bool validate_arith_insn(uint16_t s __attribute_maybe_unused__, union clever_operand* ops){
    if(!(is_simple(&ops[0]) || is_simple(&ops[1])))
        return false;

    if(!(is_gpr_or_not_reg(&ops[0])&&is_gpr_or_not_reg(&ops[1])))
        return false;

    return is_writable(&ops[0]);
}

static bool validate_unary_arith_insn(uint16_t s __attribute_maybe_unused__, union clever_operand* ops){
    return is_gpr_or_not_reg(&ops[0])&&is_writable(&ops[0]);
}
static bool validate_cmp_insn(uint16_t s __attribute_maybe_unused__, union clever_operand* ops){
    return is_reg(&ops[0])||is_reg(&ops[1]);
}

static bool validate_mov(uint16_t s __attribute_maybe_unused__, union clever_operand* ops){
    if(!(is_mov_simple(&ops[0])||is_mov_simple(&ops[1])))
        return false;
    return is_writable(&ops[1]);
}
static bool validate_lea(uint16_t s __attribute_maybe_unused__, union clever_operand* op){
    return is_reg(&op[0])&&is_mem(&op[0]);
}

static bool validate_pop(uint16_t s __attribute_maybe_unused__, union clever_operand* op){
    return is_writable(&op[0]);
}

static bool validate_needs_mem(uint16_t s __attribute_maybe_unused__, union clever_operand* op){
    return is_mem(&op[0]);
}

static bool validate_flags_only(uint16_t s, union clever_extensions* ext __attribute_maybe_unused__){
    return (s&0xe)!=0;
}


char* print_flags_h(uint16_t s, char* p){
    if(s&0x1){
        *p++ = '.';
        *p++ = 'f';
    }
    return p;
}
char* print_fixed_h(uint16_t s, char* p){
    if(CLEVER_OP_H(s)==0)
        return p;
    *p++ = '.';

    *p++ = '0'+((s&0xc)>>2);

    if(s&0x2)
        *p++ = 'i';
    else
        *p++ = 'x';
    if(s&0x1)
        *p++ = 'f';
    return p;
}

static bool validate_float2int(uint16_t s __attribute_maybe_unused__, union clever_operand* op){
    if(!(is_simple(&op[0])||is_float_simple(&op[1])))
        return false;
    if(!is_writable(&op[0]))
        return false;
    return is_gpr_or_not_reg(&op[0])&&is_float(&op[1]);
}

static bool validate_int2float(uint16_t s __attribute_maybe_unused__, union clever_operand* op){
    if(!(is_simple(&op[1])||is_float_simple(&op[0])))
        return false;
    if(!is_writable(&op[0]))
        return false;
    return is_gpr_or_not_reg(&op[1])&&is_float(&op[0]);
}
static bool validate_float_unop(uint16_t s __attribute_maybe_unused__, union clever_operand* op){
    return is_writable(&op[0])&&is_float(&op[0]);
}

static bool validate_float_binop(uint16_t s __attribute_maybe_unused__, union clever_operand* op){
    if(!(is_float_simple(&op[0])||is_float_simple(&op[1])))
        return false;
    if(!is_writable(&op[0]))
        return false;
    return is_float(&op[0])&&is_float(&op[1]);
}

static bool validate_float_top(uint16_t s __attribute_maybe_unused__, union clever_operand* op){
    bool has_complex = false;
    for(int i = 0;i<3;i++){
        if(!is_float_simple(&op[0])){
            if(has_complex)
                return false;
            has_complex = true;
        }
        if(!is_float(&op[0]))
            return false;
    }
    return is_writable(&op[0]);
}

static bool validate_float_cmpz(uint16_t s __attribute_maybe_unused__, union clever_operand* op){
    return is_float(&op[0]);
}

static bool validate_float_cmp(uint16_t s __attribute_maybe_unused__, union clever_operand* op){
    if(!(is_float_simple(&op[0])||is_float_simple(&op[1])))
        return false;
    return is_float(&op[0])&&is_float(&op[1]);
}

char* print_cc_h(uint16_t s, char* p){
    strcpy(p,ccsuffix[CLEVER_OP_H(s)]);
    p += strlen(ccsuffix[CLEVER_OP_H(s)]);
    return p;
}

static size_t add_block_mem_refs(uint16_t s, union clever_operand* op, size_t curr_size){
    switch(CLEVER_OPC(s)){
    case 0x02a:
    case 0x02b:
    case 0x02d:
        op[curr_size].indirect.base = r5;
        op[curr_size].indirect.index_kind_and_val = 0x10;
        op[curr_size].indirect.scale = 0;
        op[curr_size].indirect.kind = indirect;
        op[curr_size].indirect.size = CLEVER_OP_H(s)&0x3;
        curr_size++;
        break;
    case 0x02c:
    case 0x02e:
        op[curr_size].direct.kind = direct;
        op[curr_size].direct.reg = r0;
        op[curr_size].direct.size = CLEVER_OP_H(s)&0x3;
        op[curr_size].direct.vec = false;
        curr_size++;
        break;
    }

    switch(CLEVER_OPC(s)){
    case 0x02a:
    case 0x02b:
    case 0x02c:
    case 0x02e:
        op[curr_size].indirect.base = r4;
        op[curr_size].indirect.index_kind_and_val = 0x10;
        op[curr_size].indirect.scale = 0;
        op[curr_size].indirect.kind = indirect;
        op[curr_size].indirect.size = CLEVER_OP_H(s)&0x3;
        curr_size++;
        break;
    case 0x02d:
        op[curr_size].direct.kind = direct;
        op[curr_size].direct.reg = r0;
        op[curr_size].direct.size = CLEVER_OP_H(s)&0x3;
        op[curr_size].direct.vec = false;
        curr_size++;
        break;
    }
    return curr_size;
}
static bool validate_block_mem_refs(uint16_t s __attribute_maybe_unused__, union clever_operand* op __attribute_maybe_unused__){
    return true; // for now
}

static const char* parse_block_mem_refs(uint16_t* opc, const char* op, const struct clever_instruction* insn __attribute_maybe_unused__){
    uint16_t size = 0;
    switch(CLEVER_OPC(*opc)){
    case 0x02a:
    case 0x02b:
    case 0x02d:
        size = insn->ops[0].indirect.size;
        break;
    case 0x02c:
    case 0x02e:
        size = insn->ops[0].direct.size;
    }
    uint16_t ss = (size==8?0:size==16?1:size==32?2:3);
    *opc |= ss;
    return op;
}

static bool validate_xhcg(uint16_t s __attribute_maybe_unused__, union clever_operand* op){
    if(!(is_mov_simple(&op[0])||is_mov_simple(&op[1])))
        return false;
    return (is_writable(&op[0])&&is_writable(&op[1]));
}
static bool validate_cmpxchg(uint16_t s __attribute_maybe_unused__, union clever_operand* op){
    bool has_complex = false;
    for(int i = 0;i<3;i++){
        if(!is_mov_simple(&op[0])){
            if(has_complex)
                return false;
            has_complex = true;
        }
    }
    return (is_writable(&op[0])&&is_writable(&op[1]));
}


static bool validate_vmov(uint16_t s __attribute_maybe_unused__, union clever_operand* op){
    if(!(is_mov_simple(&op[0])||is_mov_simple(&op[1])))
        return false;
    return is_writable(&op[0]);
}


static const char* parse_arith_lf(uint16_t* opc, const char* op, const struct clever_instruction* insn __attribute_maybe_unused__){
    if(*op=='.'){
        op++;
        while(!isspace(*op)){
            switch(*op++){
                case 'l': *opc |= 0x80; break;
                case 'f': *opc |= 0x01; break;
                default: return NULL;
            }
        }
    }
    return op;
}
static const char* parse_mul_ssf(uint16_t* opc, const char* op,const struct clever_instruction* insn){
    uint16_t s = insn->ops[0].size_only.size;
    uint16_t ss = (s==8?0:s==16?1:s==32?2:3);
    *opc |= ss<<2;
    if(*op=='.'){
        op++;
        if(*op++!='f')
            return NULL;
        else
            *opc |= 0x01;
    }
    return op;
}
static const char* parse_div_sswf(uint16_t* opc, const char* op, const struct clever_instruction* insn){
    uint16_t s = insn->ops[0].size_only.size;
    uint16_t ss = (s==8?0:s==16?1:s==32?2:3);
    *opc |= ss<<2;
    if(*op=='.'){
        op++;
        while(!isspace(*op)){
            switch(*op++){
                case 'w': *opc |= 0x02; break;
                case 'f': *opc |= 0x01; break;
                default: return NULL;
            }
        }
    }
    return op;
}
static const char* parse_flags(uint16_t* opc, const char* op, const struct clever_instruction* insn __attribute_maybe_unused__){
    if(*op=='.'){
        op++;
        if(*op++!='f')
            return NULL;
        else
            *opc |= 0x01;
    }
    return op;
}
static const char* parse_fixed(uint16_t* opc, const char* op, const struct clever_instruction* insn __attribute_maybe_unused__){
    if(*op=='.'){
        unsigned long l = strtoul(op,(char**)&op,10);
        if(l>3)
            return NULL;
        *opc |= (uint16_t)(l<<2);
        while(!isspace(*op)){
            switch(*op++){
                case 'i': *opc |= 0x02; break;
                case 'f': *opc |= 0x01; break;
                default: return NULL;
            }
        }
    }
    return op;
}
static const char* parse_cc(uint16_t* opc, const char* op, const struct clever_instruction* insn __attribute_maybe_unused__){
    for (const char* const* p = &16[ccsuffix];p-->ccsuffix;){
        if (strncmp(op,*p,strlen(*p))==0){
            op += strlen(*p);
            *opc |= (p-ccsuffix);
            break;
        }
    }
    return op;
}
