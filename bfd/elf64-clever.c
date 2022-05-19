
#include "sysdep.h"
#include "bfd.h"
#include "libbfd.h"
#include "elf-bfd.h"
#include "dwarf2.h"
#include "libiberty.h"

#include "elf/clever.h"


#include "elf64-clever.h"

#ifdef CORE_HEADER
#include <stdarg.h>
#include CORE_HEADER
#endif

/* In case we're on a 32-bit machine, construct a 64-bit "-1" value.  */
#define MINUS_ONE (~ (bfd_vma) 0)

/* The relocation "howto" table.  Order of fields:
   type, rightshift, size, bitsize, pc_relative, bitpos, complain_on_overflow,
   special_function, name, partial_inplace, src_mask, dst_mask, pcrel_offset.  */
static reloc_howto_type clever_elf_howto_table[] = {
   HOWTO(R_CLEVER_NONE, 0, 3, 0, false, 0, complain_overflow_dont,
	bfd_elf_generic_reloc, "R_CLEVER_NONE",	false, 0, 0x00000000,
	false),
   HOWTO(R_CLEVER_16,0,1,16,false,0,complain_overflow_unsigned, bfd_elf_generic_reloc, "R_CLEVER_16",false,0, 0xffff, false),
   HOWTO(R_CLEVER_32,0,2,32,false,0,complain_overflow_unsigned, bfd_elf_generic_reloc, "R_CLEVER_32",false,0,0xffffffff,false),
   HOWTO(R_CLEVER_64,0,3,64,false,0,complain_overflow_unsigned, bfd_elf_generic_reloc,"R_CLEVER_64",false,0, MINUS_ONE,false),
   HOWTO(R_CLEVER_NONE, 0, 3, 0, false, 0, complain_overflow_dont,
	bfd_elf_generic_reloc, "R_CLEVER_NONE",	false, 0, 0x00000000,
	false),
   HOWTO(R_CLEVER_16_PCREL,0,1,16,true,0,complain_overflow_signed,bfd_elf_generic_reloc, "R_CLEVER_16_PCREL",false,0,0xffff,true),
   HOWTO(R_CLEVER_32_PCREL,0,2,32,true,0,complain_overflow_signed,bfd_elf_generic_reloc, "R_CLEVER_32_PCREL",false,0,0xffffffff,true),
   HOWTO(R_CLEVER_64_PCREL,0,3,64,true,0,complain_overflow_signed, bfd_elf_generic_reloc, "R_CLEVER_64_PCREL",false,0,MINUS_ONE,true),
   HOWTO(R_CLEVER_SIMM, 0,1,12,false,0,complain_overflow_unsigned,bfd_elf_generic_reloc, "R_CLEVER_SIMM",true, 0,0xfff,false),
   HOWTO(R_CLEVER_SIMM_PCREL,0,1,12,true,0,complain_overflow_signed,bfd_elf_generic_reloc,"R_CLEVER_SIMM_PCREL",true,0,0xff,true),
   HOWTO(R_CLEVER_RELAX_LONG,0,0,0,false,0,complain_overflow_dont, bfd_elf_generic_reloc, "R_CLEVER_RELAX_LONG", true,0,0,false),
   HOWTO(R_CLEVER_RELAX_LONG_PCREL,0,0,0,false,0,complain_overflow_dont, bfd_elf_generic_reloc, "R_CLEVER_RELAX_LONG_PCREL", true,0,0,false),
   HOWTO(R_CLEVER_RELAX_SHORT,0,0,0,false,0,complain_overflow_dont, bfd_elf_generic_reloc, "R_CLEVER_RELAX_LONG", true,0,0,false),
   HOWTO(R_CLEVER_RELAX_SHORT_PCREL,0,0,0,false,0,complain_overflow_dont, bfd_elf_generic_reloc, "R_CLEVER_RELAX_LONG_PCREL", true,0,0,false),
   HOWTO(R_CLEVER_NONE, 0, 3, 0, false, 0, complain_overflow_dont,
	bfd_elf_generic_reloc, "R_CLEVER_NONE",	false, 0, 0x00000000,
	false),
	HOWTO(R_CLEVER_NONE, 0, 3, 0, false, 0, complain_overflow_dont,
	bfd_elf_generic_reloc, "R_CLEVER_NONE",	false, 0, 0x00000000,
	false),
   HOWTO(R_CLEVER_GOT,0,3,64,false,0,complain_overflow_unsigned, bfd_elf_generic_reloc, "R_CLEVER_GOT", false, 0,MINUS_ONE, false),
   HOWTO(R_CLEVER_GOT_PCREL, 0,3,64,true,0,complain_overflow_signed, bfd_elf_generic_reloc, "R_CLEVER_GOT_PCREL", false, 0, MINUS_ONE, true),
   HOWTO(R_CLEVER_PLT, 0,3,64,false,0,complain_overflow_unsigned,bfd_elf_generic_reloc, "R_CLEVER_PLT", false, 0, MINUS_ONE, false),
   HOWTO(R_CLEVER_PLT_PCREL, 0, 3, 64, true, 0, complain_overflow_signed,bfd_elf_generic_reloc, "R_CLEVER_PLT_PCREL", false, 0, MINUS_ONE, true),
   HOWTO(R_CLEVER_DYNENT, 0, 3, 64, false, 0, complain_overflow_dont, bfd_elf_generic_reloc, "R_CLEVER_DYNENT", false, 0, MINUS_ONE, false)
};

