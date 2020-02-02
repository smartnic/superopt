#include <iostream>
#include <cassert>
#include "inst.h"

using namespace std;

void prog_state_base::print() {
  for (int i = 0; i < regs.size(); i++) {
    cout << "Register "  << i << " " << regs[i] << endl;
  }
};

void prog_state_base::clear() {
  pc = 0;
  for (int i = 0; i < regs.size(); i++) {
    regs[i] = 0;
  }
};

void inst_base::to_abs_bv(vector<int>& abs_vec) const {
  const int num_args = _args.size();
  abs_vec.push_back(_opcode);
  for (int i = 0; i < num_args; i++) {
    abs_vec.push_back(_args[i]);
  }
}

bool inst_base::operator==(const inst_base &x) const {
  bool res = (_opcode  == x._opcode);
  for (int i = 0; i < _args.size(); i++) {
    res = res && (_args[i] == x._args[i]);
  }
  return res;
}

inst_base& inst_base::operator=(const inst_base &rhs) {
  _opcode = rhs._opcode;
  for (int i = 0; i < rhs._args.size(); i++) {
    _args[i] = rhs._args[i];
  }
  return *this;
}

int inst_base::get_operand(int op_index) const {
  assert(op_index < _args.size());
  return _args[op_index];
}

void inst_base::set_operand(int op_index, op_t op_value) {
  assert(op_index < _args.size());
  _args[op_index] = op_value;
}

int inst_base::get_opcode() const {
  return _opcode;
}

void inst_base::set_opcode(int op_value) {
  _opcode = op_value;
}

size_t instHash::operator()(const inst_base &x) const {
  size_t res = hash<int>()(x._opcode);
  for (int i = 0; i < x._args.size(); i++) {
    res ^= hash<int>()(x._args[i]) << (i + 1);
  }
  return res;
}
