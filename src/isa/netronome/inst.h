#pragma once

#include <vector>
#include <unordered_set>
#include "z3++.h"
#include "../../../src/isa/inst_var.h"
#include "../../../src/isa/inst.h"
#include "inst_var.h"
#include "inst_codegen.h"

using namespace std;

static constexpr int MAX_PROG_LEN = 7;
// Max number of operands in one instruction
static constexpr int MAX_OP_LEN = 4;

// // Number of bits of a single opcode or operand
// static constexpr int OP_NUM_BITS = 5;
// // Number of bits of a single instruction
// static constexpr int INST_NUM_BITS = 20;

// Instruction opcodes
enum OPCODES {
  NOP = 0,
  IMMED,
  ALU,
  NUM_INSTR // not an opcode, just the count of the number of defined opcodes
};


enum ALU_OPS {
  ALU_PLUS = 0,
  ALU_PLUS_16,
  ALU_PLUS_8,
  // ALU_PLUS_CARRY,
  // ALU_MINUS_CARRY,
  ALU_MINUS,
  ALU_B_MINUS_A,
  ALU_B,
  ALU_INV_B,
  ALU_AND,
  ALU_INV_AND,
  ALU_AND_INV,
  ALU_OR,
  ALU_XOR,
  NUM_ALU_INSTR // number of alu operations
};

static constexpr int num_operands[NUM_INSTR] = {
  [NOP]   = 0,
  [IMMED] = 2,
  [ALU]   = 4,
};

static constexpr int insn_num_regs[NUM_INSTR] = {
  [NOP]   = 0,
  [IMMED] = 1,
};

static constexpr int opcode_type[NUM_INSTR] = {
  [NOP]   = OP_NOP,
  [IMMED] = OP_OTHERS,
  [ALU]   = OP_OTHERS,
};



// Max value for immediate operand
static constexpr int MAX_CONST = 32;
// Operand types for instructions
static constexpr int OP_UNUSED = 0;
static constexpr int OP_REG = 1;
static constexpr int OP_IMM = 2;
static constexpr int OP_OFF = 3;
static constexpr int OP_OPTYPE = 4; // operation subtype (eg. + or - within alu[] )

// /* The definitions below assume a minimum 16-bit integer data type */
#define OPTYPE(opcode, opindex) ((optable[opcode] >> ((opindex) * 4)) & 31)
#define OP1(x) (x)
#define OP2(x) (x << 4)
#define OP3(x) (x << 8)
#define OP4(x) (x << 12)
#define JMP_OPS (OP1(OP_REG) | OP2(OP_REG) | OP3(OP_OFF))
#define UNUSED_OPS (OP1(OP_UNUSED) | OP2(OP_UNUSED) | OP3(OP_UNUSED))
static constexpr int optable[NUM_INSTR] = {
  [NOP]   = UNUSED_OPS,
  [IMMED] = OP1(OP_REG) | OP2(OP_IMM) | OP3(OP_UNUSED),
  [ALU]   = OP1(OP_REG) | OP2(OP_REG) | OP3(OP_OPTYPE) | OP4(OP_REG),
};
#undef OP1
#undef OP2
#undef OP3
#undef JMP_OPS
#undef OP4
#undef UNUSED_OPS

class inst: public inst_base {
 public:
  inst(int opcode = NOP, int arg1 = 0, int arg2 = 0, int arg3 = 0, int arg4 = 0) {
    _args.resize(MAX_OP_LEN);
    _opcode  = opcode;
    _args[0] = arg1;
    _args[1] = arg2;
    _args[2] = arg3;
    _args[3] = arg4;
  }
  inst& operator=(const inst &rhs);
  bool operator==(const inst &x) const;
  string opcode_to_str(int) const;
  void print() const;
  int get_max_operand_val(int op_index, int inst_index = 0) const;
  int get_min_operand_val(int op_index, int inst_index = 0) const;
  int get_jmp_dis() const;
  vector<int> get_canonical_reg_list() const;
  static vector<int> get_isa_canonical_reg_list();
  void insert_jmp_opcodes(unordered_set<int>& jmp_set) const;
  int inst_output_opcode_type() const;
  int inst_output() const;
  bool is_real_inst() const;
  bool is_reg(int op_index) const;
  int implicit_ret_reg() const;
  void set_as_nop_inst();
  // return the register for storing the given input
  unsigned int get_input_reg() const {return 0;}
  int get_num_operands() const {return num_operands[_opcode];}
  int get_insn_num_regs() const {return insn_num_regs[_opcode];}
  int get_opcode_type() const {return opcode_type[_opcode];}
  // smt
  z3::expr smt_inst(smt_var& sv) const;
  z3::expr smt_inst_jmp(smt_var& sv) const;
  static z3::expr smt_set_pre(z3::expr input, smt_var& sv);
};

void interpret(inout_t& output, inst* program, int length, prog_state &ps, const inout_t& input);
