#pragma once

#include <vector>
#include <unordered_set>
#include "../../src/utils.h"
#include "../../src/verify/smt_var.h"

using namespace std;

enum ISA_TYPES {
  TOY_ISA = 0,
  EBPF,
};

// Opcode types for instructions
#define OP_NOP 0
#define OP_RET 1
#define OP_UNCOND_JMP 2
#define OP_COND_JMP 3
#define OP_OTHERS 4
#define OP_ST 5
#define OP_LD 6

// Return opcode types for the end instruction of a program
#define RET_C 0   // return immediate number
#define RET_X 1   // return register

class prog_state_base {
  int _pc = 0; /* Assume only straight line code execution for now */
 public:
  vector<reg_t> _regs; /* assume only registers for now */
  void print();
  void clear();
};

#define RAISE_EXCEPTION(x) {\
  string err_msg = string(x) + string(" has not been implemented"); \
  cerr << err_msg << endl;\
  throw (err_msg); \
}

class inst_base {
 public:
  int _opcode;
  vector<op_t> _args;
  inst_base() {}
  void to_abs_bv(vector<op_t>& abs_vec) const;
  int get_operand(int op_index) const;
  void set_operand(int op_index, op_t op_value);
  int get_opcode() const;
  int get_opcode_by_idx(int idx) const;
  void set_opcode(int op_value);

  /* Functions class inst should support */
  // inst& operator=(const inst &rhs)
  bool operator==(const inst_base &x) const {RAISE_EXCEPTION("inst::operator==");}
  void print() const {RAISE_EXCEPTION("inst::print");}
  vector<int> get_reg_list() const {RAISE_EXCEPTION("inst::get_reg_list");}
  string opcode_to_str(int) const {RAISE_EXCEPTION("inst::opcode_to_str");}
  op_t get_max_operand_val(int op_index, int inst_index = 0) const {RAISE_EXCEPTION("inst::get_max_operand_val");}
  int get_jmp_dis() const {RAISE_EXCEPTION("inst::get_jmp_dis");}
  // insert all jmp opcode in jmp_set, used by proposals.cc to
  // avoid jumps in the last line of the program
  void insert_jmp_opcodes(unordered_set<int>& jmp_set) const {RAISE_EXCEPTION("inst::insert_jmp_opcodes");}
  int inst_output_opcode_type() const {RAISE_EXCEPTION("inst::inst_output_opcode_type");}
  int inst_output() const {RAISE_EXCEPTION("inst::inst_output");}
  bool is_real_inst() const {RAISE_EXCEPTION("inst::is_real_inst");}
  void set_as_nop_inst() {RAISE_EXCEPTION("inst::set_as_nop_inst");}
  unsigned int get_input_reg() const {RAISE_EXCEPTION("inst::get_input_reg");}
  int get_num_operands() const {RAISE_EXCEPTION("inst::get_num_operands");}
  int get_insn_num_regs() const {RAISE_EXCEPTION("inst::get_insn_num_regs");}
  int get_opcode_type() const {RAISE_EXCEPTION("inst::get_opcode_type");}
  // smt
  // return SMT for the given OP_OTHERS type instruction, other types return false
  z3::expr smt_inst(smt_var& sv) const {RAISE_EXCEPTION("inst::smt_inst");}
  // return SMT for the given OP_COND_JMP type instruction, other types return false
  z3::expr smt_inst_jmp(smt_var& sv) const {RAISE_EXCEPTION("inst::smt_inst_jmp");}
};

struct instHash {
  size_t operator()(const inst_base &x) const;
};

/* inst.cc should support */
// int interpret(inst* program, int length, prog_state &ps, int input);

