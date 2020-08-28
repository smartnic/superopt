#pragma once

#include <vector>
#include <unordered_set>
#include <unordered_map>
#include "z3++.h"
#include "bpf.h"
#include "../../../src/utils.h"
#include "../../../src/isa/inst_var.h"
#include "../../../src/isa/inst.h"
#include "inst_codegen.h"
#include "inst_var.h"

using namespace std;

// Max number of operands in one instruction
static constexpr int MAX_OP_LEN = 3;

// Number of bits of a single opcode or operand
static constexpr int OP_NUM_BITS = 32;
// Number of bits of a single instruction
static constexpr int INST_NUM_BITS = 20;

enum OPCODE_IDX {
  IDX_NOP = 0,
  // ALU64
  IDX_ADD64XC,
  IDX_ADD64XY,
  IDX_OR64XC,
  IDX_OR64XY,
  IDX_AND64XC,
  IDX_AND64XY,
  IDX_LSH64XC,
  IDX_LSH64XY,
  IDX_RSH64XC,
  IDX_RSH64XY,
  IDX_MOV64XC,
  IDX_MOV64XY,
  IDX_ARSH64XC,
  IDX_ARSH64XY,
  // ALU32
  IDX_ADD32XC,
  IDX_ADD32XY,
  IDX_OR32XC,
  IDX_OR32XY,
  IDX_AND32XC,
  IDX_AND32XY,
  IDX_LSH32XC,
  IDX_LSH32XY,
  IDX_RSH32XC,
  IDX_RSH32XY,
  IDX_MOV32XC,
  IDX_MOV32XY,
  IDX_ARSH32XC,
  IDX_ARSH32XY,
  // Byteswap
  IDX_LE,
  IDX_BE,
  // LD MAP ID
  IDX_LDMAPID,
  // Memory
  IDX_LDXB,
  IDX_STXB,
  IDX_LDXH,
  IDX_STXH,
  IDX_LDXW,
  IDX_STXW,
  IDX_LDXDW,
  IDX_STXDW,
  IDX_STB,
  IDX_STH,
  IDX_STW,
  IDX_STDW,
  IDX_XADD64,
  IDX_XADD32,
  IDX_LDABSH,
  IDX_LDINDH,
  // JMP
  IDX_JA,
  IDX_JEQXC,
  IDX_JEQXY,
  IDX_JGTXC,
  IDX_JGTXY,
  IDX_JNEXC,
  IDX_JNEXY,
  IDX_JSGTXC,
  IDX_JSGTXY,
  IDX_JEQ32XC,
  IDX_JEQ32XY,
  IDX_JNE32XC,
  IDX_JNE32XY,
  IDX_CALL, // function call
  // Exit
  IDX_EXIT,
  NUM_INSTR, // Number of opcode types
};

/* supported BPF functions */
static const int sp_bpf_func[] = {
  BPF_FUNC_map_lookup_elem,
  BPF_FUNC_map_update_elem,
  BPF_FUNC_map_delete_elem,
  BPF_FUNC_tail_call,
};

static constexpr int SP_BPF_FUNC_MAX_ID = (sizeof(sp_bpf_func) / sizeof(int));

