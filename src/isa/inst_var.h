#pragma once

#include <string>
#include <vector>
#include "z3++.h"
#include "../../src/utils.h"

using namespace std;

// For most applications this is sufficient. An application may use multiple Z3 contexts.
// Objects created in one context cannot be used in another one.
// reference: https://github.com/Z3Prover/z3/blob/master/src/api/python/z3/z3.py
extern z3::context smt_c;

#define Z3_true string_to_expr("true")
#define Z3_false string_to_expr("false")

// convert string s into expr e
// if e = "true"/"false" the type of e is bool_val
// else the type of e is int_const
z3::expr string_to_expr(string s);
z3::expr to_bool_expr(string s);
z3::expr to_expr(int64_t x, unsigned sz = NUM_REG_BITS);
z3::expr to_expr(uint64_t x, unsigned sz = NUM_REG_BITS);
z3::expr to_expr(int32_t x, unsigned sz = NUM_REG_BITS);
z3::expr to_expr(string s, unsigned sz);

// SMT Variable format
// register: r_[prog_id]_[node_id]_[reg_id]_[version_id]
class smt_var_base {
 protected:
  // _name: [prog_id]_[node_id]
  string _name;
  // store the curId
  vector<unsigned int> reg_cur_id;
  vector<z3::expr> reg_var;
 public:
  // 1. Convert prog_id and node_id into _name, that is string([prog_id]_[node_id])
  // 2. Initialize reg_val[i] = r_[_name]_0, i = 0, ..., num_regs
  smt_var_base(unsigned int prog_id, unsigned int node_id, unsigned int num_regs);
  ~smt_var_base();
  // inital value for [versionId] is 0, and increases when updated
  z3::expr update_reg_var(unsigned int reg_id);
  z3::expr get_cur_reg_var(unsigned int reg_id);
  z3::expr get_init_reg_var(unsigned int reg_id);
  void init() {}
  void clear();
};

class prog_state_base {
  int _pc = 0; /* Assume only straight line code execution for now */
 public:
  vector<reg_t> _regs; /* assume only registers for now */
  void print() const;
  void clear();
};

class inout_t_base {
 public:
  void clear() {RAISE_EXCEPTION("inout_t::clear()");}
  void init() {RAISE_EXCEPTION("inout_t::init()");}
  bool operator==(const inout_t_base &rhs) const {RAISE_EXCEPTION("inout_t::operator==");}
  friend ostream& operator<<(ostream& out, const inout_t_base& x) {RAISE_EXCEPTION("inout_t::operator<<");}
};
