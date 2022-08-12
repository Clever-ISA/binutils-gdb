
#include "as.h"
#include "safe-ctype.h"
#include "dwarf2dbg.h"
#include "opcode/clever.h"
#include "elf/clever.h"

#include <limits.h>


#define MAX_INSN_LEN 64


extern const struct clever_branch_info clever_ubranches[];
extern const struct clever_branch_info clever_sbranches[];

extern const struct clever_branch_info clever_cond_branch_info;

extern const struct clever_instruction_info clever_insns[];

/* A copy of the original instruction (used in error messages).  */
char ins_parse[MAX_INSN_LEN];

/* Round up a section size to the appropriate boundary.  */

valueT
md_section_align (segT seg, valueT val)
{
  /* Round .text sections to a multiple of 2.  */
  if (strncmp(seg->name,".text",5))
    return (val + 1) & ~1;
  return val;
}

enum{
    addr_size = 0,
    reg_size = 1,
    mem_size = 2,
    imm_size = 3,
};

// Default size for various kinds of operands w/o size specifiers
static int def_size[4] = {3,3,3,0};
// Bitfield of sizes (0-3) for register operands that gpr specializations should not be applied to on
static unsigned int no_spec_on;

// Whether or not gpr specializations are applied at all
static _Bool no_gpr_spec;

// Whether or not gpr destination (first operand) specializations are applied
static _Bool no_left_spec;
// Whether or not gpr source (second operand) specializations are applied
static _Bool no_right_spec;

// // Whether or not relaxations should be generated for symbols
// static _Bool allow_relax = 1;
// // Whether or not CLEVER_RELAX_SHORT or CLEVER_RELAX_SHORT_PCREL should be generated
// static _Bool use_short_relax = 1;
// // Whether or not CLEVER_RELAX_SHORT should be generated
// static _Bool use_short_relax_abs = 1;
// // Whether or not CLEVER_RELAX_SHORT_PCREL should be generated
// static _Bool use_short_relax_rel = 1;

static void relax_set(int val){
#ifdef md_flush_pending_output
  md_flush_pending_output ();
#endif

  SKIP_ALL_WHITESPACE();
  if(strncmp(input_line_pointer,"short",5)==0){
    input_line_pointer+=5;
    SKIP_ALL_WHITESPACE();
    if(strncmp(input_line_pointer,"rel",3)==0){
        input_line_pointer+=3;
        use_short_relax_rel = val;
    }else if(strncmp(input_line_pointer, "abs",3)==0){
        input_line_pointer+=3;
        use_short_relax_rel = val;
    }else{
        use_short_relax = val;
    }
  }else{
    allow_relax = val;
  }
  demand_empty_rest_of_line();
}

enum size_kind{
    size_any = 0,
    size_imm_only = 1,
    size_no_imm = 2,
    size_vector = 3,
};

struct size_names{
    const char* name;
    const char* bitwidth;
    enum size_kind kind;
    size_t ss;
    size_t size_bits;
    size_t size_bytes;
};

const struct size_names names[] = {
    {"byte","8",size_no_imm,0, 8,1},
    {"short","12",size_imm_only,0, 12,2},
    {"half","16", size_any,1, 16,2},
    {"single","32", size_any,2, 32,4},
    {"double","64", size_any, 3, 64, 8},
    {"vector", "128", size_vector, 4, 128, 16},
    {"quad", "128", size_vector, 4, 128, 16},
    {NULL,NULL,0,0,0,0}
};

static const struct size_names* parse_size(int allow_numeric){
    for(int i = 0;names[i].name;i++){
        if(strncmp(names[i].name,input_line_pointer,strlen(names[i].name))==0){
            input_line_pointer += strlen(names[i].name);
            if(*input_line_pointer!=' '&&*input_line_pointer!='['&&!is_it_end_of_statement()){
                input_line_pointer -= strlen(names[i].name);
                return NULL;
            }
            return &names[i];
        }else if(allow_numeric&&strncmp(names[i].bitwidth,input_line_pointer,strlen(names[i].bitwidth))==0){
            input_line_pointer += strlen(names[i].bitwidth);
            if(*input_line_pointer!=' '&&*input_line_pointer!='['&&!is_it_end_of_statement()){
                input_line_pointer -= strlen(names[i].bitwidth);
                return NULL;
            }
            return &names[i];
        }
    }
    return NULL;
}

