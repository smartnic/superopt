#pragma once

#include "z3++.h"
#include "../../src/isa/inst.h"

using namespace z3;

// For most applications this is sufficient. An application may use multiple Z3 contexts.
// Objects created in one context cannot be used in another one.
// reference: https://github.com/Z3Prover/z3/blob/master/src/api/python/z3/z3.py
extern context smt_c;

// SMT Variable format
// [type]_[prog_id]_[node_id]_[reg_id/mem_id]_[version_id]
// [type]: r means register; m means memory
class smt_var {
 private:
  string _name;
  // store the curId
  vector<unsigned int> reg_cur_id;
  std::vector<expr> reg_var;
 public:
  // 1. Convert prog_id and node_id into _name, that is string([prog_id]_[node_id])
  // 2. Initialize reg_val[i] = r_[_name]_0, i = 0, ..., num_regs
  smt_var(unsigned int prog_id, unsigned int node_id, unsigned int num_regs);
  ~smt_var();
  // inital value for [versionId] is 0, and increases when updated
  expr update_reg_var(unsigned int reg_id);
  expr get_cur_reg_var(unsigned int reg_id);
  expr get_init_reg_var(unsigned int reg_id);
};

// convert string s into expr e
// if e = "true"/"false" the type of e is bool_val
// else the type of e is int_const
expr string_to_expr(string s);