#define OPCODE_BPF_ALU64_IMM(OP) BPF_ALU64 | BPF_OP(OP) | BPF_K
#define OPCODE_BPF_ALU64_REG(OP) BPF_ALU64 | BPF_OP(OP) | BPF_X
#define OPCODE_BPF_ALU32_IMM(OP) BPF_ALU | BPF_OP(OP) | BPF_K
#define OPCODE_BPF_ALU32_REG(OP) BPF_ALU | BPF_OP(OP) | BPF_X
#define OPCODE_BPF_ENDIAN(TYPE) BPF_ALU | BPF_END | BPF_SRC(TYPE)
#define OPCODE_BPF_MOV64_IMM BPF_ALU64 | BPF_MOV | BPF_K
#define OPCODE_BPF_MOV64_REG BPF_ALU64 | BPF_MOV | BPF_X
#define OPCODE_BPF_MOV32_IMM BPF_ALU | BPF_MOV | BPF_K
#define OPCODE_BPF_MOV32_REG BPF_ALU | BPF_MOV | BPF_X
#define OPCODE_BPF_LDMAPID (BPF_LD | BPF_DW | BPF_IMM)
#define OPCODE_BPF_LDX_MEM(SIZE) BPF_LDX | BPF_SIZE(SIZE) | BPF_MEM
#define OPCODE_BPF_STX_MEM(SIZE) BPF_STX | BPF_SIZE(SIZE) | BPF_MEM
#define OPCODE_BPF_ST_MEM(SIZE) BPF_ST | BPF_SIZE(SIZE) | BPF_MEM
#define OPCODE_BPF_XADD(SIZE) BPF_STX | BPF_XADD | BPF_SIZE(SIZE)
#define OPCODE_BPF_LDABS(SIZE) BPF_LD | BPF_ABS | BPF_SIZE(SIZE)
#define OPCODE_BPF_LDIND(SIZE) BPF_LD | BPF_IND | BPF_SIZE(SIZE)
#define OPCODE_BPF_JMP_IMM(OP) BPF_JMP | BPF_OP(OP) | BPF_K
#define OPCODE_BPF_JMP_REG(OP) BPF_JMP | BPF_OP(OP) | BPF_X
#define OPCODE_BPF_JMP32_IMM(OP) BPF_JMP32 | BPF_OP(OP) | BPF_K
#define OPCODE_BPF_JMP32_REG(OP) BPF_JMP32 | BPF_OP(OP) | BPF_X
#define OPCODE_BPF_JMP_A BPF_JMP | BPF_JA
#define OPCODE_BPF_FUNC_CALL BPF_JMP | BPF_CALL
#define OPCODE_BPF_EXIT_INSN BPF_JMP | BPF_EXIT

// Instruction opcodes
enum OPCODES {
  NOP = 0,
  ADD64XC  = OPCODE_BPF_ALU64_IMM(BPF_ADD),
  ADD64XY  = OPCODE_BPF_ALU64_REG(BPF_ADD),
  OR64XC   = OPCODE_BPF_ALU64_IMM(BPF_OR),
  OR64XY   = OPCODE_BPF_ALU64_REG(BPF_OR),
  AND64XC  = OPCODE_BPF_ALU64_IMM(BPF_AND),
  AND64XY  = OPCODE_BPF_ALU64_REG(BPF_AND),
  LSH64XC  = OPCODE_BPF_ALU64_IMM(BPF_LSH),
  LSH64XY  = OPCODE_BPF_ALU64_REG(BPF_LSH),
  RSH64XC  = OPCODE_BPF_ALU64_IMM(BPF_RSH),
  RSH64XY  = OPCODE_BPF_ALU64_REG(BPF_RSH),
  MOV64XC  = OPCODE_BPF_MOV64_IMM,
  MOV64XY  = OPCODE_BPF_MOV64_REG,
  ARSH64XC = OPCODE_BPF_ALU64_IMM(BPF_ARSH),
  ARSH64XY = OPCODE_BPF_ALU64_REG(BPF_ARSH),
  ADD32XC  = OPCODE_BPF_ALU32_IMM(BPF_ADD),
  ADD32XY  = OPCODE_BPF_ALU32_REG(BPF_ADD),
  OR32XC   = OPCODE_BPF_ALU32_IMM(BPF_OR),
  OR32XY   = OPCODE_BPF_ALU32_REG(BPF_OR),
  AND32XC  = OPCODE_BPF_ALU32_IMM(BPF_AND),
  AND32XY  = OPCODE_BPF_ALU32_REG(BPF_AND),
  LSH32XC  = OPCODE_BPF_ALU32_IMM(BPF_LSH),
  LSH32XY  = OPCODE_BPF_ALU32_REG(BPF_LSH),
  RSH32XC  = OPCODE_BPF_ALU32_IMM(BPF_RSH),
  RSH32XY  = OPCODE_BPF_ALU32_REG(BPF_RSH),
  MOV32XC  = OPCODE_BPF_MOV32_IMM,
  MOV32XY  = OPCODE_BPF_MOV32_REG,
  ARSH32XC = OPCODE_BPF_ALU32_IMM(BPF_ARSH),
  ARSH32XY = OPCODE_BPF_ALU32_REG(BPF_ARSH),
  LE       = OPCODE_BPF_ENDIAN(BPF_TO_LE),
  BE       = OPCODE_BPF_ENDIAN(BPF_TO_BE),
  LDMAPID  = OPCODE_BPF_LDMAPID,
  LDXB     = OPCODE_BPF_LDX_MEM(BPF_B),
  STXB     = OPCODE_BPF_STX_MEM(BPF_B),
  LDXH     = OPCODE_BPF_LDX_MEM(BPF_H),
  STXH     = OPCODE_BPF_STX_MEM(BPF_H),
  LDXW     = OPCODE_BPF_LDX_MEM(BPF_W),
  STXW     = OPCODE_BPF_STX_MEM(BPF_W),
  LDXDW    = OPCODE_BPF_LDX_MEM(BPF_DW),
  STXDW    = OPCODE_BPF_STX_MEM(BPF_DW),
  STB      = OPCODE_BPF_ST_MEM(BPF_B),
  STH      = OPCODE_BPF_ST_MEM(BPF_H),
  STW      = OPCODE_BPF_ST_MEM(BPF_W),
  STDW     = OPCODE_BPF_ST_MEM(BPF_DW),
  XADD64   = OPCODE_BPF_XADD(BPF_DW),
  XADD32   = OPCODE_BPF_XADD(BPF_W),
  LDABSH   = OPCODE_BPF_LDABS(BPF_H),
  LDINDH   = OPCODE_BPF_LDIND(BPF_H),
  JA       = OPCODE_BPF_JMP_A,
  JEQXC    = OPCODE_BPF_JMP_IMM(BPF_JEQ),
  JEQXY    = OPCODE_BPF_JMP_REG(BPF_JEQ),
  JGTXC    = OPCODE_BPF_JMP_IMM(BPF_JGT),
  JGTXY    = OPCODE_BPF_JMP_REG(BPF_JGT),
  JNEXC    = OPCODE_BPF_JMP_IMM(BPF_JNE),
  JNEXY    = OPCODE_BPF_JMP_REG(BPF_JNE),
  JSGTXC   = OPCODE_BPF_JMP_IMM(BPF_JSGT),
  JSGTXY   = OPCODE_BPF_JMP_REG(BPF_JSGT),
  JEQ32XC  = OPCODE_BPF_JMP32_IMM(BPF_JEQ),
  JEQ32XY  = OPCODE_BPF_JMP32_REG(BPF_JEQ),
  JNE32XC  = OPCODE_BPF_JMP32_IMM(BPF_JNE),
  JNE32XY  = OPCODE_BPF_JMP32_REG(BPF_JNE),
  CALL     = OPCODE_BPF_FUNC_CALL,
  EXIT     = OPCODE_BPF_EXIT_INSN,
};

