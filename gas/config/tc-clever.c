
#include "as.h"
#include "safe-ctype.h"
#include "dwarf2dbg.h"
#include "opcode/clever.h"
#include "elf/clever.h"

#include <limits.h>

const char *md_shortopts = "";
int is_strict = 0;
struct option md_longopts[] =
{
  {"strict-machine", optional_argument,&is_strict,1},
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

extern const struct clever_branch_info clever_ubranches[];
extern const struct clever_branch_info clever_sbranches[];

extern const struct clever_branch_info clever_cond_branch_info;

extern const struct clever_instruction_info clever_insns[];

/* Primary Opcode (non-specialization) hash table.  */
static htab_t clever_primary_inst_hash;

/* A copy of the original instruction (used in error messages).  */
char ins_parse[MAX_INST_LEN];

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
}

// Default size for various kinds of operands w/o size specifiers
static int def_size[3] = {3,3,3,0};
// Bitfield of sizes (0-3) for register operands that gpr specializations should not be applied to on
static unsigned int no_spec_on;

// Whether or not gpr destination (first operand) specializations are applied
static _Bool no_left_spec;
// Whether or not gpr source (second operand) specializations are applied
static _Bool no_right_spec;

// Whether or not relaxations should be generated for symbols
static _Bool allow_relax = 1;
// Whether or not CLEVER_RELAX_SHORT or CLEVER_RELAX_SHORT_PCREL should be generated
static _Bool use_short_relax = 1;
// Whether or not CLEVER_RELAX_SHORT should be generated
static _Bool use_short_relax_abs = 1;
// Whether or not CLEVER_RELAX_SHORT_PCREL should be generated
static _Bool use_short_relax_rel = 1;

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
}

struct size_names{
    const char* name;
    const char* bitwidth;
    enum size_kind kind;
    size_t ss;
    size_t size_bytes;
}

const struct size_names names[] = {
    {"byte","8",size_no_imm,0,1},
    {"short","12",size_imm_only,0,2},
    {"half","16", size_any,1,2},
    {"single","32", size_any,2,4},
    {"double","64", size_any, 3, 8},
    {"vector", "128", size_vector, 4, 16},
    {"quad", "128", size_vector, 4, 16},
    {NULL,NULL,0,0,0}
};

const struct size_name* parse_size(int allow_numeric){
    for(int i = 0;names[i].name;i++){
        if(strncmp(names[i].name,input_line_pointer,strlen(names[i].name))==0){
            input_line_pointer += strlen(names[i].name);
            if(*input_line_pointer!=' '&&*input_line_pointer!='['&&!is_it_end_of_statement()){
                input_line_pointer -= strlen(names[i].name);
                return NULL;
            }
            return &names[i];
        }else if(allow_numeric&&strncmp(names[i].bitwidth,input_line_pointer,strlen(names[i].bitwidth))==0){
            input_line_pointer += strlen(names[i].bitwdith);
            if(*input_line_pointer!=' '&&*input_line_pointer!='['&&!is_it_end_of_statement()){
                input_line_pointer -= strlen(names[i].bitwdith);
                return NULL;
            }
            return &names[i];
        }
    }
    return NULL;
}

static void set_def_size(int off){
    SKIP_ALL_WHITESPACE();
    const struct size_name* size = parse_size(1);
    if(!size)
        as_bad(_("Invalid size control %s"),input_line_pointer);
    if(off==imm_size&&size->kind==size_no_imm)
        as_bad(_("Cannot use size %s as default immediate size"),size->name);
    if(off!=imm_size&&size->kind==size_imm_only)
        as_bad(_("Cannot use size %s as default immediate size"),size->name);
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
  {"spec", set_spec, 1},
  {"nospec", set_spec, 0},
  {0, 0, 0}
};