static void set_def_size(int off){
    SKIP_ALL_WHITESPACE();
    const struct size_names* size = parse_size(1);
    if(!size)
        as_bad(_("Invalid size control %s"),input_line_pointer);
    if(off==imm_size&&size->kind==size_no_imm)
        as_bad(_("Cannot use size %s as default immediate size"),size->name);
    if(off!=imm_size&&size->kind==size_imm_only)
        as_bad(_("Cannot use size %s as default memory or register size"),size->name);
    demand_empty_rest_of_line();
}


static void set_spec(int val){
    SKIP_ALL_WHITESPACE();
    const struct size_names* size = parse_size(1);

    if(!size){
        if(strncmp(input_line_pointer,"left",4)==0){
            input_line_pointer+=4;
            no_left_spec = val;
        }else if(strncmp(input_line_pointer, "right",5)==0){
            input_line_pointer+=5;
            no_right_spec = val;
        }else{
            no_gpr_spec = val;
        }
    }else if(size->kind==size_imm_only){
        as_bad(_("Cannot set specialization behaviour for size %s"),size->name);
    }{
        no_spec_on = (no_spec_on&~(1<<(size->ss)))|(val<<(size->ss));
    }

    demand_empty_rest_of_line();
}

const pseudo_typeS md_pseudo_table[] =
{
  /* In CR16 machine, align is in bytes (not a ptwo boundary).  */
  {"align", s_align_bytes, 0},
  {"relax", relax_set, 1},
  {"norelax", relax_set, 0},
  {"addrsize",set_def_size, addr_size},
  {"regsize", set_def_size, reg_size},
  {"immsize", set_def_size, imm_size},
  {"memsize", set_def_size, mem_size},
  {"spec", set_spec, 0},
  {"nospec", set_spec, 1},
  {0, 0, 0}
};

static size_t operand_size(const union clever_operand* op){
    switch(op->direct.kind){
        case direct: return op->direct.size;
        case indirect: return op->indirect.size;
        case short_imm: return op->short_imm.imm.size;
        case long_imm: return op->long_imm.imm.size;
    }
}


static const struct clever_instruction_info* pick_specialization(struct clever_instruction* insn){
    if(no_gpr_spec)
        return NULL;
    else if(no_left_spec&&!insn->insn->gpr_src_spec)
        return NULL;
    else if (insn->num_ops==0)
        return NULL; // no operand to specialize on
    else if((insn->opc&0xf)!=0) // both `l` and `f` get unset when using a gpr-specialization
            return NULL;
    else{
        if(!no_left_spec&&insn->insn->gpr_dest_spec){
            if(insn->ops[0].direct.kind==direct){
                union clever_operand* op = &insn->ops[0];
                if(op->direct.reg<16){
                    // GPR Specialization candidate
                    size_t ss = (op->direct.size==8?0: op->direct.size==16?1: op->direct.size==32?2 : 3);
                    if(!(no_spec_on&(1<<ss))){
                        for(size_t i = 1;i<insn->num_ops;i++){
                            if(op->direct.size<operand_size(&insn->ops[i]))
                                goto no_left_spec;
                        }
                        return insn->insn->gpr_dest_spec;
                    }
                }
            }
        }
        no_left_spec:
        if(!no_right_spec&&insn->insn->gpr_src_spec){
            if(insn->ops[insn->num_ops-1].direct.kind==direct){
                union clever_operand* op = &insn->ops[0];
                if(op->direct.reg<16){
                    // GPR Specialization candidate
                    size_t ss = (op->direct.size==8?0: op->direct.size==16?1: op->direct.size==32?2 : 3);
                    if(!(no_spec_on&(1<<ss))){
                        for(size_t i = 1;i<insn->num_ops;i++){
                            if(op->direct.size<operand_size(&insn->ops[i]))
                                return NULL;
                        }
                        return insn->insn->gpr_src_spec;
                    }
                }
            }
        }
    }
    return NULL;
}


