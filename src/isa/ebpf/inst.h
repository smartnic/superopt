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
  JA       = OPCODE_BPF_JMP_A,
  JEQXC    = OPCODE_BPF_JMP_IMM(BPF_JEQ),
  JEQXY    = OPCODE_BPF_JMP_REG(BPF_JEQ),
  JGTXC    = OPCODE_BPF_JMP_IMM(BPF_JGT),
  JGTXY    = OPCODE_BPF_JMP_REG(BPF_JGT),
  JSGTXC   = OPCODE_BPF_JMP_IMM(BPF_JSGT),
  JSGTXY   = OPCODE_BPF_JMP_REG(BPF_JSGT),
  EXIT     = OPCODE_BPF_EXIT_INSN,
};

static const int IDX_2_OPCODE[NUM_INSTR] = {
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
  [IDX_JA] = JA,
  [IDX_JEQXC] = JEQXC,
  [IDX_JEQXY] = JEQXY,
  [IDX_JGTXC] = JGTXC,
  [IDX_JGTXY] = JGTXY,
  [IDX_JSGTXC] = JSGTXC,
  [IDX_JSGTXY] = JSGTXY,
  [IDX_EXIT] = EXIT,
};

static const unordered_map<int, int> num_operands = {
  {NOP, 0},
  {ADD64XC, 2},
  {ADD64XY, 2},
  {LSH64XC, 2},
  {LSH64XY, 2},
  {RSH64XC, 2},
  {RSH64XY, 2},
  {MOV64XC, 2},
  {MOV64XY, 2},
  {ARSH64XC, 2},
  {ARSH64XY, 2},
  {ADD32XC, 2},
  {ADD32XY, 2},
  {LSH32XC, 2},
  {LSH32XY, 2},
  {RSH32XC, 2},
  {RSH32XY, 2},
  {MOV32XC, 2},
  {MOV32XY, 2},
  {ARSH32XC, 2},
  {ARSH32XY, 2},
  {LE, 2},
  {BE, 2},
  {JA, 1},
  {JEQXC, 3},
  {JEQXY, 3},
  {JGTXC, 3},
  {JGTXY, 3},
  {JSGTXC, 3},
  {JSGTXY, 3},
  {EXIT, 0},
};

// number of registers for each opcode
// e.g., for ADD64XY, two operands are registers
static const unordered_map<int, int> insn_num_regs = {
  {NOP, 0},
  {ADD64XC, 1},
  {ADD64XY, 2},
  {LSH64XC, 1},
  {LSH64XY, 2},
  {RSH64XC, 1},
  {RSH64XY, 2},
  {MOV64XC, 1},
  {MOV64XY, 2},
  {ARSH64XC, 1},
  {ARSH64XY, 2},
  {ADD32XC, 1},
  {ADD32XY, 2},
  {LSH32XC, 1},
  {LSH32XY, 2},
  {RSH32XC, 1},
  {RSH32XY, 2},
  {MOV32XC, 1},
  {MOV32XY, 2},
  {ARSH32XC, 1},
  {ARSH32XY, 2},
  {LE, 1},
  {BE, 1},
  {JA, 0},
  {JEQXC, 1},
  {JEQXY, 2},
  {JGTXC, 1},
  {JGTXY, 2},
  {JSGTXC, 1},
  {JSGTXY, 2},
  {EXIT, 0},
};

static const unordered_map<int, int> opcode_type = {
  {NOP, OP_NOP},
  {ADD64XC, OP_OTHERS},
  {ADD64XY, OP_OTHERS},
  {LSH64XC, OP_OTHERS},
  {LSH64XY, OP_OTHERS},
  {RSH64XC, OP_OTHERS},
  {RSH64XY, OP_OTHERS},
  {MOV64XC, OP_OTHERS},
  {MOV64XY, OP_OTHERS},
  {ARSH64XC, OP_OTHERS},
  {ARSH64XY, OP_OTHERS},
  {ADD32XC, OP_OTHERS},
  {ADD32XY, OP_OTHERS},
  {LSH32XC, OP_OTHERS},
  {LSH32XY, OP_OTHERS},
  {RSH32XC, OP_OTHERS},
  {RSH32XY, OP_OTHERS},
  {MOV32XC, OP_OTHERS},
  {MOV32XY, OP_OTHERS},
  {ARSH32XC, OP_OTHERS},
  {ARSH32XY, OP_OTHERS},
  {LE, OP_OTHERS},
  {BE, OP_OTHERS},
  {JA, OP_UNCOND_JMP},
  {JEQXC, OP_COND_JMP},
  {JEQXY, OP_COND_JMP},
  {JGTXC, OP_COND_JMP},
  {JGTXY, OP_COND_JMP},
  {JSGTXC, OP_COND_JMP},
  {JSGTXY, OP_COND_JMP},
  {EXIT, OP_RET},
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
#define OPTYPE(opcode, opindex) ((optable.find(opcode)->second >> ((opindex) * 5)) & 31)
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
static const unordered_map<int, int> optable = {
  {NOP, UNUSED_OPS},
  {ADD64XC, ALU_OPS_IMM},
  {ADD64XY, ALU_OPS_REG},
  {LSH64XC, SH64_OPS_IMM},
  {LSH64XY, ALU_OPS_REG},
  {RSH64XC, SH64_OPS_IMM},
  {RSH64XY, ALU_OPS_REG},
  {MOV64XC, ALU_OPS_IMM},
  {MOV64XY, ALU_OPS_REG},
  {ARSH64XC, SH64_OPS_IMM},
  {ARSH64XY, ALU_OPS_REG},
  {ADD32XC, ALU_OPS_IMM},
  {ADD32XY, ALU_OPS_REG},
  {LSH32XC, SH32_OPS_IMM},
  {LSH32XY, ALU_OPS_REG},
  {RSH32XC, SH32_OPS_IMM},
  {RSH32XY, ALU_OPS_REG},
  {MOV32XC, ALU_OPS_IMM},
  {MOV32XY, ALU_OPS_REG},
  {ARSH32XC, SH32_OPS_IMM},
  {ARSH32XY, ALU_OPS_REG},
  {LE,  BYTESWAP},
  {BE,  BYTESWAP},
  {JA,  JA_OPS},
  {JEQXC, JMP_OPS_IMM},
  {JEQXY, JMP_OPS_REG},
  {JGTXC, JMP_OPS_IMM},
  {JGTXY, JMP_OPS_REG},
  {JSGTXC,  JMP_OPS_IMM},
  {JSGTXY,  JMP_OPS_REG},
  {EXIT,  UNUSED_OPS},
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
  int get_opcode_by_idx(int idx) const;
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
  int get_num_operands() const;
  int get_insn_num_regs() const;
  int get_opcode_type() const;
  // smt
  z3::expr smt_inst(smt_var& sv) const;
  z3::expr smt_inst_jmp(smt_var& sv) const;
};

int opcode_2_idx(int opcode);
int64_t interpret(inst* program, int length, prog_state &ps, int64_t input = 0);
