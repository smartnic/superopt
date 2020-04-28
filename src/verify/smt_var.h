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

#define STACK_SIZE 512 // 512 bytes

// convert string s into expr e
// if e = "true"/"false" the type of e is bool_val
// else the type of e is int_const
z3::expr string_to_expr(string s);
z3::expr to_bool_expr(string s);
z3::expr to_expr(int64_t x, unsigned sz = NUM_REG_BITS);
z3::expr to_expr(uint64_t x, unsigned sz = NUM_REG_BITS);
z3::expr to_expr(int32_t x, unsigned sz = NUM_REG_BITS);
z3::expr to_expr(string s, unsigned sz);

struct map_attr { // map attribute
  unsigned int key_sz;
  unsigned int val_sz;
  unsigned int max_entries;
  map_attr(unsigned int k_sz, unsigned int v_sz, unsigned int n_entries = 0) {
    key_sz = k_sz; val_sz = v_sz; max_entries = n_entries;
  }
};
ostream& operator<<(ostream& out, const map_attr& m_attr);

class smt_mem_range {
 public:
  z3::expr start = string_to_expr("true"); // start address, 64-bit bitvector
  z3::expr end = string_to_expr("true"); // end address, 64-bit bitvector
  smt_mem_range() {}
  smt_mem_range(z3::expr s, z3::expr e) {
    start = s.simplify();
    end = e.simplify();
  }
  void set_range(z3::expr s, z3::expr e) {
    start = s.simplify();
    end = e.simplify();
  }
};

class smt_mem_layout {
 public:
  // Assmue that stack and maps next to each other
  // stack | map1 | map2 | ...
  smt_mem_range _stack;
  vector<smt_mem_range> _maps;
  vector<map_attr> _maps_attr;

  smt_mem_layout(uint64_t stack_start = (uint64_t)0xff12000000001234) {
    z3::expr s = to_expr(stack_start);
    z3::expr e = s - 1 + STACK_SIZE;
    _stack.set_range(s, e);
  }
  void add_map(map_attr m_attr);
  void set_map_attr(int map_id, map_attr m_attr) {
    _maps_attr[map_id] = m_attr;
  }
  z3::expr get_stack_bottom_addr() {return (_stack.end + 1);}
};

class smt_wt {
 private:
  bool is_equal(z3::expr e1, z3::expr e2);
 public:
  vector<z3::expr> addr; // 64-bit bitvector
  vector<z3::expr> val;  // 8-bit bitvector
  void add(z3::expr a, z3::expr v) {addr.push_back(a); val.push_back(v);}
  void clear() {addr.clear(); val.clear();}
  unsigned int size() {return addr.size();}
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

#define Z3_true string_to_expr("true")
#define Z3_false string_to_expr("false")

class smt_map_wt {
 public:
  vector<z3::expr> is_valid; // flag, indicate whether this entry is valid or not
  vector<z3::expr> addr_map;
  vector<z3::expr> key;
  vector<z3::expr> addr_v;
  void add(z3::expr iv, z3::expr a, z3::expr k, z3::expr v) {
    is_valid.push_back(iv);
    addr_map.push_back(a);
    key.push_back(k);
    addr_v.push_back(v);
  }
  void clear() {is_valid.clear(); addr_map.clear(); key.clear(); addr_v.clear();}
  unsigned int size() {return addr_map.size();}
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
  void init_addrs_map_v_next(smt_mem_layout& m_layout);
  z3::expr get_and_update_addr_v_next(int map_id, smt_mem_layout& m_layout);
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
  unsigned int mem_addr_id, is_vaild_id, key_cur_id,
           val_cur_id, addr_v_cur_id, map_helper_func_ret_cur_id;
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
  z3::expr update_mem_addr();
  z3::expr update_is_valid();
  z3::expr update_key(unsigned int k_sz = NUM_BYTE_BITS);
  z3::expr update_val(unsigned int v_sz = NUM_BYTE_BITS);
  z3::expr update_addr_v();
  z3::expr update_map_helper_func_ret();
  void clear();
};