const char *md_shortopts = "";
int is_strict = 0;

enum{
    OPT_STRICT_MACHINE,
    OPT_SET_MACHINE,
    OPT_NO_EXTENSION,
    OPT_EXTENSIONS,
};

struct option md_longopts[] =
{
  {"strict-machine", no_argument,NULL,OPT_STRICT_MACHINE},
  {"machine", required_argument, NULL, OPT_SET_MACHINE},
  {"extension", required_argument, NULL, OPT_EXTENSIONS},
  {"no-extension", required_argument, NULL, OPT_NO_EXTENSION},
  {NULL, no_argument, NULL, 0}
};
size_t md_longopts_size = sizeof (md_longopts);

union clever_extensions enabled_extensions = CLEVER_ALL_EXTENSIONS;

static const struct {const char* name; enum clever_extension extension;} extension_name[] = {
    {"float",XFloat},
    {"vector", XVec},
    {"float-ext", XFloatExt},
    {"rand", XRand},
    {"virtualization", XVirtualization},
    {NULL, -1}
};

struct clever_machine_spec{
    const char* name;
    union clever_extensions available_extensions;
    int mach;
};

static const struct clever_machine_spec machines[] = {
    {"clever1.0",CLEVER1_0_ALL_EXTENSIONS,bfd_mach_clever1_0},
    {"clever",CLEVER_ALL_EXTENSIONS, bfd_mach_clever1_0},
    {NULL,{},0},
};

const struct clever_machine_spec* mach = &machines[1];

int is_strict;

int
md_parse_option (int c, const char *arg)
{
    switch(c){
    case OPT_STRICT_MACHINE:
        is_strict = 1;
        enabled_extensions = mach->available_extensions;
    break;
    case OPT_SET_MACHINE:
        for(const struct clever_machine_spec* m = machines;;m++){
            if(!m->name){
                as_bad(_("Invalid machine name %s"),arg);
                return 0;
            }
            if(strcasecmp(m->name,arg)==0){
                mach = m;
                break;
            }
        }
            
        if(is_strict)
            enabled_extensions = mach->available_extensions;
        return 1;
    case OPT_EXTENSIONS:
    case OPT_NO_EXTENSION:
        for(int i = 0;;i++){
            if(!extension_name[i].name){
                as_bad(_("Invalid extension name %s"),arg);
                return 0;
            }else if(strcasecmp(extension_name[i].name,arg)==0){
                size_t ext = extension_name[i].extension;
                enabled_extensions.flags[ext/(sizeof(unsigned int) * CHAR_BIT)] = (enabled_extensions.flags[ext/(sizeof(unsigned int) * CHAR_BIT)]& ~(1<<(ext%(sizeof(unsigned int) * CHAR_BIT)))) | ((c-OPT_NO_EXTENSION)<<(ext%(sizeof(unsigned int) * CHAR_BIT)));
            }
        }
        return 1;
    }
    return 0;
}

void
md_begin(void){}

const char *
md_atof (int type, char *litP, int *sizeP)
{
  return ieee_md_atof (type, litP, sizeP, target_little_endian);
}

static void modify_instruction(struct clever_instruction* insn){
    if(!no_gpr_spec){
        const struct clever_instruction_info* spec_insn = pick_specialization(insn);
        if(spec_insn){
            int spec_reg;
            insn->num_ops--;
            if(insn->insn->gpr_dest_spec==spec_insn){
                spec_reg = 
                for(size_t i = 0;i<insn->num_ops;i++)
                    insn->ops[i] = insn->ops[i+1];
            }
            insn->insn = spec_insn;
        }
    }
    if(insn->insn->operand_count==branch_addr){
        insn->num_ops =0;
    }else if(insn->num_ops>insn->insn->operand_count)
        insn->num_ops = insn->insn->operand_count;
}

