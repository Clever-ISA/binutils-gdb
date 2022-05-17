#include "sysdep.h"
#include "dis-asm.h"
#include "libiberty.h"
#include <opcode/clever.h>
#include "clever-opc.h"
#include <stddef.h>

const struct clever_instruction_info invalid_instruction = {0, 0, 0, XMain, "**INVALID INSTRUCTION**", NULL, NULL, NULL, NULL, NULL};


union clever_extensions enabled_extensions = {.x_main = 1, .x_float = 1, .x_vec = 1, .x_float_ext = 1, .x_rand = 1, .x_virtualization = 1};

static const struct clever_branch_info* decode_branch_insn(uint16_t op,union clever_extensions* ext){
    if(CLEVER_BRANCH_IS_CONDITIONAL(op))
        return &clever_cond_branch_info;
    else if(CLEVER_IS_SUPER_BRANCH(op)){
        ptrdiff_t off;
        if(CLEVER_BRANCH_IS_RELATIVE(op)){
            off = CLEVER_BRANCH_UNCONDITIONAL_OP(op)-first_hyper_branch+(last_supervisor_branch-first_super_branch);
        }else{
            off = CLEVER_BRANCH_UNCONDITIONAL_OP(op);
            if(off>(last_supervisor_branch-first_super_branch))
                return NULL;
        }
        if(CLEVER_HAS_EXTENSION(ext,clever_sbranches[off].insn->clever_extension)) // This check could probably be lifted into the BRANCH_IS_RELATIVE check, but this is future-proof
            return &clever_sbranches[off];
        else
            return NULL; 
    }else{
        ptrdiff_t off = CLEVER_BRANCH_UNCONDITIONAL_OP(op);
        if(off>last_user_branch)
            return NULL;
        if(!(clever_ubranches[off].allow_relative)&&CLEVER_BRANCH_IS_RELATIVE(op))
            return false;
        if(CLEVER_HAS_EXTENSION(ext,clever_ubranches[off].insn->clever_extension)) // Likewise, this is presently strictly true, but user branches might be added behind features
            return &clever_ubranches[off];
        else
            return NULL;
    }
}

static const struct clever_instruction_info* decode_insn(uint16_t op, union clever_extensions* ext){
    if(CLEVER_IS_BRANCH(op)){
        const struct clever_branch_info* branch = decode_branch_insn(op,ext);
        if(branch)
            return branch->insn;
    }

    for(ptrdiff_t p = 0;clever_insns[p].mnemonic;p++){
        if(CLEVER_OPC(op)==clever_insns[p].opcode){
            if((clever_insns[p].validate_h)(op,ext)){
                if(CLEVER_HAS_EXTENSION(ext,clever_insns[p].clever_extension))
                    return &clever_insns[p];
            } // Overloading opcodes using reserved h fields may be done in the future, for example, `xchg` prefix
                
        }
    }
    return NULL;
}

static long decode_operand(uint16_t t, bfd_vma memaddr, disassemble_info* inf, union clever_operand* op){

    op->direct.kind = CLEVER_OPR_KIND(t);

    switch(CLEVER_OPR_KIND(t)){
        case direct:
            op->direct.vec = CLEVER_REG_IS_VECTOR(t);
            op->direct.reg = CLEVER_REG_OPERAND(t);
            op->direct.size = CLEVER_REG_SIZE_SS(t);
            break;
        case indirect:
            op->indirect.base =CLEVER_INDIRECT_BASE(t);
            op->indirect.size = CLEVER_INDIRECT_SIZE_SS(t);
            op->indirect.scale = CLEVER_INDIRECT_SCALE(t);
            op->indirect.index_kind_and_val = (CLEVER_INDIRECT_IS_STATIC_OFF(t)<<4)|CLEVER_INDIRECT_OFFSET(t);
            break;
        case short_imm:
            op->short_imm.imm.size = 0;
            op->short_imm.imm.symbol = NULL;
            op->short_imm.imm.value = CLEVER_SHORT_VAL(t);
            op->short_imm.rel = CLEVER_SHORT_REL(t);
            break;
        case long_imm:
            op->long_imm.imm.size = CLEVER_LONG_IMM_SIZE(t);
            bfd_byte buf[16] = {0};
            if((inf->read_memory_func)(memaddr,buf,CLEVER_SS_TO_SIZE(op->long_imm.imm.size),inf)<0)
                return -1;
            op->long_imm.imm.value = ((uint64_t)buf[0])|(((uint64_t)buf[1])<<8)|(((uint64_t)buf[2])<<16)|(((uint64_t)buf[3])<<24)
                |(((uint64_t)buf[4])<<32)|((((uint64_t)buf[5])<<40))|(((uint64_t)buf[6])<<48)|(((uint64_t)buf[7])<<56);
            if(op->long_imm.imm.size==4)
                op->long_imm.imm.value_hi = ((uint64_t)buf[8])|(((uint64_t)buf[9])<<8)|(((uint64_t)buf[10])<<16)|(((uint64_t)buf[11])<<24)
                |(((uint64_t)buf[12])<<32)|((((uint64_t)buf[13])<<40))|(((uint64_t)buf[14])<<48)|(((uint64_t)buf[15])<<56);
            else
                op->long_imm.imm.symbol = NULL;
            op->long_imm.rel = CLEVER_LONG_IMM_REL(t);
            op->long_imm.mem = CLEVER_LONG_IS_MEM(t);
            op->long_imm.mem_size = CLEVER_LONG_MEM_SIZE(t);
            return CLEVER_SS_TO_SIZE(op->long_imm.imm.size);
    }
    return 0;
}


