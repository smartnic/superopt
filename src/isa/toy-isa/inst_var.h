#pragma once

#include "../../../src/isa/inst_var.h"

using namespace std;

static constexpr int NUM_REGS = 4;

class smt_var: public smt_var_base {
 public:
  smt_var(unsigned int prog_id, unsigned int node_id, unsigned int num_regs)
    : smt_var_base(prog_id, node_id, num_regs) {};
  ~smt_var() {};
  void get_from_previous_block(smt_var& sv) {};
};

class prog_state: public prog_state_base {
 public:
  prog_state() {_regs.resize(NUM_REGS, 0);}
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

void get_cmp_lists(vector<int>& val_list1, vector<int>& val_list2,
                   inout_t& output1, inout_t& output2);
