#pragma once

#include <vector>
#include <unordered_set>
#include "z3++.h"
#include "../../../src/utils.h"
#include "../../../src/verify/smt_var.h"
#include "../../../src/isa/inst.h"
#include "inst_codegen.h"

using namespace std;

static constexpr int NUM_REGS = 11;
static constexpr int MAX_PROG_LEN = 7;
// Max number of operands in one instruction
static constexpr int MAX_OP_LEN = 3;

// Number of bits of a single opcode or operand
static constexpr int OP_NUM_BITS = 32;
// Number of bits of a single instruction
static constexpr int INST_NUM_BITS = 20;

// Instruction opcodes
enum OPCODES {
  NOP = 0,
  // ALU64
  ADD64XC,
  ADD64XY,
  LSH64XC,
  LSH64XY,
  RSH64XC,
  RSH64XY,
  MOV64XC,
  MOV64XY,
  ARSH64XC,
  ARSH64XY,
  // ALU32
  ADD32XC,
  ADD32XY,
  LSH32XC,
  LSH32XY,
  RSH32XC,
  RSH32XY,
  MOV32XC,
  MOV32XY,
  ARSH32XC,
  ARSH32XY,
  // Byteswap
  LE,
  BE,
  // JMP
  JA,
  JEQXC,
  JEQXY,
  JGTXC,
  JGTXY,
  JSGTXC,
  JSGTXY,
  // Exit
  EXIT,
  NUM_INSTR, // Number of opcode types
};

static constexpr int num_operands[NUM_INSTR] = {
  [NOP]      = 0,
  [ADD64XC]  = 2,
  [ADD64XY]  = 2,
  [LSH64XC]  = 2,
  [LSH64XY]  = 2,
  [RSH64XC]  = 2,
  [RSH64XY]  = 2,
  [MOV64XC]  = 2,
  [MOV64XY]  = 2,
  [ARSH64XC] = 2,
  [ARSH64XY] = 2,
  [ADD32XC]  = 2,
  [ADD32XY]  = 2,
  [LSH32XC]  = 2,
  [LSH32XY]  = 2,
  [RSH32XC]  = 2,
  [RSH32XY]  = 2,
  [MOV32XC]  = 2,
  [MOV32XY]  = 2,
  [ARSH32XC] = 2,
  [ARSH32XY] = 2,
  [LE]       = 2,
  [BE]       = 2,
  [JA]       = 1,
  [JEQXC]    = 3,
  [JEQXY]    = 3,
  [JGTXC]    = 3,
  [JGTXY]    = 3,
  [JSGTXC]   = 3,
  [JSGTXY]   = 3,
  [EXIT]     = 0,
};

// number of registers for each opcode
// e.g., for ADD64XY, two operands are registers
static constexpr int insn_num_regs[NUM_INSTR] = {
  [NOP]      = 0,
  [ADD64XC]  = 1,
  [ADD64XY]  = 2,
  [LSH64XC]  = 1,
  [LSH64XY]  = 2,
  [RSH64XC]  = 1,
  [RSH64XY]  = 2,
  [MOV64XC]  = 1,
  [MOV64XY]  = 2,
  [ARSH64XC] = 1,
  [ARSH64XY] = 2,
  [ADD32XC]  = 1,
  [ADD32XY]  = 2,
  [LSH32XC]  = 1,
  [LSH32XY]  = 2,
  [RSH32XC]  = 1,
  [RSH32XY]  = 2,
  [MOV32XC]  = 1,
  [MOV32XY]  = 2,
  [ARSH32XC] = 1,
  [ARSH32XY] = 2,
  [LE]       = 1,
  [BE]       = 1,
  [JA]       = 0,
  [JEQXC]    = 1,
  [JEQXY]    = 2,
  [JGTXC]    = 1,
  [JGTXY]    = 2,
  [JSGTXC]   = 1,
  [JSGTXY]   = 2,
  [EXIT]     = 0,
};

