#pragma once

#include "../../../src/isa/inst_var.h"

using namespace std;

static constexpr int NUM_REGS = 4;

class smt_var: public smt_var_base {
 public:
  z3::expr ret_val = to_expr("ret_val");
  smt_var(): smt_var_base() {}
  smt_var(unsigned int prog_id, unsigned int node_id, unsigned int num_regs)
    : smt_var_base(prog_id, node_id, num_regs) {
    ret_val = to_expr("ret_val_" + to_string(prog_id));
  };
  ~smt_var() {};
  void init(unsigned int prog_id, unsigned int node_id, unsigned int num_regs, unsigned int n_blocks = 1) {
    smt_var_base::init(prog_id, node_id, num_regs);
    ret_val = to_expr("ret_val_" + to_string(prog_id));
  }
};

class smt_var_bl {
 public:
  void store_state_before_smt_block(smt_var& sv) {}
  z3::expr gen_smt_after_smt_block(smt_var& sv, z3::expr& pc) {return Z3_true;}
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
void gen_random_input(vector<inout_t>& inputs, int reg_min, int reg_max);