int opcode_2_idx(int opcode);
static const int idx_2_opcode[NUM_INSTR] = {
  [IDX_NOP] = NOP,
  [IDX_ADD64XC] = ADD64XC,
  [IDX_ADD64XY] = ADD64XY,
  [IDX_OR64XC] = OR64XC,
  [IDX_OR64XY] = OR64XY,
  [IDX_AND64XC] = AND64XC,
  [IDX_AND64XY] = AND64XY,
  [IDX_LSH64XC] = LSH64XC,
  [IDX_LSH64XY] = LSH64XY,
  [IDX_RSH64XC] = RSH64XC,
  [IDX_RSH64XY] = RSH64XY,
  [IDX_MOV64XC] = MOV64XC,
  [IDX_MOV64XY] = MOV64XY,
  [IDX_ARSH64XC] = ARSH64XC,
  [IDX_ARSH64XY] = ARSH64XY,
  [IDX_ADD32XC] = ADD32XC,
  [IDX_ADD32XY] = ADD32XY,
  [IDX_OR32XC]  = OR32XC,
  [IDX_OR32XY]  = OR32XY,
  [IDX_AND32XC] = AND32XC,
  [IDX_AND32XY] = AND32XY,
  [IDX_LSH32XC] = LSH32XC,
  [IDX_LSH32XY] = LSH32XY,
  [IDX_RSH32XC] = RSH32XC,
  [IDX_RSH32XY] = RSH32XY,
  [IDX_MOV32XC] = MOV32XC,
  [IDX_MOV32XY] = MOV32XY,
  [IDX_ARSH32XC] = ARSH32XC,
  [IDX_ARSH32XY] = ARSH32XY,
  [IDX_LE] = LE,
  [IDX_BE] = BE,
  [IDX_LDMAPID] = LDMAPID,
  [IDX_LDXB] = LDXB,
  [IDX_STXB] = STXB,
  [IDX_LDXH] = LDXH,
  [IDX_STXH] = STXH,
  [IDX_LDXW] = LDXW,
  [IDX_STXW] = STXW,
  [IDX_LDXDW] = LDXDW,
  [IDX_STXDW] = STXDW,
  [IDX_STB] = STB,
  [IDX_STH] = STH,
  [IDX_STW] = STW,
  [IDX_STDW] = STDW,
  [IDX_XADD64] = XADD64,
  [IDX_XADD32] = XADD32,
  [IDX_LDABSH] = LDABSH,
  [IDX_LDINDH] = LDINDH,
  [IDX_JA] = JA,
  [IDX_JEQXC] = JEQXC,
  [IDX_JEQXY] = JEQXY,
  [IDX_JGTXC] = JGTXC,
  [IDX_JGTXY] = JGTXY,
  [IDX_JNEXC] = JNEXC,
  [IDX_JNEXY] = JNEXY,
  [IDX_JSGTXC] = JSGTXC,
  [IDX_JSGTXY] = JSGTXY,
  [IDX_JEQ32XC] = JEQ32XC,
  [IDX_JEQ32XY] = JEQ32XY,
  [IDX_JNE32XC] = JNE32XC,
  [IDX_JNE32XY] = JNE32XY,
  [IDX_CALL] = CALL,
  [IDX_EXIT] = EXIT,
};

