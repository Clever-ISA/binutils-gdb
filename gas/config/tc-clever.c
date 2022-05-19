
#include "as.h"
#include "safe-ctype.h"
#include "dwarf2dbg.h"
#include "opcode/clever.h"
#include "elf/clever.h"
#include "clever-opc.h"

#include <limits.h>

const char *md_shortopts = "";
int is_strict = 0;
struct option md_longopts[] =
{
  {"strict-machine", optional_argument,&is_strict,}
  {NULL, no_argument, NULL, 0}
};


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
