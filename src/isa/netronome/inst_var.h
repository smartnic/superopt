#pragma once

#include "../../../src/isa/inst_var.h"

using namespace std;

/*
General purpose registers: 16 A bank registers followed by 16 B bank registers in order,
i.e. regs[0] = register a0, regs[1] = register a1, ... regs[15] = register a15, regs[16] = register b0, ... regs[31] = register b15
*/
static constexpr int NUM_REGS = 32;

class smt_var: public smt_var_base {
 public:
  smt_var(unsigned int prog_id, unsigned int node_id, unsigned int num_regs)
    : smt_var_base(prog_id, node_id, num_regs) {};
  ~smt_var() {};
  void get_from_previous_block(smt_var& sv) {};
};

class prog_state: public prog_state_base {
 public:
  int _unsigned_carry = 0;

  prog_state() {_regs.resize(NUM_REGS, 0);}

  friend ostream& operator<<(ostream& out, const prog_state& ps) {
    out << "Registers:[";
    for (int i = 0; i < NUM_REGS; i++) {
      if (i > 0) out << ",";
      out << ps._regs[i];
    }
    out << "] ";
    out << "Carry bit:" << ps._unsigned_carry;
    return out;
  }

};

class inout_t: public inout_t_base {
 public:
  int reg;
  void clear() {reg = 0;}
  void init() {}
  bool operator==(const inout_t &rhs) const {return (reg == rhs.reg);}
  friend ostream& operator<<(ostream& out, const inout_t& x) {
    out << x.reg;
    return out;
  }
};

void get_cmp_lists(vector<reg_t>& val_list1, vector<reg_t>& val_list2,
                   inout_t& output1, inout_t& output2);
