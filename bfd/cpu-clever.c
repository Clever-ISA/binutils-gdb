
#include "sysdep.h"
#include "bfd.h"
#include "libbfd.h"
#include "libiberty.h"


static const bfd_arch_info_type *
compatible (const bfd_arch_info_type *a, const bfd_arch_info_type *b)
{
  /* If a & b are for different architecture we can do nothing.  */
  if (a->arch != b->arch)
      return NULL;

  /* If a & b are for the same machine then all is well.  */
  if (a->mach == b->mach)
    return a;

  /* Otherwise if either a or b is the 'default' machine
     then it can be polymorphed into the other.  */
  if (a->the_default)
    return b;

  if (b->the_default)
    return a;

  /* the mach refers to the Clever-ISA specfication version - as all versions are forward compatible, we can select the best one  */
  if (a->mach < b->mach)
    return b;
  else if (a->mach > b->mach)
    return a;

  /* Never reached!  */
  return NULL;
}

static void *
bfd_arch_clever_fill (bfd_size_type count, bool code)
{
    // 2 bytes
    static const char nop0[] = {0x01, 0x00}; // `nop`

    // 4 bytes
    static const char nop1[] = {0x01, 0x10, 0x00, 0x00}; // `nop byte r0`

    // 6 bytes
    static const char nop2[] = {0x01, 0x10, 0xC0, 0x00, 0x00, 0x00}; // `nop half 0`

    // 8 bytes
    static const char nop3[] = {0x01, 0x20, 0x00, 0x00, 0xC0, 0x00, 0x00, 0x00}; // `nop byte r0, half 0`

    // 10 bytes
    static const char nop4[] = {0x01, 0x20, 0x00, 0x00, 0xC1, 0x00, 0x00, 0x00, 0x00, 0x00}; // `nop byte r0, single 0`

    // 12 bytes
    static const char nop5[] = {0x01, 0x10, 0xC2, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; // `nop double 0`

    // 14 bytes
    static const char nop6[] = {0x01, 0x20, 0x00, 0x00, 0xC2, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; // `nop byte r0, double 0`

    // 16 bytes
    static const char nop7[] = {0x01,0x20, 0xC0, 0x00, 0x00, 0x00, 0xC2, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; // `nop half 0, double 0`

    // 18 bytes
    static const char nop8[] = {0x01,0x20, 0xC1, 0x00, 0x00, 0x00, 0x00, 0x00, 0xC2, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; // `nop single 0, double 0`

    // 20 bytes
    static const char nop9[] = {0x01,0x30, 0x00, 0x00, 0xC1, 0x00, 0x00, 0x00, 0x00, 0x00, 0xC2, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; // `nop byte r0, signle 0, double 0`

    // 22 bytes
    static const char nop10[] = {0x01, 0x20,0xC2, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,0xC2, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; // `nop double 0, double 0`

    // 24 bytes
    static const char nop11[] = {0x01, 0x30,0xC0, 0x00, 0x00, 0x00, 0xC2, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,0xC2, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; // `nop half 0, double 0, double 0`

    static const char* nops[] = {nop0,nop1, nop2, nop3, nop4, nop5, nop6, nop7, nop8, nop9, nop10, nop11};

    void *fill = bfd_malloc (count);
    if (fill == NULL)
        return fill;

    bfd_size_type nop_size = 24;

    if(code){
        if((count%2)==0)
            return NULL; // Need to signal an error, but this is basically an ICE. 
        bfd_byte *p = fill;
        while (count >= nop_size)
        {
        memcpy (p, nops[(nop_size/2) - 1], nop_size);
        p += nop_size;
        count -= nop_size;
        }
        if (count != 0)
            memcpy (p, nops[(count/2) - 1], count);
    }else{
        memset(fill,0,count);
    }

    return fill;
}

/* Fill the buffer with zero or long nop instruction if CODE is TRUE.  */
static void *
bfd_arch_clever_long_nop_fill (bfd_size_type count,
			     bool is_bigendian ATTRIBUTE_UNUSED,
			     bool code)
{
  return bfd_arch_clever_fill (count, code);
}

#define N(BITS, MACH, NAME, PRINT, DEF, FILL, NEXT)	\
  { BITS, /* Bits in a word.  */		\
    BITS, /* Bits in an address.  */		\
    8,    /* Bits in a byte. */			\
    bfd_arch_clever,				\
    MACH, /* Machine number.  */		\
    NAME,					\
    PRINT,					\
    3,   /* Section alignment power.  */	\
    DEF, /* Default architecture version ?  */	\
    compatible,			            \
    bfd_default_scan,				\
    FILL,					\
    NEXT,					\
    0 /* Maximum instruction length.  */	\
  }

const bfd_arch_info_type bfd_clever_arch = N(64, bfd_mach_clever1_0, "clever","clever",true,bfd_arch_clever_long_nop_fill,NULL);