static const int num_operands[NUM_INSTR] = {
  [IDX_NOP]      = 0,
  [IDX_ADD64XC]  = 2,
  [IDX_ADD64XY]  = 2,
  [IDX_OR64XC]   = 2,
  [IDX_OR64XY]   = 2,
  [IDX_AND64XC]  = 2,
  [IDX_AND64XY]  = 2,
  [IDX_LSH64XC]  = 2,
  [IDX_LSH64XY]  = 2,
  [IDX_RSH64XC]  = 2,
  [IDX_RSH64XY]  = 2,
  [IDX_MOV64XC]  = 2,
  [IDX_MOV64XY]  = 2,
  [IDX_ARSH64XC] = 2,
  [IDX_ARSH64XY] = 2,
  [IDX_ADD32XC]  = 2,
  [IDX_ADD32XY]  = 2,
  [IDX_OR32XC]   = 2,
  [IDX_OR32XY]   = 2,
  [IDX_AND32XC]  = 2,
  [IDX_AND32XY]  = 2,
  [IDX_LSH32XC]  = 2,
  [IDX_LSH32XY]  = 2,
  [IDX_RSH32XC]  = 2,
  [IDX_RSH32XY]  = 2,
  [IDX_MOV32XC]  = 2,
  [IDX_MOV32XY]  = 2,
  [IDX_ARSH32XC] = 2,
  [IDX_ARSH32XY] = 2,
  [IDX_LE]       = 2,
  [IDX_BE]       = 2,
  [IDX_LDMAPID]  = 2,
  [IDX_LDXB]     = 3,
  [IDX_STXB]     = 3,
  [IDX_LDXH]     = 3,
  [IDX_STXH]     = 3,
  [IDX_LDXW]     = 3,
  [IDX_STXW]     = 3,
  [IDX_LDXDW]    = 3,
  [IDX_STXDW]    = 3,
  [IDX_STB]      = 3,
  [IDX_STH]      = 3,
  [IDX_STW]      = 3,
  [IDX_STDW]     = 3,
  [IDX_XADD64]   = 3,
  [IDX_XADD32]   = 3,
  [IDX_LDABSH]   = 1,
  [IDX_LDINDH]   = 1,
  [IDX_JA]       = 1,
  [IDX_JEQXC]    = 3,
  [IDX_JEQXY]    = 3,
  [IDX_JGTXC]    = 3,
  [IDX_JGTXY]    = 3,
  [IDX_JNEXC]    = 3,
  [IDX_JNEXY]    = 3,
  [IDX_JSGTXC]   = 3,
  [IDX_JSGTXY]   = 3,
  [IDX_JEQ32XC]  = 3,
  [IDX_JEQ32XY]  = 3,
  [IDX_JNE32XC]  = 3,
  [IDX_JNE32XY]  = 3,
  [IDX_CALL]     = 1,
  [IDX_EXIT]     = 0,
};