static void print_operand(const union clever_operand* op){
    char* buf = frag_more(2);
    switch(op->direct.kind){
    case direct:
    {
        uint16_t ss = (op->direct.size==8?0:op->direct.size==16?1:op->direct.size==32?2:op->direct.size==64?3:4);
        uint16_t s = (op->direct.vec<<13)|(ss<<8)|(op->direct.reg);
        number_to_chars_bigendian(buf,s,2);
    }
        
    case indirect:
    {
        uint16_t ss = (op->indirect.size==8?0:op->indirect.size==16?1:op->indirect.size==32?2:3);
        uint16_t l = (op->indirect.scale==1?0:op->indirect.scale==2?1:op->indirect.scale==4?2:op->indirect.scale==8?3:op->indirect.scale==16?4:op->indirect.scale==32?5:op->indirect.scale==64?6:7);
        uint16_t s = 0x4000|(op->indirect.base)|(ss<<4)|((op->indirect.index_kind_and_val>>4)<<6)|(l<<7)|((op->indirect.index_kind_and_val&0xf)<<10);
        number_to_chars_bigendian(buf,s,2);
    }
    case short:
    {
        uint16_t simm = 0x8000|(op->short_imm.rel<<13)|((uint16_t)(op->short_imm.imm.value&0xfff));
        if(op->short_imm.imm.symbol){
            enum bfd_reloc_code_real code = op->short_imm.rel?BFD_RELOC_12_PCREL:BFD_RELOC_CLEVER_12;
            fragS * frag = frag_now;
            symbolS * s = symbol_find_or_make(op->short_imm.imm.symbol);
            fix_new(frag,frag_now_fix()-2,2, s,frag->fr_offset,op->short_imm.rel,code);
        }
        number_to_chars_bigendian(buf,simm,2);
    }
    case long:
    {
        addressT relax_fix = frag_now_fix()-2;
        addressT addr_fix = frag_now_fix();
        bool rel = op->long_imm.rel;
        uint16_t size = op->long_imm.imm.size;
        uint16_t ss = (size==16?0:size==32?1:size==64?2:3);
        uint16_t zize = op->long_imm.mem_size;
        uint16_t zz = (zize==8?0:zize==16?1:zize==32?2:zize==64?3:4);
        uint16_t ctrl = 0xC000 | (op->long_imm.mem<<13) | (op->long_imm.rel<<10) | (ss<<8) | (zz<<4);
        number_to_chars_bigendian(buf,ctrl,2);
        buf = frag_more(size/8);
        if(op->long_imm.imm.symbol){
            enum bfd_reloc_code_real code;
            if(size==16)
                code= op->long_imm.rel?BFD_RELOC_16_PCREL:BFD_RELOC_16;
            else if(size==32)
                code = op->long_imm.rel?BFD_RELOC_32_PCREL:BFD_RELOC_32;
            else{
                char* sym = strchr(op->long_imm.imm.symbol,'@');
                if(sym){
                    *sym = '\0';
                    sym++;
                    if(strcmp(sym,"PLT")){
                        code = op->long_imm.rel?BFD_RELOC_64_PLT_PCREL:BFD_RELOC_CLEVER_PLT64_NONREL;
                    }
                    else if(strcmp(sym,"GOT"))
                        code = op->long_imm.rel?BFD_RELOC_CLEVER_GOT64?BFD_RELOC_CLEVER_GOT64_NONREL;
                    else if(strcmp(sym,"DYNENT"))
                        code = BFD_RELOC_CLEVER_DYNENT;
                    else{
                        -1[sym] = '@';
                        code = op->long_imm.rel?BFD_RELOC_64_PCREL?BFD_RELOC_64;
                    }
                }
            }
            fragS * fragP = frag_now;
            symbolS * sym = symbol_find_or_make(long_imm->long_imm.imm.symbol);
            fix_new(fragP,addr_fix,size,sym,frag->fr_offset,op->long_imm.rel,code);
            // TODO: relaxations
            (void)use_short_relax;
            (void)use_short_relax_abs;
            (void)use_short_relax_rel;
            (void)allow_relax;
        }
        number_to_chars_littleendian(buf,op->long_imm.imm.value,size>64?8:size/8);
        if(op->long_imm.imm.size==128){
            number_to_chars_littleendian(buf+8,op->long_imm.imm.value_hi,8);
        }
    }
    default: break;
    }
}

