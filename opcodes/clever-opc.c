#include "clever-opc.h"

#include <string.h>
#include <stdlib.h>

static unsigned int extract_unconditional_branch_size(uint16_t s){
    return CLEVER_SS_TO_SIZE(CLEVER_BRANCH_UNCONDITIONAL_SIZE(s));
}

static unsigned int no_addr_operand(uint16_t s __attribute_maybe_unused__){
    return 0;
}

static bool validate_indirect_branch_ss(uint16_t s, union clever_extensions* cext __attribute_maybe_unused__){
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
    if(CLEVER_OPCODE_H(s)==14){
        p-=4;
        strcpy(p,"fret");
        return p+4;
    }else if(CLEVER_OPCODE_H(s)==15){
        p-=4;
        strcpy(p,"ifjmp");
        return p+5;
    }else{
        return p;
    }
}

static char* print_vfield(uint16_t s, char* p){
    if(CLEVER_OPCODE_H(s)==1){
        strcpy(p,".v");
        return p+2;
    }else{
        return p;
    }
}

static size_t add_none(uint16_t s __attribute_maybe_unused__, clever_operand* ops __attribute_maybe_unused__, size_t curr_size){
    return curr_size;
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
    ops[curr_size].short_imm.imm.value = 0;
    return curr_size+1;
}

const struct clever_instruction_info branch_jmp   = {0x7c0, 0x000, branch_addr, XMain, "jmp"  , validate_indirect_branch_ss, noop_print_h, add_none, add_none};
const struct clever_instruction_info branch_call  = {0x7c1, 0x000, branch_addr, XMain, "call" , validate_indirect_branch_ss, noop_print_h, add_none, add_none};
const struct clever_instruction_info branch_fcall = {0x7c2, 0x000, branch_addr, XMain, "fcall", validate_indirect_branch_ss, noop_print_h, add_none, add_none};
const struct clever_instruction_info branch_ret   = {0x7c3, 0x000, 0,           XMain, "ret"  , validate_allow_none,         noop_print_h, add_none, add_none};
const struct clever_instruction_info branch_scall = {0x7c4, 0x000, 0,           XMain, "scall", validate_allow_none,         noop_print_h, add_none, add_none};
const struct clever_instruction_info branch_int   = {0x7c5, 0x000, 0,           XMain, "int"  , validate_allow_any,          noop_print_h, add_int , add_none};
const struct clever_instruction_info branch_ijmp  = {0x7c6, 0x000, 0,           XMain, "ijmp" , validate_allow_any,     print_ijmp_target, add_ijmp_target,add_none};
const struct clever_instruction_info branch_icall = {0x7c7, 0x000, 0,           XMain, "icall", validate_allow_any,          noop_print_h, add_reg_op,add_none};
const struct clever_instruction_info branch_ifcall= {0x7c8, 0x000, 0,           XMain,"ifcall", validate_allow_none,         noop_print_h, add_reg_op,add_none};
const struct clever_instruction_info branch_jsm   = {0x7c9, 0x000, branch_addr, XMain, "jsm"  , validate_indirect_branch_ss, noop_print_h, add_none, add_none};
const struct clever_instruction_info branch_callsm= {0x7ca, 0x000, branch_addr, XMain,"callsm", validate_callsm,             print_vfield, add_none, add_none};
const struct clever_instruction_info branch_retrsm= {0x7cb, 0x000, 0,           XMain,"retrsm", validate_allow_none,         noop_print_h, add_none, add_none};

const struct clever_instruction_info branch_scret  = {0xfc6, 0x000, 0,  XMain, "scret", validate_allow_none, noop_print_h, add_none,add_none};
const struct clever_instruction_info branch_iret   = {0xfc7, 0x000, 0,  XMain, "iret" , validate_allow_none, noop_print_h, add_none,add_none};

const struct clever_instruction_info branch_hret   = {0xfd6, 0x000, 0, XVirtualization, "hret"   , validate_allow_none, noop_print_h, add_none,add_none};
const struct clever_instruction_info branch_hresume= {0xfd7, 0x000, 0, XVirtualization, "hresume", validate_allow_none, noop_print_h, add_none,add_none};


const struct clever_branch_info ubranches[last_user_branch] = {
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

const struct clever_branch_info sbranches[(last_supervisor_branch-first_super_branch)+(last_hyper_branch-hypervisor_branches)] = {
    {no_addr_operand, no_indirect_branch, false, &branch_scret},
    {no_addr_operand, no_indirect_branch, false, &branch_iret},

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

static char* add_jump_suffix(uint16_t opc, char* op){
    switch(CLEVER_OPC(opc)&0xf){
        case parity: 
            strcpy(op,"p");
            op += 1;
            break;
        case carry:
            strcpy(op,"c");
            op += 1;
            break;
        case overflow:
            strcpy(op,"v");
            op += 1;
            break;
        case zero:
            strcpy(op,"z");
            op += 1;
            break;
        case less_than:
            strcpy(op,"lt");
            op += 2;
            break;
        case less_equal:
            strcpy(op,"le");
            op += 2;
            break;
        case below_equal:
            strcpy(op,"be");
            op += 2;
            break;
        case minus:
            strcpy(op,"mi");
            op += 2;
            break;
        case plus:
            strcpy(op,"pl");
            op += 2;
            break;
        case above:
            strcpy(op,"a");
            op += 1;
            break;
        case greater_than:
            strcpy(op,"gt");
            op += 2;
            break;
        case greater_equal:
            strcpy(op,"ge");
            op += 2;
            break;
        case nonzero:
            strcpy(op, "nz");
            op += 2;
            break;
        case no_overflow:
            strcpy(op,"nv");
            op += 2;
            break;
        case no_carry:
            strcpy(op,"nc");
            op += 2;
            break;
        case no_parity:
            strcpy(op,"np");
            op += 2;
            break;
    }
    
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

const struct clever_instruction_info branch_cond = {0x700, 0x000, 0, XMain, "j", validate_allow_any, add_jump_suffix, add_none, add_none};

static unsigned int cond_branch_size(uint16_t s){
    return CLEVER_INDIRECT_SIZE_SS(CLEVER_BRANCH_CONDITIONAL_SIZE(CLEVER_OPC(s)));
}

const struct clever_branch_info cond_branch_info = {cond_branch_size, no_indirect_branch, true, &branch_cond};

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