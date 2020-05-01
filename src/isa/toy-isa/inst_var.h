#pragma once

#include "../../../src/isa/inst_var.h"

using namespace std;

class smt_var: public smt_var_base {
 public:
  smt_var(unsigned int prog_id, unsigned int node_id, unsigned int num_regs)
    : smt_var_base(prog_id, node_id, num_regs) {};
  ~smt_var() {};
  void get_from_previous_block(smt_var& sv) {};
};
