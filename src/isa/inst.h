#pragma once

#include <vector>
#include <unordered_set>
#include "../../src/verify/smt_var.h"

using namespace std;

// Opcode types for instructions
#define OP_NOP 0
#define OP_RET 1
#define OP_UNCOND_JMP 2
#define OP_COND_JMP 3
#define OP_OTHERS 4

// Return opcode types for the end instruction of a program
#define RET_C 0   // return immediate number
#define RET_X 1   // return register

class prog_state {
  int pc = 0; /* Assume only straight line code execution for now */
 public:
  vector<int> regs; /* assume only registers for now */
  void print();
  void clear();
};

class inst {
 public:
  int _opcode;
  vector<int> _args;
  inst() {}
  void print() const;
  virtual string opcode_to_str(int) const {return "";}
  int to_abs_bv() const;
  vector<int> get_reg_list() const;
  bool operator==(const inst &x) const;
  inst& operator=(const inst &rhs);
  virtual int get_max_operand_val(int op_index, int inst_index = 0) const {return 0;}
  int get_operand(int op_index) const;
  void set_operand(int op_index, int op_value);
  int get_opcode() const;
  void set_opcode(int op_value);
  void convert_to_pointers(vector<inst*> &instptr_list, inst* instruction) const;
  virtual void make_insts(vector<inst*> &instptr_list, const vector<inst*> &other) const {}
  virtual void make_insts(vector<inst*> &instptr_list, const inst* instruction) const {}
  virtual void clear_insts() {}
  virtual int get_jmp_dis() const {return 0;}
  virtual void insert_jmp_opcodes(unordered_set<int>& jmp_sets) const {}
  virtual int inst_output_opcode_type() const {return 0;}
  virtual int inst_output() const {return 0;}
  virtual bool is_real_inst() const {return false;}
  // for class toy_isa
  virtual int get_num_regs() const {return 0;}
  virtual int get_max_prog_len() const {return 0;}
  virtual int get_max_op_len() const {return 0;}
  virtual int get_op_num_bits() const {return 0;}
  virtual int get_num_instr() const {return 0;}
  virtual int get_num_operands() const {return 0;}
  virtual int get_insn_num_regs() const {return 0;}
  virtual int get_opcode_type() const {return 0;}
  virtual int interpret(const vector<inst*> &instptr_list, prog_state &ps, int input) const {return 0;}
  // smt
  // return SMT for the given OP_OTHERS type instruction, other types return false
  virtual z3::expr smt_inst(smt_var& sv) const {return string_to_expr("false");}
  // return SMT for the given OP_COND_JMP type instruction, other types return false
  virtual z3::expr smt_inst_jmp(smt_var& sv) const {return string_to_expr("false");}
};

struct instHash {
  size_t operator()(const inst &x) const;
};