static void print_instruction(const struct clever_instruction* insn){
    if(insn->prefix_insn){
        char* buf = frag_more(2);
        number_to_chars_bigendian(buf,insn->prefix_opc,2);
    }

    char* buf = frag_more(2);
    number_to_chars_bigendian(buf,insn->opc,2);
    for(size_t n = 0;n<insn->num_ops)
        print_operand(&n[insn->ops]);
}

struct reg_name{
    enum clever_register reg;
    const char* name;
};

static const struct reg_name regs[] = {
#define CLEVER_REG_NAME(reg) {reg, #reg}
#define CLEVER_VEC_NAMES(reg) {reg##l,#reg "l"}, {reg##h,#reg "h"}, {reg##l, #reg}
    CLEVER_REG_NAME(r0),
    CLEVER_REG_NAME(r1),
    CLEVER_REG_NAME(r2),
    CLEVER_REG_NAME(r3),
    CLEVER_REG_NAME(r4),
    CLEVER_REG_NAME(r5),
    CLEVER_REG_NAME(r6),
    CLEVER_REG_NAME(r7),
    CLEVER_REG_NAME(r8),
    CLEVER_REG_NAME(r9),
    CLEVER_REG_NAME(r10),
    CLEVER_REG_NAME(r11),
    CLEVER_REG_NAME(r12),
    CLEVER_REG_NAME(r13),
    CLEVER_REG_NAME(r14),
    CLEVER_REG_NAME(r15),
    CLEVER_REG_NAME(ip),
    CLEVER_REG_NAME(flags),
    CLEVER_REG_NAME(mode),
    CLEVER_REG_NAME(fpcw),
    CLEVER_REG_NAME(f0),
    CLEVER_REG_NAME(f1),
    CLEVER_REG_NAME(f2),
    CLEVER_REG_NAME(f3),
    CLEVER_REG_NAME(f4),
    CLEVER_REG_NAME(f5),
    CLEVER_REG_NAME(f6),
    CLEVER_REG_NAME(f7),
    CLEVER_VEC_NAMES(v0),
    CLEVER_VEC_NAMES(v1),
    CLEVER_VEC_NAMES(v2),
    CLEVER_VEC_NAMES(v3),
    CLEVER_VEC_NAMES(v4),
    CLEVER_VEC_NAMES(v5),
    CLEVER_VEC_NAMES(v6),
    CLEVER_VEC_NAMES(v7),
    CLEVER_VEC_NAMES(v8),
    CLEVER_VEC_NAMES(v9),
    CLEVER_VEC_NAMES(v10),
    CLEVER_VEC_NAMES(v11),
    CLEVER_VEC_NAMES(v12),
    CLEVER_VEC_NAMES(v13),
    CLEVER_VEC_NAMES(v14),
    CLEVER_VEC_NAMES(v15),
    CLEVER_REG_NAME(cr0),
    CLEVER_REG_NAME(cr1),
    CLEVER_REG_NAME(cr2),
    CLEVER_REG_NAME(cr3),
    CLEVER_REG_NAME(cr4),
    CLEVER_REG_NAME(cr5),
    CLEVER_REG_NAME(cr6),
    CLEVER_REG_NAME(cr7),
    CLEVER_REG_NAME(cpuidhi),
    CLEVER_REG_NAME(cpuidlo),
    CLEVER_REG_NAME(cpuex2),
    CLEVER_REG_NAME(cpuex3),
    CLEVER_REG_NAME(cpuex4),
    CLEVER_REG_NAME(cpuex5),
    CLEVER_REG_NAME(cpuex6),
    CLEVER_REG_NAME(mscpuex),
    CLEVER_REG_NAME(msr0),
    CLEVER_REG_NAME(msr1),
    CLEVER_REG_NAME(msr2),
    CLEVER_REG_NAME(msr3),
    CLEVER_REG_NAME(msr4),
    CLEVER_REG_NAME(msr5),
    CLEVER_REG_NAME(msr6),
    {"und",reserved255}
#undef CLEVER_VEC_NAMES
#undef CLEVER_REG_NAME
}

