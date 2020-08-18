#include <string>
#include "inst_var.h"

using namespace std;

z3::context smt_c;

z3::expr string_to_expr(string s) {
  if (s == "true") {
    return smt_c.bool_val(true);
  } else if (s == "false") {
    return smt_c.bool_val(false);
  }
  return smt_c.bv_const(s.c_str(), NUM_REG_BITS);
}

z3::expr to_bool_expr(string s) {
  return smt_c.bool_const(s.c_str());
}

z3::expr to_expr(int64_t x, unsigned sz) {
  return smt_c.bv_val(x, sz);
}

z3::expr to_expr(uint64_t x, unsigned sz) {
  return smt_c.bv_val(x, sz);
}

z3::expr to_expr(int32_t x, unsigned sz) {
  return smt_c.bv_val(x, sz);
}

z3::expr to_expr(string s, unsigned sz) {
  return smt_c.bv_const(s.c_str(), sz);
}

// use dfs
bool dag::is_path_a2b(unsigned int a, unsigned int b) {
  if (a == b) return true;
  for (int i = 0; i < out_edges_list[a].size(); i++) {
    if (is_path_a2b(out_edges_list[a][i], b)) return true;
  }
  return false;
}

// check whether there is a way from a to b that does not go through c
// use dfs
bool dag::is_path_a2b_without_c(unsigned int a, unsigned int b, unsigned int c) {
  if (a == c) return false; // does not go through c
  if (a == b) return true;
  for (int i = 0; i < out_edges_list[a].size(); i++) {
    unsigned int node = out_edges_list[a][i];
    if (is_path_a2b_without_c(node, b, c)) return true;
  }
  return false;
}

// 1. check whether there is a way from root to a that goes through b
//    since there is a way from root to b, just need to check whether
//    there is a way from b to a
// 2. check whether there is a way from root to a that does not go through b
int dag::is_b_on_root2a_path(unsigned int a, unsigned int b) {
  assert(a < out_edges_list.size());
  assert(b < out_edges_list.size());
  if (! is_path_a2b(b, a)) return INT_false;
  if (is_path_a2b_without_c(root, a, b)) return INT_uncertain;
  return INT_true;
}

ostream& operator<<(ostream& out, const dag& d) {
  out << "dag: " << endl
      << "root: " << d.root << endl;
  out << "edge: " << endl;
  for (int i = 0; i < d.out_edges_list.size(); i++) {
    out << " " << i << ", out: ";
    for (int j = 0; j < d.out_edges_list[i].size(); j++) {
      out << d.out_edges_list[i][j] << " ";
    }
    out << endl;
  }
  return out;
}

smt_var_base::smt_var_base() {
  path_cond_id = 0;
}

smt_var_base::smt_var_base(unsigned int prog_id, unsigned int node_id, unsigned int num_regs) {
  path_cond_id = 0;
  init(prog_id, node_id, num_regs);
}

smt_var_base::~smt_var_base() {
}

// reset register related variables
// designed for different basic blocks of one program,
void smt_var_base::set_new_node_id(unsigned int node_id, const vector<unsigned int>& nodes_in,
                                   const vector<z3::expr>& node_in_pc_list,
                                   const vector<vector<z3::expr>>& nodes_in_regs) {
  for (int i = 0; i < reg_cur_id.size(); i++) {
    reg_cur_id[i] = 0;
  }
  size_t pos = _name.find('_');
  assert(pos != string::npos); // string::npos: flag of not found
  string prog_id_str = _name.substr(0, pos);
  _name = prog_id_str + "_" + to_string(node_id);
  string name_prefix = "r_" + _name + "_";
  for (size_t i = 0; i < reg_var.size(); i++) {
    string name = name_prefix + to_string(i) + "_0";
    reg_var[i] = string_to_expr(name);
  }
}

z3::expr smt_var_base::update_path_cond() {
  path_cond_id++;
  string name = "pc_" + _name + "_" + to_string(path_cond_id);
  return to_bool_expr(name);
}

z3::expr smt_var_base::update_reg_var(unsigned int reg_id) {
  reg_cur_id[reg_id]++;
  string name = "r_" + _name + "_" + to_string(reg_id) \
                + "_" + to_string(reg_cur_id[reg_id]);
  reg_var[reg_id] = string_to_expr(name);
  return get_cur_reg_var(reg_id);
}

z3::expr smt_var_base::get_cur_reg_var(unsigned int reg_id) {
  return reg_var[reg_id];
}

z3::expr smt_var_base::get_init_reg_var(unsigned int reg_id) {
  string name = "r_" + _name + "_" + to_string(reg_id) + "_0";
  return string_to_expr(name);
}

void smt_var_base::init(unsigned int prog_id, unsigned int node_id, unsigned int num_regs, unsigned int n_blocks) {
  reg_cur_id.resize(num_regs, 0);
  _name = to_string(prog_id) + "_" + to_string(node_id);
  string name_prefix = "r_" + _name + "_";
  for (size_t i = 0; i < num_regs; i++) {
    string name = name_prefix + to_string(i) + "_0";
    reg_var.push_back(string_to_expr(name));
  }
}

void smt_var_base::clear() {
  for (size_t i = 0; i < reg_var.size(); i++) {
    reg_cur_id[i] = 0;
    string name = "r_" + _name + "_" + to_string(i) + "_0";
    reg_var[i] = string_to_expr(name);
  }
}

void prog_state_base::print() const {
  for (int i = 0; i < _regs.size(); i++) {
    cout << "Register "  << i << " " << _regs[i] << endl;
  }
};

void prog_state_base::clear() {
  _pc = 0;
  for (int i = 0; i < _regs.size(); i++) {
    _regs[i] = 0;
  }
};