struct elf_reloc_map
{
  bfd_reloc_code_real_type bfd_reloc_val;
  unsigned char elf_reloc_val;
};

static const struct elf_reloc_map clever_reloc_map[] =
{
	{BFD_RELOC_NONE, R_CLEVER_NONE},
	{BFD_RELOC_16, R_CLEVER_16},
	{BFD_RELOC_32, R_CLEVER_32},
	{BFD_RELOC_64, R_CLEVER_64},
	{BFD_RELOC_12_PCREL, R_CLEVER_SIMM_PCREL},
	{BFD_RELOC_16_PCREL, R_CLEVER_16_PCREL},
	{BFD_RELOC_32_PCREL, R_CLEVER_32_PCREL},
	{BFD_RELOC_64_PCREL, R_CLEVER_64_PCREL},
	{BFD_RELOC_64_PLT_PCREL, R_CLEVER_PLT_PCREL},
	{BFD_RELOC_CLEVER_GOT64, R_CLEVER_GOT_PCREL},
	{BFD_RELOC_CLEVER_GOT64_NONREL, R_CLEVER_GOT},
	{BFD_RELOC_CLEVER_PLT64_NONREL, R_CLEVER_PLT},
	{BFD_RELOC_CLEVER_12, R_CLEVER_SIMM},
	{0,0}
};

static reloc_howto_type *
elf_clever_rtype_to_howto (__attribute_maybe_unused__ bfd *abfd, unsigned r_type)
{
	if (r_type>=R_CLEVER_end){
		_bfd_error_handler (_("%pB: unsupported relocation type %#x"),
			      abfd, r_type);
	  bfd_set_error (bfd_error_bad_value);
	  return NULL;
	}else if(r_type!=clever_elf_howto_table[r_type].type){
		_bfd_error_handler (_("%pB: unsupported relocation type %#x"),
			      abfd, r_type);
	  bfd_set_error (bfd_error_bad_value);
	  return NULL;
	}else{
		return &clever_elf_howto_table[r_type];
	}
}

static reloc_howto_type *
elf_clever_reloc_type_lookup (__attribute_maybe_unused__ bfd *abfd,
			      bfd_reloc_code_real_type code)
{
  unsigned int i;

  for (i = 0; i < sizeof (clever_reloc_map) / sizeof (struct elf_reloc_map);
       i++)
    {
      if (clever_reloc_map[i].bfd_reloc_val == code)
	return elf_clever_rtype_to_howto (abfd,
					  clever_reloc_map[i].elf_reloc_val);
    }
  return NULL;
}

static reloc_howto_type *
elf_clever_reloc_name_lookup (__attribute_maybe_unused__ bfd *abfd,
			      const char *r_name)
{
  unsigned int i;
  for (i = 0; i < ARRAY_SIZE (clever_elf_howto_table); i++)
    if (clever_elf_howto_table[i].name != NULL
	&& strcasecmp (clever_elf_howto_table[i].name, r_name) == 0)
      return &clever_elf_howto_table[i];

  return NULL;
}

