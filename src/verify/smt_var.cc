#include <string>
#include "smt_var.h"

using namespace std;

z3::context smt_c;

ostream& operator<<(ostream& out, const map_attr& m_attr) {
  out << m_attr.key_sz << " " << m_attr.val_sz << " " << m_attr.max_entries;
  return out;
}

/* class smt_wt start */
bool smt_wt::is_equal(z3::expr e1, z3::expr e2) {
  z3::tactic t = z3::tactic(smt_c, "bv");
  z3::solver s = t.mk_solver();
  s.add(!(e1 == e2));
  if (s.check() == z3::unsat) return true;
  return false;
}

smt_wt& smt_wt::operator=(const smt_wt &rhs) {
  addr.clear();
  val.clear();
  for (int i = 0; i < rhs.addr.size(); i++) {
    add(rhs.addr[i], rhs.val[i]);
  }
  return *this;
}

bool smt_wt::operator==(const smt_wt &rhs) {
  bool res = (addr.size() == rhs.addr.size()) &&
             (val.size() == rhs.val.size());
  if (! res) return res;
  for (int i = 0; i < addr.size(); i++) {
    res = res && is_equal(addr[i], rhs.addr[i]) && is_equal(val[i], rhs.val[i]);
  }
  return res;
}

ostream& operator<<(ostream& out, const smt_wt& s) {
  for (int i = 0; i < s.addr.size(); i++) {
    out << i << ": " << s.addr[i].simplify() << " " << s.val[i].simplify() << endl;
  }
  return out;
}

/* class smt_wt end */

/* class smt_map_wt start */

ostream& operator<<(ostream& out, const smt_map_wt& s) {
  for (int i = 0; i < s.key.size(); i++) {
    out << i << ": " << s.is_valid[i] << " " << s.addr_map[i] << " "
        << s.key[i].simplify() << " " << s.addr_v[i].simplify() << endl;
  }
  return out;
}

/* class smt_map_wt start */

/* class smt_wt end */
void smt_mem::init_addrs_map_v_next(smt_mem_layout& m_layout) {
  for (int i = 0; i < m_layout._maps.size(); i++) {
    _addrs_map_v_next.push_back(m_layout._maps[i].start);
  }
}

z3::expr smt_mem::get_and_update_addr_v_next(int map_id, smt_mem_layout& m_layout) {
  z3::expr res = _addrs_map_v_next[map_id];
  _addrs_map_v_next[map_id] = _addrs_map_v_next[map_id] +
                              to_expr((uint64_t)m_layout._maps_attr[map_id].val_sz);
  return res;
}

ostream& operator<<(ostream& out, const smt_mem& s) {
  out << "memory WT:" << endl << s._mem_table._wt
      << "memory URT:" << endl << s._mem_table._urt
      << "map WT:" << endl << s._map_table._wt
      << "map URT:" << endl << s._map_table._urt
      << endl;
  return out;
}
/* class smt_wt end */

/* class smt_var start */
smt_var::smt_var(unsigned int prog_id, unsigned int node_id, unsigned int num_regs) {
  reg_cur_id.resize(num_regs, 0);
  _name = to_string(prog_id) + "_" + to_string(node_id);
  string name_prefix = "r_" + _name + "_";
  for (size_t i = 0; i < num_regs; i++) {
    string name = name_prefix + to_string(i) + "_0";
    reg_var.push_back(string_to_expr(name));
  }
  mem_addr_id = 0;
  is_vaild_id = 0;
  key_cur_id = 0;
  val_cur_id = 0;
  addr_v_cur_id = 0;
  map_helper_func_ret_cur_id = 0;
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

z3::expr smt_var::update_mem_addr() {
  mem_addr_id++;
  string name = "ma_" + _name + "_" + to_string(mem_addr_id);
  return string_to_expr(name);
}

z3::expr smt_var::update_is_valid() {
  is_vaild_id++;
  string name = "iv_" + _name + "_" + to_string(is_vaild_id);
  return to_bool_expr(name);
}

z3::expr smt_var::update_key(unsigned int k_sz) {
  key_cur_id++;
  string name = "k_" + _name + "_" + to_string(key_cur_id);
  return to_expr(name, k_sz);
}

z3::expr smt_var::update_val(unsigned int v_sz) {
  val_cur_id++;
  string name = "v_" + _name + "_" + to_string(val_cur_id);
  return to_expr(name, v_sz);
}

z3::expr smt_var::update_addr_v() {
  addr_v_cur_id++;
  string name = "av_" + _name + "_" + to_string(addr_v_cur_id);
  return to_expr(name, NUM_ADDR_BITS);
}

z3::expr smt_var::update_map_helper_func_ret() {
  map_helper_func_ret_cur_id++;
  string name = "func_ret_" + _name + "_" + to_string(map_helper_func_ret_cur_id);
  return string_to_expr(name);
}

void smt_var::clear() {
  for (size_t i = 0; i < reg_var.size(); i++) {
    reg_cur_id[i] = 0;
    string name = "r_" + _name + "_" + to_string(i) + "_0";
    reg_var[i] = string_to_expr(name);
    mem_addr_id = 0;
    is_vaild_id = 0;
    key_cur_id = 0;
    val_cur_id = 0;
    addr_v_cur_id = 0;
    map_helper_func_ret_cur_id = 0;
  }
  mem_var.clear();
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