// number of registers for each opcode
// e.g., for ADD64XY, two operands are registers
static const int insn_num_regs[NUM_INSTR] = {
  [IDX_NOP]      = 0,
  [IDX_ADD64XC]  = 1,
  [IDX_ADD64XY]  = 2,
  [IDX_OR64XC]   = 1,
  [IDX_OR64XY]   = 2,
  [IDX_AND64XC]  = 1,
  [IDX_AND64XY]  = 2,
  [IDX_LSH64XC]  = 1,
  [IDX_LSH64XY]  = 2,
  [IDX_RSH64XC]  = 1,
  [IDX_RSH64XY]  = 2,
  [IDX_MOV64XC]  = 1,
  [IDX_MOV64XY]  = 2,
  [IDX_ARSH64XC] = 1,
  [IDX_ARSH64XY] = 2,
  [IDX_ADD32XC]  = 1,
  [IDX_ADD32XY]  = 2,
  [IDX_OR32XC]   = 1,
  [IDX_OR32XY]   = 2,
  [IDX_AND32XC]  = 1,
  [IDX_AND32XY]  = 2,
  [IDX_LSH32XC]  = 1,
  [IDX_LSH32XY]  = 2,
  [IDX_RSH32XC]  = 1,
  [IDX_RSH32XY]  = 2,
  [IDX_MOV32XC]  = 1,
  [IDX_MOV32XY]  = 2,
  [IDX_ARSH32XC] = 1,
  [IDX_ARSH32XY] = 2,
  [IDX_LE]       = 1,
  [IDX_BE]       = 1,
  [IDX_LDMAPID]  = 1,
  [IDX_LDXB]     = 2,
  [IDX_STXB]     = 2,
  [IDX_LDXH]     = 2,
  [IDX_STXH]     = 2,
  [IDX_LDXW]     = 2,
  [IDX_STXW]     = 2,
  [IDX_LDXDW]    = 2,
  [IDX_STXDW]    = 2,
  [IDX_STB]      = 1,
  [IDX_STH]      = 1,
  [IDX_STW]      = 1,
  [IDX_STDW]     = 1,
  [IDX_XADD64]   = 2,
  [IDX_XADD32]   = 2,
  [IDX_LDABSH]   = 0,
  [IDX_LDINDH]   = 1,
  [IDX_JA]       = 0,
  [IDX_JEQXC]    = 1,
  [IDX_JEQXY]    = 2,
  [IDX_JGTXC]    = 1,
  [IDX_JGTXY]    = 2,
  [IDX_JNEXC]    = 1,
  [IDX_JNEXY]    = 2,
  [IDX_JSGTXC]   = 1,
  [IDX_JSGTXY]   = 2,
  [IDX_JEQ32XC]  = 1,
  [IDX_JEQ32XY]  = 2,
  [IDX_JNE32XC]  = 1,
  [IDX_JNE32XY]  = 2,
  [IDX_CALL]     = 0,
  [IDX_EXIT]     = 0,
};

