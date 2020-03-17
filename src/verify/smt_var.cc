#include <string>
#include "smt_var.h"

using namespace std;

z3::context smt_c;

/* class smt_stack start */
bool smt_stack::is_equal(z3::expr e1, z3::expr e2) {
  z3::tactic t = z3::tactic(smt_c, "bv");
  z3::solver s = t.mk_solver();
  s.add(!(e1 == e2));
  if (s.check() == z3::unsat) return true;
  return false;
}

smt_stack& smt_stack::operator=(const smt_stack &rhs) {
  addr.clear();
  val.clear();
  for (int i = 0; i < rhs.addr.size(); i++) {
    add(rhs.addr[i], rhs.val[i]);
  }
  return *this;
}

bool smt_stack::operator==(const smt_stack &rhs) {
  bool res = (addr.size() == rhs.addr.size()) &&
              (val.size() == rhs.val.size());
  if (! res) return res;
  for (int i = 0; i < addr.size(); i++) {
    res = res && is_equal(addr[i], rhs.addr[i]) && is_equal(val[i], rhs.val[i]);
  }
  return res;
}

/* class smt_stack end */

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

void smt_var::clear() {
  for (size_t i = 0; i < reg_var.size(); i++) {
    reg_cur_id[i] = 0;
    string name = "r_" + _name + "_" + to_string(i) + "_0";
    reg_var[i] = string_to_expr(name);
  }
  stack_var.clear();
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
