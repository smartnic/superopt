#include <string>
#include "smt_var.h"

using namespace std;

z3::context smt_c;
#define NUM_BYTE_BITS 8
z3::sort z3_mem_t = smt_c.array_sort(smt_c.bv_sort(NUM_REG_BITS), smt_c.bv_sort(NUM_BYTE_BITS));

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

// memory: m_[prog_id]_[node_id]_[version_id]
z3::expr smt_var::update_mem_var() {
  mem_cur_id++;
  string name = "m_" + _name + "_" + to_string(mem_cur_id);
  mem_var = to_constant_expr(name);
  return mem_var;
}

z3::expr smt_var::get_cur_mem_var() {
  return mem_var;
}
/* class smt_var end */

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

z3::expr to_constant_expr(string s) {
  return smt_c.constant(s.c_str(), z3_mem_t);
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
