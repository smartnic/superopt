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

#define NULL_ADDR to_expr(0)

// convert string s into expr e
// if e = "true"/"false" the type of e is bool_val
// else the type of e is int_const
z3::expr string_to_expr(string s);
z3::expr to_bool_expr(string s);
z3::expr to_expr(int64_t x, unsigned sz = NUM_REG_BITS);
z3::expr to_expr(uint64_t x, unsigned sz = NUM_REG_BITS);
z3::expr to_expr(int32_t x, unsigned sz = NUM_REG_BITS);
z3::expr to_expr(string s, unsigned sz);

class mem_range {
 public:
  z3::expr start = string_to_expr("true"); // start address, 64-bit bitvector
  z3::expr end = string_to_expr("true"); // end address, 64-bit bitvector
  mem_range() {}
  mem_range(z3::expr s, z3::expr e) {
    start = s;
    end = e;
  }
  void set_range(z3::expr s, z3::expr e) {
    start = s;
    end = e;
  }
};

class mem_layout {
 public:
  mem_range _stack;
  vector<mem_range> _maps;

  void add_map(z3::expr s, z3::expr e) {_maps.push_back(mem_range(s, e));}
  void set_stack_range(z3::expr s, z3::expr e) {_stack.set_range(s, e);}
};

class smt_wt {
 private:
  bool is_equal(z3::expr e1, z3::expr e2);
 public:
  vector<z3::expr> addr; // 64-bit bitvector
  vector<z3::expr> val;  // 8-bit bitvector
  void add(z3::expr a, z3::expr v) {addr.push_back(a); val.push_back(v);}
  void clear() {addr.clear(); val.clear();}
  smt_wt& operator=(const smt_wt &rhs);
  bool operator==(const smt_wt &rhs);
  friend ostream& operator<<(ostream& out, const smt_wt& s);
};

class mem_wt {
 public:
  smt_wt _wt; // write table, each element is for write instructions
  smt_wt _urt; // uninitalized read table, each element is for read before write instructions
  void clear() {_wt.clear(); _urt.clear();}
};

class smt_map_wt {
 public:
  vector<z3::expr> addr_map; // 64-bit bitvector
  vector<z3::expr> key;
  vector<z3::expr> addr_v;
  void add(z3::expr a, z3::expr k, z3::expr v) {
    addr_map.push_back(a);
    key.push_back(k);
    addr_v.push_back(v);
  }
  void clear() {addr_map.clear(); key.clear(); addr_v.clear();}
  friend ostream& operator<<(ostream& out, const smt_map_wt& s);
};

class map_wt {
 public:
  smt_map_wt _wt;
  smt_map_wt _urt;
  void clear() {_wt.clear(); _urt.clear();}
};

class smt_mem {
 public:
  mem_wt _mem_table;
  map_wt _map_table;
  vector<z3::expr> _addrs_map_v_next;

  smt_mem() {}
  void init_addrs_map_v_next(mem_layout& m_layout);
  z3::expr get_and_update_addr_v_next(int map_id);
  void clear() {_mem_table.clear(); _map_table.clear(); _addrs_map_v_next.clear();}
  friend ostream& operator<<(ostream& out, const smt_mem& s);
};

// SMT Variable format
// register: r_[prog_id]_[node_id]_[reg_id]_[version_id]
// map key: k_[prog_id]_[node_id]_[version_id]
// map value: v_[prog_id]_[node_id]_[version_id]
// map address of value: av_[prog_id]_[node_id]_[version_id]
class smt_var {
 private:
  // _name: [prog_id]_[node_id]
  string _name;
  // store the curId
  vector<unsigned int> reg_cur_id;
  vector<z3::expr> reg_var;
  // store the curId of map related variables
  unsigned int key_cur_id, val_cur_id, addr_v_cur_id;
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
  // map related functions
  z3::expr update_key();
  z3::expr update_val();
  z3::expr update_addr_v();
  void clear();
};
