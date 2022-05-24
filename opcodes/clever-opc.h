#ifndef CLEVER_OPC_H
#define CLEVER_OPC_H

#include "opcode/clever.h"


#ifdef __cplusplus
extern "C"{
#endif

extern const struct clever_branch_info clever_ubranches[];
extern const struct clever_branch_info clever_sbranches[];

extern const struct clever_branch_info clever_cond_branch_info;

extern const struct clever_instruction_info clever_insns[];

#ifdef __cplusplus
};
#endif

#endif