static constexpr int opcode_type[NUM_INSTR] = {
  [NOP]      = OP_NOP,
  [ADD64XC]  = OP_OTHERS,
  [ADD64XY]  = OP_OTHERS,
  [LSH64XC]  = OP_OTHERS,
  [LSH64XY]  = OP_OTHERS,
  [RSH64XC]  = OP_OTHERS,
  [RSH64XY]  = OP_OTHERS,
  [MOV64XC]  = OP_OTHERS,
  [MOV64XY]  = OP_OTHERS,
  [ARSH64XC] = OP_OTHERS,
  [ARSH64XY] = OP_OTHERS,
  [ADD32XC]  = OP_OTHERS,
  [ADD32XY]  = OP_OTHERS,
  [LSH32XC]  = OP_OTHERS,
  [LSH32XY]  = OP_OTHERS,
  [RSH32XC]  = OP_OTHERS,
  [RSH32XY]  = OP_OTHERS,
  [MOV32XC]  = OP_OTHERS,
  [MOV32XY]  = OP_OTHERS,
  [ARSH32XC] = OP_OTHERS,
  [ARSH32XY] = OP_OTHERS,
  [LE]       = OP_OTHERS,
  [BE]       = OP_OTHERS,
  [JA]       = OP_UNCOND_JMP,
  [JEQXC]    = OP_COND_JMP,
  [JEQXY]    = OP_COND_JMP,
  [JGTXC]    = OP_COND_JMP,
  [JGTXY]    = OP_COND_JMP,
  [JSGTXC]   = OP_COND_JMP,
  [JSGTXY]   = OP_COND_JMP,
  [EXIT]     = OP_RET,
};

// Max and Min value for immediate number(32bits), offset(16bits)
// MAX value for immediate number of shift 32 and shift 64
static constexpr int32_t MAX_IMM = 0x7fffffff;
static constexpr int32_t MIN_IMM = 0x80000000;
static constexpr int32_t MAX_OFF = 0x7fff;
static constexpr int32_t MIN_OFF = 0xffff8000;
static constexpr int32_t MAX_IMM_SH32 = 31;
static constexpr int32_t MAX_IMM_SH64 = 63;
// 3 types of OP_IMM_ENDIAN: 16, 32, 64
// type counts from 0
static constexpr int32_t TYPES_IMM_ENDIAN = 2;

// Operand types for instructions
static constexpr int OP_UNUSED = 0;
static constexpr int OP_REG = 1;
static constexpr int OP_IMM = 2;
static constexpr int OP_OFF = 3;
static constexpr int OP_IMM_SH32 = 4;
static constexpr int OP_IMM_SH64 = 5;
static constexpr int OP_IMM_ENDIAN = 6;

