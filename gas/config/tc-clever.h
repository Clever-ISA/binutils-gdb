
#define TC_CLEVER

#define LISTING_HEADER "Clever GAS "

#define TARGET_ARCH bfd_arch_clever
#define TARGET_MACH bfd_mach_clever1_0

#define TARGET_FORMAT "elf64-clever"

#define TARGET_BYTES_BIG_ENDIAN 0

#define WORKING_DOT_WORD

#define DIFF_EXPR_OK    1

/* Characters which always start a comment.  */
const char comment_chars[] = "#;";

/* Characters which start a comment at the beginning of a line.  */
const char line_comment_chars[] = "#";

/* This array holds machine specific line separator characters.  */
const char line_separator_chars[] = "";

/* Chars that can be used to separate mant from exp in floating point nums.  */
const char EXP_CHARS[] = "eE";

/* Chars that mean this number is a floating point constant as in 0f12.456  */
const char FLT_CHARS[] = "f";

