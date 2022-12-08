# source_sh ${srcdir}/emulparams/plt_unwind.sh
# source_sh ${srcdir}/emulparams/dynamic_undefined_weak.sh
# source_sh ${srcdir}/emulparams/extern_protected_data.sh
# source_sh ${srcdir}/emulparams/static.sh
SCRIPT_NAME=elf
ELFSIZE=64
OUTPUT_FORMAT="elf64-clever"
NO_REL_RELOCS=yes
TEXT_START_ADDR=0x400000
MAXPAGESIZE="CONSTANT (MAXPAGESIZE)"
COMMONPAGESIZE="CONSTANT (COMMONPAGESIZE)"
ARCH="clever"
MACHINE=
TEMPLATE_NAME=elf
GENERATE_SHLIB_SCRIPT=yes
GENERATE_PIE_SCRIPT=yes