static bool
elf_clever_info_to_howto (bfd *abfd, arelent *cache_ptr,
			  Elf_Internal_Rela *dst)
{
  unsigned r_type;

  r_type = ELF64_R_TYPE (dst->r_info);
  cache_ptr->howto = elf_clever_rtype_to_howto (abfd, r_type);
  if (cache_ptr->howto == NULL)
    return false;
  BFD_ASSERT (r_type == cache_ptr->howto->type || cache_ptr->howto->type == R_CLEVER_NONE);
  return true;
}

// /* Functions for the clever ELF linker.	 */

// /* The size in bytes of an entry in the global offset table.  */

// #define GOT_ENTRY_SIZE 8

// /* The size in bytes of an entry in the lazy procedure linkage table.  */

// #define LAZY_PLT_ENTRY_SIZE 32


// /*
// 	01 40 e6 30 00 00 00 00 00 00 00 00 // R_CLEVER_64_PCREL(_GLOBAL_OFFSET_TABLE+16)
//     00 aa e6 30 00 00 00 00 00 00 00 00 // R_CLEVER_64_PCREL(_GLOBAL_OFFSET_TABLE_+8)
//     7c 9a
// 	01 10 c0 00 00 00
// */

// /*
// 	00 aa e6 30 00 00 00 00 00 00 00 00 // R_CLEVER_GOT_PCREL(<symbol>)
//     7c 9a 
//     01 40 c4 00 00 00 00 00 00 00 00 00 // R_CLEVER_DYNENT(<symbol>)
//     7d 01 00 00 00 00 // R_CLEVER_32_PCREL(.plt[0])
// */

// static const bfd_byte elf_clever_plt0_entry[LAZY_PLT_ENTRY_SIZE] = {
// 	0x01, 0x40, 0xe6, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
// 	0x00, 0xaa, 0xe6, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
// 	0x7c, 0x9a,
// 	0x01, 0x10, 0xc0, 0x00, 0x00, 0x00,
// };

// static const bfd_byte elf_clever_plt_entry[LAZY_PLT_ENTRY_SIZE] = {
// 	0x00, 0xaa, 0xe6, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
// 	0x7c, 0x9a,
// 	0x01, 0x40, 0xc4, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
// 	0x7d, 0x01, 0x00, 0x00, 0x00, 0x00
// };

// static const bfd_byte elf_clever_plt_eh_frame[] = {0, 0, 0, 0};

// static const struct elf_clever_lazy_plt_layout elf_clever_plt = {
// 	elf_clever_plt0_entry,
// 	LAZY_PLT_ENTRY_SIZE,
// 	elf_clever_plt_entry,
// 	LAZY_PLT_ENTRY_SIZE,
	
// 	16,
// 	4,
// 	12,

// 	4,
// 	18,
// 	28,

// 	12,
// 	32,
// 	14,

// 	elf_clever_plt_eh_frame,
// 	sizeof(elf_clever_plt_eh_frame)
// };


#define TARGET_LITTLE_SYM		    clever_elf64_vec
#define TARGET_LITTLE_NAME		    "elf64-clever"
#define ELF_ARCH			    bfd_arch_clever
#define ELF_TARGET_ID			    GENERIC_ELF_DATA
#define ELF_MACHINE_CODE		    EM_CLEVER
#if DEFAULT_LD_Z_SEPARATE_CODE
# define ELF_MAXPAGESIZE		    0x1000
#else
# define ELF_MAXPAGESIZE		    0x200000
#endif
#define ELF_MINPAGESIZE			    0x1000
#define ELF_COMMONPAGESIZE		    0x1000

// #define elf_backend_can_gc_sections	    1
// #define elf_backend_can_refcount	    1
// #define elf_backend_want_got_plt	    1
// #define elf_backend_plt_readonly	    1
// #define elf_backend_want_plt_sym	    0
// #define elf_backend_got_header_size	    (GOT_ENTRY_SIZE*3)
// #define elf_backend_rela_normal		    1
// #define elf_backend_plt_alignment	    4
// #define elf_backend_extern_protected_data   1
// #define elf_backend_caches_rawsize	    1
// #define elf_backend_dtrel_excludes_plt	    1
// #define elf_backend_want_dynrelro	    1

#define elf_info_to_howto		    elf_clever_info_to_howto

#define bfd_elf64_bfd_reloc_type_lookup	    elf_clever_reloc_type_lookup
#define bfd_elf64_bfd_reloc_name_lookup \
  elf_clever_reloc_name_lookup



#undef	elf64_bed
#define elf64_bed elf64_clever_bed

#include "elf64-target.h"