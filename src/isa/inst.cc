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

void inst::to_abs_bv(vector<int>& abs_vec) const {
  const int num_args = _args.size();
  abs_vec.push_back(_opcode);
  for (int i = 0; i < num_args; i++) {
    abs_vec.push_back(_args[i]);
  }
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
    _args[i] = rhs._args[i];
  }
  return *this;
}

int inst::get_operand(int op_index) const {
  assert(op_index < _args.size());
  return _args[op_index];
}

void inst::set_operand(int op_index, int64_t op_value) {
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

void inst::convert_to_pointers(vector<inst*> &instptr_list, inst* instruction) const {
  for (int i = 0; i < instptr_list.size(); i++) {
    instptr_list[i] = &instruction[i];
  }
}

size_t instHash::operator()(const inst &x) const {
  size_t res = hash<int>()(x._opcode);
  for (int i = 0; i < x._args.size(); i++) {
    res ^= hash<int>()(x._args[i]) << (i + 1);
  }
  return res;
}