static void expr_to_operand(expressionS *expr, union clever_operand* op, _Bool imm_signed, _Bool is_mem, _Bool is_addr, const struct size_names* size){
    _Bool is_short = size&&size->size_bits==12&&!is_mem;
    switch(expr->X_op){
    case O_constant:
        if(is_short){
            op->short_imm.kind = short_imm;
            op->short_imm.imm.size = 12;
            op->short_imm.imm.value = expr->X_add_number&0xfff;
            op->short_imm.imm.symbol = NULL;
        }else if(size){
            op->long_imm.kind = long_imm;
            op->long_imm.imm.size = size->size_bits;
            op->long_imm.imm.value = expr->X_add_number;
            op->long_imm.imm.symbol = NULL;
        }else{
            size_t sizebits;
            if(imm_signed){
                if(!is_mem&&expr->X_add_number>=-0x800&&expr->X_add_number<=0x7ff)
                    sizebits = 12;
                else if(expr->X_add_number>=-0x8000&&expr->X_add_number<=0x7fff)
                    sizebits = 16;
                else if(expr->X_addr_number>=-0x80000000&&expr->X_add_number<=0x7fffffff)
                    sizebits = 32;
                else
                    sizebits = 64;
            }else{
                if(expr->X_add_number<0)
                    sizebits = 64;
                else if(!is_mem&&expr->X_add_number<0xfff)
                    sizebits = 12;
                else if(expr->X_add_number<0xffff)
                    sizebits = 16;
                else if(expr->X_add_number<0xffffffff)
                    sizebits = 32;
                else
                    sizebits =64;
            }

            if(sizebits==12){
                op->short_imm.kind = short_imm;
                op->short_imm.imm.size = 12;
                op->short_imm.imm.value = expr->X_add_number&0xfff;
                op->short_imm.imm.symbol = NULL;
            }else{
                op->long_imm.kind = long_imm;
                op->long_imm.imm.size = size->size_bits;
                op->long_imm.imm.value = expr->X_add_number;
                op->long_imm.imm.symbol = NULL;
            }
        }
    case O_symbol:
        {
            if(!is_addr){

            }
        }
    }
}

