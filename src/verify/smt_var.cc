#include <string>
#include "smt_var.h"

using namespace std;

z3::context smt_c;

/* class smt_var start */
smt_var::smt_var(unsigned int prog_id, unsigned int node_id, unsigned int num_regs) {
  reg_cur_id.resize(num_regs, 0);
  _name = to_string(prog_id) + "_" + to_string(node_id);
  string name_prefix = "r_" + _name + "_";
  for (size_t i = 0; i < num_regs; i++) {
    string name = name_prefix + to_string(i) + "_0";
    reg_var.push_back(string_to_expr(name));
  }
}

smt_var::~smt_var() {
}

z3::expr smt_var::update_reg_var(unsigned int reg_id) {
  reg_cur_id[reg_id]++;
  string name = "r_" + _name + "_" + to_string(reg_id) \
                     + "_" + to_string(reg_cur_id[reg_id]);
  reg_var[reg_id] = string_to_expr(name);
  return get_cur_reg_var(reg_id);
}

z3::expr smt_var::get_cur_reg_var(unsigned int reg_id) {
  return reg_var[reg_id];
}

z3::expr smt_var::get_init_reg_var(unsigned int reg_id) {
  string name = "r_" + _name + "_" + to_string(reg_id) + "_0";
  return string_to_expr(name);
}
/* class smt_var end */

z3::expr string_to_expr(string s) {
  if (s == "true") {
    return smt_c.bool_val(true);
  } else if (s == "false") {
    return smt_c.bool_val(false);
  }
  return smt_c.int_const(s.c_str());
}

z3::expr to_expr(int64_t x) {
  return smt_c.int_val(x);
}

z3::expr to_expr(int32_t x) {
  return smt_c.int_val(x);
}

z3::expr to_expr(string s, unsigned n) {
  return smt_c.bv_const(s.c_str(), n);
}

z3::expr to_expr(int x, unsigned n) {
  return smt_c.bv_val(x, n);
}

z3::expr to_expr(int64_t x, unsigned n) {
  return smt_c.bv_val(x, n);
}