static long decode_instruction(disassemble_info* inf, struct clever_instruction* insn, bfd_vma memaddr){
    long disp = 0;
    bfd_byte ibuf[2];
    uint16_t iword;
    if((inf->read_memory_func)(inf->buffer_vma,ibuf,2,inf)<0)
        return -1;
    disp += 2;
    memaddr += 2;
    iword = (((uint16_t)ibuf[0])<<8)|((uint16_t)ibuf[1]);
    insn->opc = iword;
    const struct clever_branch_info* branch = decode_branch_insn(iword,&enabled_extensions);
    if(branch){
        insn->insn = branch->insn;
        insn->num_ops = (branch->insn->pre_insert_operands)(iword,insn->ops,insn->num_ops);
        bfd_byte buf[8] = {0};
        size_t branch_ss = (branch->dest_size)(iword);
        if(branch_ss>8)
            return -1;
        if(branch_ss>0){
            if((inf->read_memory_func)(memaddr,buf,branch_ss,inf)<0)
                return -1;
            insn->ops[insn->num_ops].long_imm.kind = long_imm;
            insn->ops[insn->num_ops].long_imm.mem = false;
            insn->ops[insn->num_ops].long_imm.mem_size = 0;
            insn->ops[insn->num_ops].long_imm.rel = CLEVER_BRANCH_IS_RELATIVE(iword);
            insn->ops[insn->num_ops].long_imm.imm.symbol = NULL;
            insn->ops[insn->num_ops].long_imm.imm.size = (branch_ss==2?1: (branch_ss==4 ? 2 : 3));
            insn->ops[insn->num_ops].long_imm.imm.value = ((uint64_t)buf[0])|(((uint64_t)buf[1])<<8)|(((uint64_t)buf[2])<<16)|(((uint64_t)buf[3])<<24)
                |(((uint64_t)buf[4])<<32)|((((uint64_t)buf[5])<<40))|(((uint64_t)buf[6])<<48)|(((uint64_t)buf[7])<<56);
            insn->num_ops++;
        }
        insn->num_ops = (branch->insn->post_insert_operands)(iword,insn->ops,insn->num_ops);
    }else{
        insn->insn = decode_insn(iword, &enabled_extensions);
        if(!insn->insn){
            insn->insn = &invalid_instruction;
            insn->num_ops = 0;
            insn->prefix_opc = 0;
            return disp;
        }
        if(insn->insn->operand_count==prefix){
            if(insn->prefix_insn){
                insn->insn = &invalid_instruction;
                insn->num_ops = 0;
                insn->prefix_opc = 0;
                return disp;
            }

            insn->prefix_insn = insn->insn;
            insn->prefix_opc = iword;
            long size = decode_instruction(inf,insn,memaddr);
            if(size<0)
                return size;
            disp += size;
            memaddr += size;
        }else{
            insn->num_ops = (insn->insn->pre_insert_operands)(iword,insn->ops,insn->num_ops);
            for(size_t n = 0;n<insn->insn->operand_count;n++){
                if((inf->read_memory_func)(inf->buffer_vma,ibuf,2,inf)<0)
                    return -1;
                disp += 2;
                memaddr += 2;
                iword = (((uint16_t)ibuf[0])<<8)|((uint16_t)ibuf[1]);
                long size = decode_operand(iword,memaddr, inf, &insn->ops[insn->num_ops++]);
                if(size<0)
                    return size;
                disp += size;
                memaddr += size;
            }
            insn->num_ops = (insn->insn->post_insert_operands)(iword,insn->ops,insn->num_ops);
            
        }
        
    }
    return disp;
}