static const int opcode_type[NUM_INSTR] = {
  [IDX_NOP]      = OP_NOP,
  [IDX_ADD64XC]  = OP_OTHERS,
  [IDX_ADD64XY]  = OP_OTHERS,
  [IDX_OR64XC]   = OP_OTHERS,
  [IDX_OR64XY]   = OP_OTHERS,
  [IDX_AND64XC]  = OP_OTHERS,
  [IDX_AND64XY]  = OP_OTHERS,
  [IDX_LSH64XC]  = OP_OTHERS,
  [IDX_LSH64XY]  = OP_OTHERS,
  [IDX_RSH64XC]  = OP_OTHERS,
  [IDX_RSH64XY]  = OP_OTHERS,
  [IDX_MOV64XC]  = OP_OTHERS,
  [IDX_MOV64XY]  = OP_OTHERS,
  [IDX_ARSH64XC] = OP_OTHERS,
  [IDX_ARSH64XY] = OP_OTHERS,
  [IDX_ADD32XC]  = OP_OTHERS,
  [IDX_ADD32XY]  = OP_OTHERS,
  [IDX_OR32XC]   = OP_OTHERS,
  [IDX_OR32XY]   = OP_OTHERS,
  [IDX_AND32XC]  = OP_OTHERS,
  [IDX_AND32XY]  = OP_OTHERS,
  [IDX_LSH32XC]  = OP_OTHERS,
  [IDX_LSH32XY]  = OP_OTHERS,
  [IDX_RSH32XC]  = OP_OTHERS,
  [IDX_RSH32XY]  = OP_OTHERS,
  [IDX_MOV32XC]  = OP_OTHERS,
  [IDX_MOV32XY]  = OP_OTHERS,
  [IDX_ARSH32XC] = OP_OTHERS,
  [IDX_ARSH32XY] = OP_OTHERS,
  [IDX_LE]       = OP_OTHERS,
  [IDX_BE]       = OP_OTHERS,
  [IDX_LDMAPID]  = OP_OTHERS,
  [IDX_LDXB]     = OP_LD,
  [IDX_STXB]     = OP_ST,
  [IDX_LDXH]     = OP_LD,
  [IDX_STXH]     = OP_ST,
  [IDX_LDXW]     = OP_LD,
  [IDX_STXW]     = OP_ST,
  [IDX_LDXDW]    = OP_LD,
  [IDX_STXDW]    = OP_ST,
  [IDX_STB]      = OP_ST,
  [IDX_STH]      = OP_ST,
  [IDX_STW]      = OP_ST,
  [IDX_STDW]     = OP_ST,
  [IDX_XADD64]   = OP_ST,
  [IDX_XADD32]   = OP_ST,
  [IDX_LDABSH]   = OP_LD,
  [IDX_LDINDH]   = OP_LD,
  [IDX_JA]       = OP_UNCOND_JMP,
  [IDX_JEQXC]    = OP_COND_JMP,
  [IDX_JEQXY]    = OP_COND_JMP,
  [IDX_JGTXC]    = OP_COND_JMP,
  [IDX_JGTXY]    = OP_COND_JMP,
  [IDX_JNEXC]    = OP_COND_JMP,
  [IDX_JNEXY]    = OP_COND_JMP,
  [IDX_JSGTXC]   = OP_COND_JMP,
  [IDX_JSGTXY]   = OP_COND_JMP,
  [IDX_JEQ32XC]  = OP_COND_JMP,
  [IDX_JEQ32XY]  = OP_COND_JMP,
  [IDX_JNE32XC]  = OP_COND_JMP,
  [IDX_JNE32XY]  = OP_COND_JMP,
  [IDX_CALL]     = OP_CALL,
  [IDX_EXIT]     = OP_RET,
};

// Max and Min value for immediate number(32bits), offset(16bits)
// MAX value for immediate number of shift 32 and shift 64
// static constexpr int32_t MAX_IMM = 0x7fffffff;
// static constexpr int32_t MIN_IMM = 0x80000000;
static constexpr int32_t MAX_IMM = 10;
static constexpr int32_t MIN_IMM = -10;
static constexpr int16_t MAX_OFF = 10;
static constexpr int16_t MIN_OFF = -10;
static constexpr int32_t MAX_IMM_SH32 = 31;
static constexpr int32_t MAX_IMM_SH64 = 63;
// 3 types of OP_IMM_ENDIAN: 16, 32, 64
// type counts from 0
static constexpr int32_t MAX_TYPES_IMM_ENDIAN = 2;
static constexpr int32_t MAX_CALL_IMM = SP_BPF_FUNC_MAX_ID - 1;
// Operand types for instructions
enum OPERANDS {
  OP_UNUSED = 0,
  OP_DST_REG,
  OP_SRC_REG,
  OP_OFF,
  OP_IMM,
};

