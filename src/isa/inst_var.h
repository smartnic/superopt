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
#define INT_true  1
#define INT_false 0
#define INT_uncertain -1

// convert string s into expr e
// if e = "true"/"false" the type of e is bool_val
// else the type of e is int_const
z3::expr string_to_expr(string s);
z3::expr to_bool_expr(string s);
z3::expr to_expr(int64_t x, unsigned sz = NUM_REG_BITS);
z3::expr to_expr(uint64_t x, unsigned sz = NUM_REG_BITS);
z3::expr to_expr(int32_t x, unsigned sz = NUM_REG_BITS);
z3::expr to_expr(string s, unsigned sz);

class dag { // directed acyclic graph
 private:
  bool is_path_a2b(unsigned int a, unsigned int b);
  bool is_path_a2b_without_c(unsigned int a, unsigned int b, unsigned int c);
 public:
  unsigned int root;
  vector<vector<unsigned int>> out_edges_list; // outgoing edges, list index: node id
  dag(unsigned int n_nodes = 1, unsigned int root_node = 0) {init(n_nodes, root_node);}
  void init(unsigned int n_nodes, unsigned int root_node) {
    out_edges_list.resize(n_nodes);
    root = root_node;
  }
  void add_edge_a2b(unsigned int a, unsigned int b) {
    out_edges_list[a].push_back(b);
  }
  int is_b_on_root2a_path(unsigned int a, unsigned int b);
  friend ostream& operator<<(ostream& out, const dag& d);
};

// SMT Variable format
// register: r_[prog_id]_[node_id]_[reg_id]_[version_id]
class smt_var_base {
 protected:
  unsigned int path_cond_id;
  // _name: [prog_id]_[node_id]
  string _name;
  // store the curId
  vector<unsigned int> reg_cur_id;
  vector<z3::expr> reg_var;
 public:
  dag pgm_dag;
  smt_var_base();
  // 1. Convert prog_id and node_id into _name, that is string([prog_id]_[node_id])
  // 2. Initialize reg_val[i] = r_[_name]_0, i = 0, ..., num_regs
  smt_var_base(unsigned int prog_id, unsigned int node_id, unsigned int num_regs);
  ~smt_var_base();
  void set_new_node_id(unsigned int node_id, const vector<unsigned int>& nodes_in,
                       const vector<z3::expr>& node_in_pc_list,
                       const vector<vector<z3::expr>>& nodes_in_regs);
  z3::expr update_path_cond();
  // inital value for [versionId] is 0, and increases when updated
  z3::expr update_reg_var(unsigned int reg_id);
  z3::expr get_cur_reg_var(unsigned int reg_id);
  z3::expr get_init_reg_var(unsigned int reg_id);
  void init() {}
  void init(unsigned int prog_id, unsigned int node_id, unsigned int num_regs, unsigned int n_blocks = 1);
  void clear();
};

class prog_state_base {
  int _pc = 0; /* Assume only straight line code execution for now */
 public:
  vector<reg_t> _regs; /* assume only registers for now */
  void init() {}
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

// exposed APIs
// void get_cmp_lists(vector<reg_t>& val_list1, vector<reg_t>& val_list2,
//                    inout_t& output1, inout_t& output2);
/* Generate the random inputs and store them in the input paramenter `inputs`.
   Parameters `reg_min` and `reg_max` are the minimum and maximum values of the input register.
   This limitation needs to be generalized later.
*/
// void gen_random_input(vector<inout_t>& inputs, reg_t reg_min, reg_t reg_max);