static int print_reg_name(clever_reg reg, disassemble_info* info){
    if(reg<16)
        return (info->fprintf_func)(info->stream,"r%u",(unsigned)reg);
    else if(reg<32&&24<=reg)
        return (info->fprintf_func)(info->stream,"f%u",(unsigned)(reg-24));
    else if(reg<96&&64<=reg){
        return (info->fprintf_func)(info->stream,"v%u%c",(unsigned)(((reg-64)>>1)), (reg&1)?'h':'l');
    }else if(reg<136&&128<=reg)
        return (info->fprintf_func)(info->stream, "cr%u", (unsigned)(reg-128));
    else if(reg<142&&138<=reg)
        return (info->fprintf_func)(info->stream, "cpuex%u", (unsigned)(reg-136));
    else if(reg<148&&154<=reg)
        return (info->fprintf_func)(info->stream, "msr%u", (unsigned)(reg-148));
    else switch(reg){
    case ip:
        return (info->fprintf_func)(info->stream, "ip");
    case flags:
        return (info->fprintf_func)(info->stream, "flags");
    case mode:
        return (info->fprintf_func)(info->stream, "mode");
    case fpcw:
        return (info->fprintf_func)(info->stream, "fpcw");
    case mscpuex:
        return (info->fprintf_func)(info->stream, "mscpuex");
    case randinfo:
        return (info->fprintf_func)(info->stream, "randinfo");
    case cpuidlo:
        return (info->fprintf_func)(info->stream, "cpuidlo");
    case cpuidhi:
        return (info->fprintf_func)(info->stream, "cpuidhi");
    case reserved63:
        return (info->fprintf_func)(info->stream, "reserved");
    case reserved255:
        return (info->fprintf_func)(info->stream, "reserved");
    default:
        return (info->fprintf_func)(info->stream, "**INVALID REGISTER** %u", (unsigned)reg);
    }
}

static const char* const sizes_by_ss[] = {
    "byte",
    "single",
    "word",
    "double",
    "vector"
};

static int print_size(unsigned ss, disassemble_info* info){
    if(ss>(sizeof(sizes_by_ss)/sizeof(sizes_by_ss[0])))
        return (info->fprintf_func)(info->stream, "**INVALID SIZE %u**",(unsigned)ss);
    else
        return (info->fprintf_func)(info->stream, "%s",sizes_by_ss[ss]);
}

static int print_imm(const clever_immediate* imm, disassemble_info* info){
    static const char hexalpha[] = "0123456789abcdef";
    int total;
    int res;
    if(imm->size==0)
        total = (info->fprintf_func)(info->stream, "short");
    else
        total = print_size(imm->size,info);
    if(total<0)
        return total;
    if(imm->size==4){
        char buf[36] = {0};
        char* p = &buf[35];
        if (imm->value==0){
            *(--p) = '0';
        }
        uint64_t val = imm->value;
        while(val!=0){
            *(--p) = hexalpha[(val)&0xf];
            val >>= 4;
        }

        if(imm->value_hi!=0){
            val = imm->value_hi;
            while(p>&buf[16])
                *(--p) = '0';
            while(val!=0){
                *(--p) = hexalpha[(val)&0xf];
                val >>= 4;
            }
        }
        *(--p) = 'x';
        *(--p) = '0';
        *(--p) = ' ';
        res = (info->fprintf_func)(info->stream, "%s", p);
        if(res<0)
            return res;
        return total+res;
    }else{
        if(imm->symbol!=NULL){
            res = (info->fprintf_func)(info->stream, " %s", imm->symbol);
            if(res<0)
                return res;
            return total+res;
        }else{
            res = (info->fprintf_func)(info->stream, " %#" PRIx64, imm->value);
        }
    }
    return total;
}