/* The definitions below assume a minimum 16-bit integer data type */
#define OPTYPE(opcode, opindex) ((optable[opcode_2_idx(opcode)] >> ((opindex) * 5)) & 31)
#define FSTOP(x) (x)
#define SNDOP(x) (x << 5)
#define TRDOP(x) (x << 10)
#define ALU_OPS_IMM (FSTOP(OP_DST_REG) | SNDOP(OP_IMM) | TRDOP(OP_UNUSED))
#define ALU_OPS_REG (FSTOP(OP_DST_REG) | SNDOP(OP_SRC_REG) | TRDOP(OP_UNUSED))
#define BYTESWAP (FSTOP(OP_DST_REG) | SNDOP(OP_IMM) | TRDOP(OP_UNUSED))
#define LDMAPID_OPS (FSTOP(OP_DST_REG) | SNDOP(OP_IMM) | TRDOP(OP_UNUSED))
#define LDX_OPS (FSTOP(OP_DST_REG) | SNDOP(OP_SRC_REG) | TRDOP(OP_OFF))
#define STX_OPS (FSTOP(OP_DST_REG) | SNDOP(OP_OFF) | TRDOP(OP_SRC_REG))
#define ST_OPS  (FSTOP(OP_DST_REG) | SNDOP(OP_OFF) | TRDOP(OP_IMM))
#define LDABS_OPS (FSTOP(OP_IMM) | SNDOP(OP_UNUSED) | TRDOP(OP_UNUSED))
#define LDIND_OPS (FSTOP(OP_SRC_REG) | SNDOP(OP_UNUSED) | TRDOP(OP_UNUSED))
#define JA_OPS (FSTOP(OP_OFF) | SNDOP(OP_UNUSED) | TRDOP(OP_UNUSED))
#define JMP_OPS_IMM (FSTOP(OP_DST_REG) | SNDOP(OP_IMM) | TRDOP(OP_OFF))
#define JMP_OPS_REG (FSTOP(OP_DST_REG) | SNDOP(OP_SRC_REG) | TRDOP(OP_OFF))
#define CALL_OPS (FSTOP(OP_IMM) | SNDOP(OP_UNUSED) | TRDOP(OP_UNUSED))
#define UNUSED_OPS (FSTOP(OP_UNUSED) | SNDOP(OP_UNUSED) | TRDOP(OP_UNUSED))
static const int optable[NUM_INSTR] = {
  [IDX_NOP]      = UNUSED_OPS,
  [IDX_ADD64XC]  = ALU_OPS_IMM,
  [IDX_ADD64XY]  = ALU_OPS_REG,
  [IDX_OR64XC]   = ALU_OPS_IMM,
  [IDX_OR64XY]   = ALU_OPS_REG,
  [IDX_AND64XC]  = ALU_OPS_IMM,
  [IDX_AND64XY]  = ALU_OPS_REG,
  [IDX_LSH64XC]  = ALU_OPS_IMM,
  [IDX_LSH64XY]  = ALU_OPS_REG,
  [IDX_RSH64XC]  = ALU_OPS_IMM,
  [IDX_RSH64XY]  = ALU_OPS_REG,
  [IDX_MOV64XC]  = ALU_OPS_IMM,
  [IDX_MOV64XY]  = ALU_OPS_REG,
  [IDX_ARSH64XC] = ALU_OPS_IMM,
  [IDX_ARSH64XY] = ALU_OPS_REG,
  [IDX_ADD32XC]  = ALU_OPS_IMM,
  [IDX_ADD32XY]  = ALU_OPS_REG,
  [IDX_OR32XC]   = ALU_OPS_IMM,
  [IDX_OR32XY]   = ALU_OPS_REG,
  [IDX_AND32XC]  = ALU_OPS_IMM,
  [IDX_AND32XY]  = ALU_OPS_REG,
  [IDX_LSH32XC]  = ALU_OPS_IMM,
  [IDX_LSH32XY]  = ALU_OPS_REG,
  [IDX_RSH32XC]  = ALU_OPS_IMM,
  [IDX_RSH32XY]  = ALU_OPS_REG,
  [IDX_MOV32XC]  = ALU_OPS_IMM,
  [IDX_MOV32XY]  = ALU_OPS_REG,
  [IDX_ARSH32XC] = ALU_OPS_IMM,
  [IDX_ARSH32XY] = ALU_OPS_REG,
  [IDX_LE]       = BYTESWAP,
  [IDX_BE]       = BYTESWAP,
  [IDX_LDMAPID]  = LDMAPID_OPS,
  [IDX_LDXB]     = LDX_OPS,
  [IDX_STXB]     = STX_OPS,
  [IDX_LDXH]     = LDX_OPS,
  [IDX_STXH]     = STX_OPS,
  [IDX_LDXW]     = LDX_OPS,
  [IDX_STXW]     = STX_OPS,
  [IDX_LDXDW]    = LDX_OPS,
  [IDX_STXDW]    = STX_OPS,
  [IDX_STB]      = ST_OPS,
  [IDX_STH]      = ST_OPS,
  [IDX_STW]      = ST_OPS,
  [IDX_STDW]     = ST_OPS,
  [IDX_XADD64]   = STX_OPS,
  [IDX_XADD32]   = STX_OPS,
  [IDX_LDABSH]   = LDABS_OPS,
  [IDX_LDINDH]   = LDIND_OPS,
  [IDX_JA]       = JA_OPS,
  [IDX_JEQXC]    = JMP_OPS_IMM,
  [IDX_JEQXY]    = JMP_OPS_REG,
  [IDX_JGTXC]    = JMP_OPS_IMM,
  [IDX_JGTXY]    = JMP_OPS_REG,
  [IDX_JNEXC]    = JMP_OPS_IMM,
  [IDX_JNEXY]    = JMP_OPS_REG,
  [IDX_JSGTXC]   = JMP_OPS_IMM,
  [IDX_JSGTXY]   = JMP_OPS_REG,
  [IDX_JEQ32XC]  = JMP_OPS_IMM,
  [IDX_JEQ32XY]  = JMP_OPS_REG,
  [IDX_JNE32XC]  = JMP_OPS_IMM,
  [IDX_JNE32XY]  = JMP_OPS_REG,
  [IDX_CALL]     = CALL_OPS,
  [IDX_EXIT]     = UNUSED_OPS,
};
#undef FSTOP
#undef SNDOP
#undef TRDOP
#undef ALU_OPS_IMM
#undef ALU_OPS_REG
#undef BYTESWAP
#undef LDMAPID_OPS
#undef LDX_OPS
#undef STX_OPS
#undef ST_OPS
#undef LDABS_OPS
#undef JMP_OPS_IMM
#undef JMP_OPS_REG
#undef CALL_OPS
#undef UNUSED_OPS

