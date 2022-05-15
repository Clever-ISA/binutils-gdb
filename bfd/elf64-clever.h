#ifndef _ELF64_CLEVER_H
#define _ELF64_CLEVER_H

#include <bfd.h>

struct elf_clever_lazy_plt_layout
{
  /* The first entry in a lazy procedure linkage table looks like this.  */
  const bfd_byte *plt0_entry;
  unsigned int plt0_entry_size;		 /* Size of PLT0 entry.  */

  /* Later entries in a lazy procedure linkage table look like this.  */
  const bfd_byte *plt_entry;
  unsigned int plt_entry_size;		/* Size of each PLT entry.  */

  /* Offsets into plt0_entry that are to be replaced with GOT[1] and
     GOT[2].  */
  unsigned int plt0_got1_offset;
  unsigned int plt0_got2_offset;

  /* Offset of the end of the PC-relative instruction containing
     plt0_got2_offset.  This is for x86-64 only.  */
  unsigned int plt0_got2_insn_end;

  /* Offsets into plt_entry that are to be replaced with...  */
  unsigned int plt_got_offset;    /* ... address of this symbol in .got. */
  unsigned int plt_reloc_offset;  /* ... offset into relocation table. */
  unsigned int plt_plt_offset;    /* ... offset to start of .plt. */

  /* Length of the PC-relative instruction containing plt_got_offset. */
  unsigned int plt_got_insn_size;

  /* Offset of the end of the PC-relative jump to plt0_entry.   */
  unsigned int plt_plt_insn_end;

  /* Offset into plt_entry where the initial value of the GOT entry
     points.  */
  unsigned int plt_lazy_offset;

  /* .eh_frame covering the lazy .plt section.  */
  const bfd_byte *eh_frame_plt;
  unsigned int eh_frame_plt_size;
};

#endif