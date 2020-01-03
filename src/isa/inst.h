#pragma once

#include <vector>
#include <bitset>

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

// TODO
// For absolute coding of each instruction
typedef bitset<20> abs_bv_inst;

class inst {
 public:
  int _opcode;
  vector<int> _args;
  inst() {}
  void print() const;
  virtual string opcode_to_str(int) const {return "";}
  // TODO
  virtual abs_bv_inst inst_to_abs_bv() const {abs_bv_inst bv(""); return bv;};
  vector<int> get_reg_list() const;
  bool operator==(const inst &x) const;
  inst& operator=(const inst &rhs);
  virtual int get_max_operand_val(int op_index, int inst_index = 0) const {return 0;}
  int get_operand(int op_index) const;
  void set_operand(int op_index, int op_value);
  int get_opcode() const;
  void set_opcode(int op_value);
  virtual int get_jmp_dis() const {return 0;}
  // assume `inst_end` is the end instruction of a program
  virtual int inst_output_opcode_type() const {return 0;}
  virtual int inst_output() const {return 0;}
  virtual bool is_real_inst() const {return false;}
  // for class toy_isa
  virtual int get_num_regs() const {return 0;}
  virtual int get_max_prog_len() const {return 0;}
  virtual int get_max_op_len() const {return 0;}
  virtual int get_num_instr() const {return 0;}
  virtual int get_num_operands() const {return 0;}
  virtual int get_insn_num_regs() const {return 0;}
  virtual int get_opcode_type() const {return 0;}
  virtual int interpret(int length, prog_state &ps, int input) {return 0;}
};
