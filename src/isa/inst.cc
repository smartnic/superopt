#include <iostream>
#include <cassert>
#include "inst.h"

using namespace std;

void prog_state::print() {
  for (int i = 0; i < regs.size(); i++) {
    cout << "Register "  << i << " " << regs[i] << endl;
  }
};

void prog_state::clear() {
  pc = 0;
  for (int i = 0; i < regs.size(); i++) {
    regs[i] = 0;
  }
};

void inst::print() const {
  cout << opcode_to_str(_opcode);
  for (int i = 0; i < get_num_operands(); i++) {
    cout << " " << _args[i];
  }
  cout << endl;
}

vector<int> inst::get_reg_list() const {
  vector<int> reg_list;
  for (int i = 0; i < get_insn_num_regs(); i++)
    reg_list.push_back(_args[i]);
  return reg_list;
}

bool inst::operator==(const inst &x) const {
  bool res = (_opcode  == x._opcode);
  for (int i = 0; i < get_max_op_len(); i++) {
    res = res && (_args[i] == x._args[i]);
  }
  return res;
}

inst& inst::operator=(const inst &rhs) {
  _opcode = rhs._opcode;
  for (int i = 0; i < rhs._args.size(); i++) {
    _args[0] = rhs._args[0];
    _args[1] = rhs._args[1];
    _args[2] = rhs._args[2];
  }
  return *this;
}

int inst::get_operand(int op_index) const {
  assert(op_index < _args.size());
  return _args[op_index];
}

void inst::set_operand(int op_index, int op_value) {
  assert(op_index < _args.size());
  _args[op_index] = op_value;
}

int inst::get_opcode() const {
  return _opcode;
}

void inst::set_opcode(int op_value) {
  assert(op_value < get_num_instr());
  _opcode = op_value;
}
