/* Clever-ISA ELF support for BFD.

   Copyright (C) 2021 Free Software Foundation, Inc.
   Contributed by Connor Horman

   This file is part of GNU Binutils.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the license, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING3. If not,
   see <http://www.gnu.org/licenses/>.  */

#ifndef _ELF_CLEVER_H
#define _ELF_CLEVER_H

#include "elf/reloc-macros.h"

START_RELOC_NUMBERS(elf_clever_reloc_type)

RELOC_NUMBER(R_CLEVER_NONE,0)

RELOC_NUMBER(R_CLEVER_16,1)
RELOC_NUMBER(R_CLEVER_32,2)
RELOC_NUMBER(R_CLEVER_64,3)
RELOC_NUMBER(R_CLEVER_16_PCREL,5)
RELOC_NUMBER(R_CLEVER_32_PCREL,6)
RELOC_NUMBER(R_CLEVER_64_PCREL,7)
RELOC_NUMBER(R_CLEVER_SIMM,8)
RELOC_NUMBER(R_CLEVER_SIMM_PCREL,9)
RELOC_NUMBER(R_CLEVER_RELAX_LONG,10)
RELOC_NUMBER(R_CLEVER_RELAX_LONG_PCREL,11)
RELOC_NUMBER(R_CLEVER_RELAX_SHORT,12)
RELOC_NUMBER(R_CLEVER_RELAX_SHORT_PCREL,13)
RELOC_NUMBER(R_CLEVER_GOT,16)
RELOC_NUMBER(R_CLEVER_GOT_PCREL,17)
RELOC_NUMBER(R_CLEVER_PLT, 18)
RELOC_NUMBER(R_CLEVER_PLT_PCREL, 19)
RELOC_NUMBER(R_CLEVER_RELAX_GOT,20)
RELOC_NUMBER(R_CLEVER_RELAX_GOT_PCREL,21)
RELOC_NUMBER(R_CLEVER_RELAX_PLT,22)
RELOC_NUMBER(R_CLEVER_RELAX_PLT_PCREL,23)
RELOC_NUMBER(R_CLEVER_DYNENT,24)

END_RELOC_NUMBERS(R_CLEVER_end)

#endif /* _ELF_CLEVER_H */