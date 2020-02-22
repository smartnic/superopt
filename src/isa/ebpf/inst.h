#pragma once

#include <vector>
#include <unordered_set>
#include <unordered_map>
#include "z3++.h"
#include "bpf.h"
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

enum OPCODE_IDX {
  IDX_NOP = 0,
  // ALU64
  IDX_ADD64XC,
  IDX_ADD64XY,
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
  // Memory
  IDX_LDXW,
  IDX_STXW,
  // JMP
  IDX_JA,
  IDX_JEQXC,
  IDX_JEQXY,
  IDX_JGTXC,
  IDX_JGTXY,
  IDX_JSGTXC,
  IDX_JSGTXY,
  // Exit
  IDX_EXIT,
  NUM_INSTR, // Number of opcode types
};

#define OPCODE_BPF_ALU64_IMM(OP) BPF_ALU64 | BPF_OP(OP) | BPF_K
#define OPCODE_BPF_ALU64_REG(OP) BPF_ALU64 | BPF_OP(OP) | BPF_X
#define OPCODE_BPF_ALU32_IMM(OP) BPF_ALU | BPF_OP(OP) | BPF_K
#define OPCODE_BPF_ALU32_REG(OP) BPF_ALU | BPF_OP(OP) | BPF_X
#define OPCODE_BPF_ENDIAN(TYPE) BPF_ALU | BPF_END | BPF_SRC(TYPE)
#define OPCODE_BPF_MOV64_IMM BPF_ALU64 | BPF_MOV | BPF_K
#define OPCODE_BPF_MOV64_REG BPF_ALU64 | BPF_MOV | BPF_X
#define OPCODE_BPF_MOV32_IMM BPF_ALU | BPF_MOV | BPF_K
#define OPCODE_BPF_MOV32_REG BPF_ALU | BPF_MOV | BPF_X
#define OPCODE_BPF_LDX_MEM(SIZE) BPF_LDX | BPF_SIZE(SIZE) | BPF_MEM
#define OPCODE_BPF_STX_MEM(SIZE) BPF_STX | BPF_SIZE(SIZE) | BPF_MEM
#define OPCODE_BPF_JMP_IMM(OP) BPF_JMP | BPF_OP(OP) | BPF_K
#define OPCODE_BPF_JMP_REG(OP) BPF_JMP | BPF_OP(OP) | BPF_X
#define OPCODE_BPF_JMP_A BPF_JMP | BPF_JA
#define OPCODE_BPF_EXIT_INSN BPF_JMP | BPF_EXIT

// Instruction opcodes
enum OPCODES {
  NOP = 0,
  ADD64XC  = OPCODE_BPF_ALU64_IMM(BPF_ADD),
  ADD64XY  = OPCODE_BPF_ALU64_REG(BPF_ADD),
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
  LDXW     = OPCODE_BPF_LDX_MEM(BPF_W),
  STXW     = OPCODE_BPF_STX_MEM(BPF_W),
  JA       = OPCODE_BPF_JMP_A,
  JEQXC    = OPCODE_BPF_JMP_IMM(BPF_JEQ),
  JEQXY    = OPCODE_BPF_JMP_REG(BPF_JEQ),
  JGTXC    = OPCODE_BPF_JMP_IMM(BPF_JGT),
  JGTXY    = OPCODE_BPF_JMP_REG(BPF_JGT),
  JSGTXC   = OPCODE_BPF_JMP_IMM(BPF_JSGT),
  JSGTXY   = OPCODE_BPF_JMP_REG(BPF_JSGT),
  EXIT     = OPCODE_BPF_EXIT_INSN,
};

int opcode_2_idx(int opcode);
static const int idx_2_opcode[NUM_INSTR] = {
  [IDX_NOP] = NOP,
  [IDX_ADD64XC] = ADD64XC,
  [IDX_ADD64XY] = ADD64XY,
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
  [IDX_LDXW] = LDXW,
  [IDX_STXW] = STXW,
  [IDX_JA] = JA,
  [IDX_JEQXC] = JEQXC,
  [IDX_JEQXY] = JEQXY,
  [IDX_JGTXC] = JGTXC,
  [IDX_JGTXY] = JGTXY,
  [IDX_JSGTXC] = JSGTXC,
  [IDX_JSGTXY] = JSGTXY,
  [IDX_EXIT] = EXIT,
};

static const int num_operands[NUM_INSTR] = {
  [IDX_NOP]      = 0,
  [IDX_ADD64XC]  = 2,
  [IDX_ADD64XY]  = 2,
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
  [IDX_LDXW]     = 3,
  [IDX_STXW]     = 3,
  [IDX_JA]       = 1,
  [IDX_JEQXC]    = 3,
  [IDX_JEQXY]    = 3,
  [IDX_JGTXC]    = 3,
  [IDX_JGTXY]    = 3,
  [IDX_JSGTXC]   = 3,
  [IDX_JSGTXY]   = 3,
  [IDX_EXIT]     = 0,
};

// number of registers for each opcode
// e.g., for ADD64XY, two operands are registers
static const int insn_num_regs[NUM_INSTR] = {
  [IDX_NOP]      = 0,
  [IDX_ADD64XC]  = 1,
  [IDX_ADD64XY]  = 2,
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
  [IDX_LDXW]     = 2,
  [IDX_STXW]     = 2,
  [IDX_JA]       = 0,
  [IDX_JEQXC]    = 1,
  [IDX_JEQXY]    = 2,
  [IDX_JGTXC]    = 1,
  [IDX_JGTXY]    = 2,
  [IDX_JSGTXC]   = 1,
  [IDX_JSGTXY]   = 2,
  [IDX_EXIT]     = 0,
};

