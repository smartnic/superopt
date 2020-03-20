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

// convert string s into expr e
// if e = "true"/"false" the type of e is bool_val
// else the type of e is int_const
z3::expr string_to_expr(string s);
z3::expr to_bool_expr(string s);
z3::expr to_expr(int64_t x, unsigned sz = NUM_REG_BITS);
z3::expr to_expr(uint64_t x, unsigned sz = NUM_REG_BITS);
z3::expr to_expr(int32_t x, unsigned sz = NUM_REG_BITS);
z3::expr to_expr(string s, unsigned sz);

class smt_stack {
 private:
  bool is_equal(z3::expr e1, z3::expr e2);
 public:
  vector<z3::expr> addr; // 64-bit bitvector
  vector<z3::expr> val;  // 8-bit bitvector
  void add(z3::expr a, z3::expr v) {addr.push_back(a); val.push_back(v);}
  void clear() {addr.clear(); val.clear();}
  smt_stack& operator=(const smt_stack &rhs);
  bool operator==(const smt_stack &rhs);
  friend ostream& operator<<(ostream& out, const smt_stack& s);
};

class mem_wt {
 public:
  smt_stack _wt; // write table, each element is for write instructions
  smt_stack _uwt; // uninitalized write table, each element is for read before write instructions
  bool _allow_uw; // allow read before write
  mem_wt(bool allow_uw = false) {_allow_uw = allow_uw;}
  void clear() {_wt.clear(); _uwt.clear();}
};

class smt_mem {
 public:
  mem_wt _stack;
  smt_mem() {_stack._allow_uw = false;}
  void clear() {_stack.clear();}
};

// SMT Variable format
// register: r_[prog_id]_[node_id]_[reg_id]_[version_id]
class smt_var {
 private:
  // _name: [prog_id]_[node_id]
  string _name;
  // store the curId
  vector<unsigned int> reg_cur_id;
  vector<z3::expr> reg_var;
 public:
  smt_mem mem_var;
  // 1. Convert prog_id and node_id into _name, that is string([prog_id]_[node_id])
  // 2. Initialize reg_val[i] = r_[_name]_0, i = 0, ..., num_regs
  smt_var(unsigned int prog_id, unsigned int node_id, unsigned int num_regs);
  ~smt_var();
  // inital value for [versionId] is 0, and increases when updated
  z3::expr update_reg_var(unsigned int reg_id);
  z3::expr get_cur_reg_var(unsigned int reg_id);
  z3::expr get_init_reg_var(unsigned int reg_id);
  void clear();
};