class inst: public inst_base {
 private:
  void set_off(int op_value);
  int32_t get_max_imm() const;
  int32_t get_min_imm() const;
  int16_t get_max_off(int inst_index) const;
  int16_t get_min_off() const;
  static void sorted_vec_insert(int32_t num, vector<int32_t>& sorted_vec);
 public:
  // original program's additional immediate numbers which are not in [MIN_IMM, MAX_IMM]
  // overall sample space for immeditae number is
  // union([MIN_IMM, MAX_IMM], _sample_neg_imms, _sample_pos_imms)
  static vector<int32_t> _sample_neg_imms; // store negative numbers
  static vector<int32_t> _sample_pos_imms; // store positive numbers
  static vector<int32_t> _sample_neg_offs; // store negative offsets
  static vector<int32_t> _sample_pos_offs; // store positive offsets
  int32_t _dst_reg;
  int32_t _src_reg;
  int32_t _imm;
  int16_t _off;
  inst(int opcode, int32_t src_reg, int32_t dst_reg, int16_t off, int32_t imm) {
    _opcode  = opcode;
    _dst_reg = dst_reg;
    _src_reg = src_reg;
    _imm = imm;
    _off = off;
  }
  inst(int opcode = NOP, int32_t arg1 = 0, int32_t arg2 = 0, int32_t arg3 = 0);
  void set_imm(int op_value);
  void to_abs_bv(vector<op_t>& abs_vec) const;
  int get_operand(int op_index) const;
  void set_operand(int op_index, op_t op_value);
  int get_opcode_by_idx(int idx) const;
  inst& operator=(const inst &rhs);
  bool operator==(const inst &x) const;
  string opcode_to_str(int) const;
  void print() const;
  vector<int> get_canonical_reg_list() const;
  static vector<int> get_isa_canonical_reg_list();
  static void add_sample_imm(const vector<int32_t>& nums);
  static void add_sample_off(const vector<int16_t>& nums);
  int32_t get_max_operand_val(int op_index, int inst_index = 0) const;
  int32_t get_min_operand_val(int op_index, int inst_index = 0) const;
  int get_jmp_dis() const;
  void insert_jmp_opcodes(unordered_set<int>& jmp_set) const;
  int inst_output_opcode_type() const;
  int inst_output() const;
  bool is_real_inst() const;
  bool is_reg(int op_index) const;
  int implicit_ret_reg() const;
  void set_as_nop_inst();
  unsigned int get_input_reg() const {return 1;}
  int get_num_operands() const {return num_operands[opcode_2_idx(_opcode)];}
  int get_insn_num_regs() const {return insn_num_regs[opcode_2_idx(_opcode)];}
  int get_opcode_type() const {return opcode_type[opcode_2_idx(_opcode)];}
  // smt
  z3::expr smt_inst(smt_var& sv, unsigned int block = 0) const;
  z3::expr smt_inst_jmp(smt_var& sv) const;
  z3::expr smt_inst_end(smt_var& sv) const;
  static z3::expr smt_set_pre(z3::expr input, smt_var& sv);
  bool is_cfg_basic_block_end() const;

  string get_bytecode_str() const;
};

void interpret(inout_t& output, inst* program, int length, prog_state &ps, const inout_t& input);