static const int opcode_type[NUM_INSTR] = {
  [IDX_NOP]      = OP_NOP,
  [IDX_ADD64XC]  = OP_OTHERS,
  [IDX_ADD64XY]  = OP_OTHERS,
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
  [IDX_LDXW]     = OP_LD,
  [IDX_STXW]     = OP_ST,
  [IDX_JA]       = OP_UNCOND_JMP,
  [IDX_JEQXC]    = OP_COND_JMP,
  [IDX_JEQXY]    = OP_COND_JMP,
  [IDX_JGTXC]    = OP_COND_JMP,
  [IDX_JGTXY]    = OP_COND_JMP,
  [IDX_JSGTXC]   = OP_COND_JMP,
  [IDX_JSGTXY]   = OP_COND_JMP,
  [IDX_EXIT]     = OP_RET,
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
static constexpr int32_t MAX_TYPES_IMM_ENDIAN = 2;

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
#define LDX_OPS (FSTOP(OP_DST_REG) | SNDOP(OP_SRC_REG) | TRDOP(OP_OFF))
#define STX_OPS (FSTOP(OP_DST_REG) | SNDOP(OP_OFF) | TRDOP(OP_SRC_REG))
#define JA_OPS (FSTOP(OP_OFF) | SNDOP(OP_UNUSED) | TRDOP(OP_UNUSED))
#define JMP_OPS_IMM (FSTOP(OP_DST_REG) | SNDOP(OP_IMM) | TRDOP(OP_OFF))
#define JMP_OPS_REG (FSTOP(OP_DST_REG) | SNDOP(OP_SRC_REG) | TRDOP(OP_OFF))
#define UNUSED_OPS (FSTOP(OP_UNUSED) | SNDOP(OP_UNUSED) | TRDOP(OP_UNUSED))
static const int optable[NUM_INSTR] = {
  [IDX_NOP]      = UNUSED_OPS,
  [IDX_ADD64XC]  = ALU_OPS_IMM,
  [IDX_ADD64XY]  = ALU_OPS_REG,
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
  [IDX_LDXW]     = LDX_OPS,
  [IDX_STXW]     = STX_OPS,
  [IDX_JA]       = JA_OPS,
  [IDX_JEQXC]    = JMP_OPS_IMM,
  [IDX_JEQXY]    = JMP_OPS_REG,
  [IDX_JGTXC]    = JMP_OPS_IMM,
  [IDX_JGTXY]    = JMP_OPS_REG,
  [IDX_JSGTXC]   = JMP_OPS_IMM,
  [IDX_JSGTXY]   = JMP_OPS_REG,
  [IDX_EXIT]     = UNUSED_OPS,
};
#undef FSTOP
#undef SNDOP
#undef TRDOP
#undef ALU_OPS_IMM
#undef ALU_OPS_REG
#undef BYTESWAP
#undef LDX_OPS
#undef STX_OPS
#undef JMP_OPS_IMM
#undef JMP_OPS_REG
#undef UNUSED_OPS

class mem_t {
 public:
  static const int MEM_SIZE = 512;
  uint8_t _mem[MEM_SIZE];
  // stack address is the bottom of the stack
  uint64_t _stack_addr = (uint64_t)&_mem[MEM_SIZE - 1] + 1;
};

class prog_state: public prog_state_base {
 public:
  mem_t _mem;
  prog_state() {_regs.resize(NUM_REGS, 0);}
  void print();
  void clear();
};

class inst: public inst_base {
 private:
  void set_imm(int op_value);
  int32_t get_max_imm() const;
  int32_t get_min_imm() const;
  int16_t get_max_off(int inst_index) const;
  int16_t get_min_off() const;
 public:
  int32_t _dst_reg;
  int32_t _src_reg;
  int32_t _imm;
  int16_t _off;
  inst(int opcode, int32_t dst_reg, int32_t src_reg, int32_t imm, int16_t off) {
    _opcode  = opcode;
    _dst_reg = dst_reg;
    _src_reg = src_reg;
    _imm = imm;
    _off = off;
  }
  inst(int opcode = NOP, int32_t arg1 = 0, int32_t arg2 = 0, int32_t arg3 = 0);
  void to_abs_bv(vector<op_t>& abs_vec) const;
  int get_operand(int op_index) const;
  void set_operand(int op_index, op_t op_value);
  int get_opcode_by_idx(int idx) const;
  inst& operator=(const inst &rhs);
  bool operator==(const inst &x) const;
  string opcode_to_str(int) const;
  void print() const;
  vector<int> get_reg_list() const;
  int32_t get_max_operand_val(int op_index, int inst_index = 0) const;
  int32_t get_min_operand_val(int op_index, int inst_index = 0) const;
  int get_jmp_dis() const;
  void insert_jmp_opcodes(unordered_set<int>& jmp_set) const;
  int inst_output_opcode_type() const;
  int inst_output() const;
  bool is_real_inst() const;
  bool is_reg(int op_index) const;
  void set_as_nop_inst();
  unsigned int get_input_reg() const {return 1;}
  int get_num_operands() const {return num_operands[opcode_2_idx(_opcode)];}
  int get_insn_num_regs() const {return insn_num_regs[opcode_2_idx(_opcode)];}
  int get_opcode_type() const {return opcode_type[opcode_2_idx(_opcode)];}
  // smt
  z3::expr smt_inst(smt_var& sv) const;
  z3::expr smt_inst_jmp(smt_var& sv) const;

  string get_bytecode_str() const;
};

int64_t interpret(inst* program, int length, prog_state &ps, int64_t input = 0);