static int parse_operand(union clever_operand* op, _Bool imm_signed){
    const struct size_names* size = parse_size(0);
    SKIP_ALL_WHITESPACE();
    _Bool rel_prefix = false;
    _Bool is_addr = false;
    _Bool is_mem = false;
    const struct size_names* zize = NULL;
    if(*input_line_pointer==','||is_it_end_of_statement()){
        if(!size){
            as_bad(_("Empty Operand in instruction %s"),ins_parse)
        }
        op->size_only.kind = size_only;
        op->size_only.size = size->size_bits;
        return 1;
    }
    if(*input_line_pointer=='['){
        is_mem = true;

        char* p = input_line_pointer++;
        while(*p++!=']'){
            if(is_it_end_of_statement()){
                as_bad(_("Unterminated [ group in instruction %s"),ins_parse);
                return 0;
            }
        }
        zize = size;
        size = parse_size(0);
        SKIP_ALL_WHITESPACE();
        *p = '\0';
    }

    if(strncasecmp(input_line_pointer,"rel",3)){
        input_line_pointer+=3;
        if(!isspace(*input_line_pointer))
            input_line_pointer-=3;
        else{
            SKIP_ALL_WHITESPACE();
            rel_prefix = true;
            is_addr = true;
        }
    }else if(strncasecmp(input_line_pointer, "abs",3)){
        input_line_pointer+=3;
        if(!isspace(*input_line_pointer))
            input_line_pointer-=3;
        else{
            is_addr = true;
            rel_prefix = false;
            SKIP_ALL_WHITESPACE();
        }
    }
    expressionS expr;
    expression(&expr);
    SKIP_ALL_WHITESPACE();
    expr_to_operand(&expr,op,imm_signed,is_mem,is_addr,size);
    if(is_mem){
        size_t ss = zize?zize->ss:def_size[mem_size];
        switch(op->direct.kind){
            case long_imm:
                op->long_imm.mem = true;
                op->long_imm.mem_size = 8<<ss;
            default:
        }
        if(!*input_line_pointer)
            *input_line_pointer=']';
        else{
            as_bad(_("Unexpected trailing tokens in [ group: %s"),ins_parse);
            return 0;
        }
        input_line_pointer++;
        SKIP_ALL_WHITESPACE();
    }

    if(*input_line_pointer!=','&&!is_it_end_of_statement()){
        as_bad(_("Unexpected trailing tokens in operand: %s"),ins_parse);
        return 0;
    }
    return 1;
}



static int parse_instruction(struct clever_instruction* insn){
    SKIP_ALL_WHITESPACE();
    char* p = input_line_pointer;

    for(int i = 0;clever_ubranches[i].insn;i++){
        if(strncmp(p,clever_ubranches[i].insn->mnemonic,strlen(clever_ubranches[i].insn->mnemonic))==0){
            p += strlen(clever_ubranches[i].insn->mnemonic);
            insn->insn = clever_ubranches[i].insn;
            insn->opc = clever_ubranches[i].insn->opcode<<4;
        }
    }

    if(!insn->insn){
        for(int i = 0;clever_sbranches[i].insn;i++){
            if(strncmp(p,clever_sbranches[i].insn->mnemonic,strlen(clever_sbranches[i].insn->mnemonic))==0){
                p += strlen(clever_ubranches[i].insn->mnemonic);
                insn->insn = clever_ubranches[i].insn;
                insn->opc = clever_ubranches[i].insn->opcode<<4;
            }
        }
    }

    if(!insn->insn){
        if(*p=='j'){
            p += 1;
            insn->insn = clever_cond_branch_info.insn;
            insn->opc = clever_cond_branch_info.insn->opcode <<4;
        }else{
            for(int i = 0;clever_insns[i].mnemonic;i++){
                if(strncmp(p,clever_insns[i].mnemonic,strlen(clever_insns[i].mnemonic))==0){
                    p += strlen(clever_insns[i].mnemonic);
                    insn->insn = &clever_insns[i];
                    insn->opc = clever_insns[i].opcode<<4;
                }
            }
        }
    }


    while(!isspace(*input_line_pointer)&&!is_it_end_of_statement())
        input_line_pointer++;

    if(!insn->insn){
        *input_line_pointer = '\0';
        as_bad(_("Unrecognized instruction `%s'"),p);
        return -1;
    }
    SKIP_ALL_WHITESPACE();
    

    if(insn->insn->operand_count==prefix){
        const struct clever_instruction_info* prefix = insn->insn;
        int i = parse_instruction(insn);

        if(i<0)
            return i;

        if(insn->prefix_insn){
            as_bad(_("Cannot combine prefixes %s and %s"),prefix->mnemonic,insn->prefix_insn->mnemonic);
            return -1;
        }

        insn->prefix_insn = prefix;
        insn->prefix_opc = prefix->opcode;

        p = (char*) (prefix->parse_h)(&insn->prefix_opc,p,insn);

        if(!isspace(*p)){
            as_bad(_("Leftover characters after reading instruction suffix"));
            return -1;
        }
    }else
    return 0;
}