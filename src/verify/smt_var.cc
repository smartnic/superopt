#include <iostream>
#include <string>
#include "smt_var.h"

using namespace z3;

context smt_c;

/* class smt_var start */
smt_var::smt_var(unsigned int prog_id, unsigned int node_id, unsigned int num_regs) {
  reg_cur_id.resize(num_regs, 0);
  _name = std::to_string(prog_id) + "_" + std::to_string(node_id);
  std::string name_prefix = "r_" + _name + "_";
  for (size_t i = 0; i < num_regs; i++) {
    std::string name = name_prefix + std::to_string(i) + "_0";
    reg_var.push_back(string_to_expr(name));
  }
}

smt_var::~smt_var() {
}

expr smt_var::update_reg_var(unsigned int reg_id) {
  reg_cur_id[reg_id]++;
  std::string name = "r_" + _name + "_" + std::to_string(reg_id) \
                     + "_" + std::to_string(reg_cur_id[reg_id]);
  reg_var[reg_id] = string_to_expr(name);
  return get_cur_reg_var(reg_id);
}

expr smt_var::get_cur_reg_var(unsigned int reg_id) {
  return reg_var[reg_id];
}

expr smt_var::get_init_reg_var(unsigned int reg_id) {
  std::string name = "r_" + _name + "_" + std::to_string(reg_id) + "_0";
  return string_to_expr(name);
}
/* class smt_var end */

expr string_to_expr(string s) {
  if (s == "true") {
    return smt_c.bool_val(true);
  } else if (s == "false") {
    return smt_c.bool_val(false);
  }
  return smt_c.int_const(s.c_str());
}