/* The definitions below assume a minimum 16-bit integer data type */
#define OPTYPE(opcode, opindex) ((optable[opcode] >> ((opindex) * 5)) & 31)
#define FSTOP(x) (x)
#define SNDOP(x) (x << 5)
#define TRDOP(x) (x << 10)
#define ALU_OPS_IMM (FSTOP(OP_REG) | SNDOP(OP_IMM) | TRDOP(OP_UNUSED))
#define ALU_OPS_REG (FSTOP(OP_REG) | SNDOP(OP_REG) | TRDOP(OP_UNUSED))
#define SH32_OPS_IMM (FSTOP(OP_REG) | SNDOP(OP_IMM_SH32) | TRDOP(OP_UNUSED))
#define SH64_OPS_IMM (FSTOP(OP_REG) | SNDOP(OP_IMM_SH64) | TRDOP(OP_UNUSED))
#define BYTESWAP (FSTOP(OP_REG) | SNDOP(OP_IMM_ENDIAN) | TRDOP(OP_UNUSED))
#define JA_OPS (FSTOP(OP_OFF) | SNDOP(OP_UNUSED) | TRDOP(OP_UNUSED))
#define JMP_OPS_IMM (FSTOP(OP_REG) | SNDOP(OP_IMM) | TRDOP(OP_OFF))
#define JMP_OPS_REG (FSTOP(OP_REG) | SNDOP(OP_REG) | TRDOP(OP_OFF))
#define UNUSED_OPS (FSTOP(OP_UNUSED) | SNDOP(OP_UNUSED) | TRDOP(OP_UNUSED))
static constexpr int optable[NUM_INSTR] = {
  [NOP]      = UNUSED_OPS,
  [ADD64XC]  = ALU_OPS_IMM,
  [ADD64XY]  = ALU_OPS_REG,
  [LSH64XC]  = SH64_OPS_IMM,
  [LSH64XY]  = ALU_OPS_REG,
  [RSH64XC]  = SH64_OPS_IMM,
  [RSH64XY]  = ALU_OPS_REG,
  [MOV64XC]  = ALU_OPS_IMM,
  [MOV64XY]  = ALU_OPS_REG,
  [ARSH64XC] = SH64_OPS_IMM,
  [ARSH64XY] = ALU_OPS_REG,
  [ADD32XC]  = ALU_OPS_IMM,
  [ADD32XY]  = ALU_OPS_REG,
  [LSH32XC]  = SH32_OPS_IMM,
  [LSH32XY]  = ALU_OPS_REG,
  [RSH32XC]  = SH32_OPS_IMM,
  [RSH32XY]  = ALU_OPS_REG,
  [MOV32XC]  = ALU_OPS_IMM,
  [MOV32XY]  = ALU_OPS_REG,
  [ARSH32XC] = SH32_OPS_IMM,
  [ARSH32XY] = ALU_OPS_REG,
  [LE]       = BYTESWAP,
  [BE]       = BYTESWAP,
  [JA]       = JA_OPS,
  [JEQXC]    = JMP_OPS_IMM,
  [JEQXY]    = JMP_OPS_REG,
  [JGTXC]    = JMP_OPS_IMM,
  [JGTXY]    = JMP_OPS_REG,
  [JSGTXC]   = JMP_OPS_IMM,
  [JSGTXY]   = JMP_OPS_REG,
  [EXIT]     = UNUSED_OPS,
};
#undef FSTOP
#undef SNDOP
#undef TRDOP
#undef ALU_OPS_IMM
#undef ALU_OPS_REG
#undef BYTESWAP
#undef JMP_OPS_IMM
#undef JMP_OPS_REG
#undef UNUSED_OPS

class prog_state: public prog_state_base {
 public:
  prog_state() {regs.resize(NUM_REGS, 0);}
};

class inst: public inst_base {
 public:
  inst(int opcode = NOP, int32_t arg1 = 0, int32_t arg2 = 0, int32_t arg3 = 0) {
    _args.resize(MAX_OP_LEN);
    _opcode  = opcode;
    _args[0] = arg1;
    _args[1] = arg2;
    _args[2] = arg3;
  }
  void set_operand(int op_index, op_t op_value);
  inst& operator=(const inst &rhs);
  bool operator==(const inst &x) const;
  string opcode_to_str(int) const;
  void print() const;
  vector<int> get_reg_list() const;
  int32_t get_max_operand_val(int op_index, int inst_index = 0) const;
  int get_jmp_dis() const;
  void insert_jmp_opcodes(unordered_set<int>& jmp_set) const;
  int inst_output_opcode_type() const;
  int inst_output() const;
  bool is_real_inst() const;
  void set_as_nop_inst();
  unsigned int get_input_reg() const {return 1;}
  int get_num_operands() const {return num_operands[_opcode];}
  int get_insn_num_regs() const {return insn_num_regs[_opcode];}
  int get_opcode_type() const {return opcode_type[_opcode];}
  // smt
  z3::expr smt_inst(smt_var& sv) const;
  z3::expr smt_inst_jmp(smt_var& sv) const;
};

int64_t interpret(inst* program, int length, prog_state &ps, int64_t input = 0);