static int print_operand(const clever_operand* op, disassemble_info* info){
    int total = 0;
    int res;
    switch(op->direct.kind){
    case direct:
        res = print_size(op->direct.size,info);
        if(res<0)
            return res;
        total+=res;
        if(op->direct.vec){
            if(op->direct.reg<64||op->direct.reg>96||(op->direct.reg&1)!=0)
                res = (info->fprintf_func)(info->stream," **INVALID VECTOR REGISTER**");
            else
                res = (info->fprintf_func)(info->stream,"v%u",(unsigned)(((op->direct.reg-64)>>1)));
        }else 
            res = print_reg_name(op->direct.reg,info);

        if(res<0)
            return res;
        total+=res;
        break;
    case indirect:
        res = print_size(op->direct.size,info);
        if(res<0)
            return res;
        total+=res;
        res = (info->fprintf_func)(info->stream,"[");
        if(res<0)
            return res;
        total+=res;
        if(op->indirect.index_kind_and_val!=0x10){
            if(op->indirect.scale!=1){
                res = (info->fprintf_func)(info->stream,"%u*",(unsigned)op->indirect.scale);
                if(res<0)
                    return res;
                total+=res;
            }
            if((op->indirect.index_kind_and_val)&0x10){
                res = (info->fprintf_func)(info->stream,"%u",(unsigned)(op->indirect.index_kind_and_val&0xf));
                if(res<0)
                    return res;
                total+=res;
            }else{
                res = print_reg_name(op->indirect.index_kind_and_val,info);
                if(res<0)
                    return res;
                total+=res;
            }
            res = (info->fprintf_func)(info->stream,"+");
            if(res<0)
                return res;
            total+=res;
        }
        res = print_reg_name(op->indirect.base,info);
        if(res<0)
            return res;
        total+=res;
        res = (info->fprintf_func)(info->stream,"]");
        if(res<0)
            return res;
        total+=res;
        break;
    case short_imm:
        res = print_imm(&op->short_imm.imm,info);
        if(res<0)
            return res;
        total+=res;
        if(&op->short_imm.rel){
            res = (info->fprintf_func)(info->stream,"+ip");
            if(res<0)
                return res;
            total+=res;
        }
        break;
    case long_imm:
        if(op->long_imm.mem){
            res = print_size(op->long_imm.mem_size,info);
            if(res<0)
                return res;
            total+=res;
            res = (info->fprintf_func)(info->stream,"[");
            if(res<0)
                return res;
            total+=res;
        }
        res = print_imm(&op->long_imm.imm,info);
        if(res<0)
            return res;
        total+=res;
        if(&op->short_imm.rel){
            res = (info->fprintf_func)(info->stream,"+ip");
            if(res<0)
                return res;
            total+=res;
        }
        if(op->long_imm.mem){
            res = (info->fprintf_func)(info->stream,"]");
            if(res<0)
                return res;
            total+=res;
        }
        break;
    }
    return total;
}

static int print_insn(const struct clever_instruction* insn, disassemble_info* info){
    char op_buff[MAX_MNEMONIC_LENGTH] = {0};
    char* p ;
    int total = 0;
    int res;
    
    if(insn->prefix_insn){
        strcpy(op_buff,insn->prefix_insn->mnemonic);
        p = op_buff+strlen(insn->insn->mnemonic);
        insn->insn->print_h(insn->opc,p);
        res = (info->fprintf_func)(info->stream,"%s ",op_buff);
        if(res<0)
            return res;
        total+=res;
    }

    strcpy(op_buff,insn->insn->mnemonic);
    p = op_buff+strlen(insn->insn->mnemonic);
    insn->insn->print_h(insn->opc,p);
    res = (info->fprintf_func)(info->stream,"%s ",op_buff);
    if(res<0)
        return res;
    total+=res;
    const char* sep = "";
    for(size_t n = 0;n<insn->num_ops;n++){
        res = (info->fprintf_func)(info->stream,"%s ",sep);
        if(res<0)
            return res;
        total+=res;
        sep = ", ";
        res = print_operand(&insn->ops[n],info);
        if(res<0)
            return res;
        total+=res;
    }
    return total;
}

int
print_insn_clever (bfd_vma memaddr, struct disassemble_info *info);

int
print_insn_clever (bfd_vma memaddr, struct disassemble_info *info)
{
    struct clever_instruction insn;
    long res = decode_instruction(info,&insn,memaddr);
    long tbytes = res;
    print_insn(&insn,info);
    return tbytes;
}

