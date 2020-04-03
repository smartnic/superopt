#include "inst_codegen.h"

using namespace std;

z3::expr latest_write_element(int idx, vector<z3::expr>& x);
z3::expr addr_not_in_wt(z3::expr& a, vector<z3::expr>& x);

void predicate_st_byte(z3::expr in, z3::expr addr, z3::expr off, smt_mem& m) {
  m._mem_table._wt.add(addr + off, in.extract(7, 0));
}

inline z3::expr addr_in_range(z3::expr addr, z3::expr start, z3::expr end) {
  return (uge(addr, start) && uge(end, addr));
}

z3::expr urt_element_constrain(z3::expr a, z3::expr v, mem_wt& x) {
  z3::expr f = string_to_expr("true");
  // add constrains on the new symbolic value "v" according to the following cases:
  // case 1: "a" can be found in x._wt(addr1).
  // if addr1 is the latest write address in x._wt and a is equal to addr1,
  // it implies v is equal to the value of addr1
  // The constrains of this case can be removed, since if the addr exists in the memory WT,
  // the value in that address is read from WT but not URT.

  // case 2: "a" cannot be found in x._wt, but x._urt(addr1).
  // if there is no address equal to a in x._wt and addr1 in x._urt is equal to
  // a, it implies v is equal to the value of addr1
  z3::expr f2 = string_to_expr("true");
  z3::expr f3 = addr_not_in_wt(a, x._wt.addr);
  // "-2" is to skip the latest add just now
  for (int i = x._urt.addr.size() - 2; i >= 0; i--) {
    f2 = f2 && z3::implies(f3 && (a == x._urt.addr[i]), v == x._urt.val[i]);
  }
  f = f && f2;
  // case 3: "a" cannot be found in x._wt or x._urt.
  // there is no constrains on the new symbolic value "v"
  return f;
}

z3::expr predicate_ld_byte(z3::expr addr, z3::expr off, smt_mem& m, z3::expr out, mem_layout& m_layout) {
  smt_wt *s = &m._mem_table._wt;
  z3::expr a = addr + off;
  z3::expr c = string_to_expr("false");
  z3::expr f = string_to_expr("true");
  for (int i = s->addr.size() - 1; i >= 0; i--) {
    f = f && z3::implies((!c) && (a == s->addr[i]), out == s->val[i]);
    c = c || (a == s->addr[i]);
  }
  // add element in urt
  m._mem_table._urt.add(a, out);
  // add constrains on the element(a, out)
  f = f && urt_element_constrain(a, out, m._mem_table);

  // safety check
  // address "a" within the memory range that does not allow ur
  // if "a" cannot be found in memory WT, the result is false
  f = f && z3::implies(addr_in_range(a, m_layout._stack.start, m_layout._stack.end) &&
                       addr_not_in_wt(a, m._mem_table._wt.addr),
                       string_to_expr("false"));
  return f;
}

// return the FOL formula that x.addr[idx] is the latest addr in x
// that is, for any i > idx, x.addr[idx] != x.addr[i]
z3::expr latest_write_element(int idx, vector<z3::expr>& x) {
  z3::expr f = string_to_expr("true");
  for (int i = x.size() - 1; i > idx; i--) {
    f = f && (x[idx] != x[i]);
  }
  return f;
}

// return the FOL formula that addr cannot be found in x
// that is, for any addr_x in x, addr != addr_x
z3::expr addr_not_in_wt(z3::expr& a, vector<z3::expr>& x) {
  z3::expr f = string_to_expr("true");
  for (int i = 0; i < x.size(); i++) {
    f = f && (a != x[i]);
  }
  return f;
}

z3::expr stack_addr_in_one_wt(smt_wt& x, smt_wt& y, mem_range& r) {
  z3::expr f = string_to_expr("true");
  for (int i = x.addr.size() - 1; i >= 0; i--) {
    f = f && z3::implies(addr_in_range(x.addr[i], r.start, r.end) &&
                         addr_not_in_wt(x.addr[i], y.addr),
                         false);
  }
  return f;
}

// need memory WTs, stack memory range
z3::expr smt_stack_eq_chk(smt_wt& x, smt_wt& y, mem_range& r) {
  z3::expr f = string_to_expr("true");
  // case 1: addrs can be found in both WTs
  z3::expr f_addr_x = string_to_expr("true");
  z3::expr f_addr_y = string_to_expr("true");
  for (int i = x.addr.size() - 1; i >= 0; i--) {
    f_addr_x = addr_in_range(x.addr[i], r.start, r.end) &&
               latest_write_element(i, x.addr);
    for (int j = y.addr.size() - 1; j >= 0; j--) {
      f_addr_y = addr_in_range(y.addr[j], r.start, r.end) &&
                 latest_write_element(j, y.addr);
      f = f && z3::implies(f_addr_x && f_addr_y && (x.addr[i] == y.addr[j]),
                           x.val[i] == y.val[j]);
    }
  }
  // case 2: addrs can only be found in one WT.
  // If these addrs are found, the result is false.
  f = f && stack_addr_in_one_wt(x, y, r) && stack_addr_in_one_wt(y, x, r);
  return f;
}

// TODO: in order to test, stack is checked
// need to modify later, since stack does not need to be checked
z3::expr smt_mem_eq_chk(smt_mem& x, smt_mem& y, mem_layout& m_layout) {
  return smt_stack_eq_chk(x._mem_table._wt, y._mem_table._wt, m_layout._stack);
}

z3::expr pgm_smt_mem_eq_chk(vector<z3::expr>& pc1, vector<smt_mem>& mem1,
                            vector<z3::expr>& pc2, vector<smt_mem>& mem2,
                            mem_layout& m_layout) {
  z3::expr f = string_to_expr("true");
  for (int i = 0; i < pc1.size(); i++) {
    for (int j = 0; j < pc2.size(); j++) {
      f = f && z3::implies((pc1[i] && pc2[j]), smt_mem_eq_chk(mem1[i], mem2[j], m_layout));
    }
  }
  return f;
}

z3::expr key_not_in_map_wt(z3::expr addr_map, z3::expr k, smt_map_wt& m_wt) {
  z3::expr f = string_to_expr("true"); // f1: k not in the map WT
  for (int i = 0; i < m_wt.key.size(); i++) {
    f == f && (!((addr_map == m_wt.addr_map[i]) &&
                 (k == m_wt.key[i])));
  }
  return f;
}

z3::expr predicate_map_lookup_helper(z3::expr addr_map, z3::expr addr_k, z3::expr addr_map_v,
                                     smt_var& sv, mem_layout& m_layout) {
  smt_mem& mem = sv.mem_var;
  z3::expr k = sv.update_key();
  z3::expr v = sv.update_val();
  z3::expr f = predicate_ld_byte(addr_k, to_expr(0), mem, k, m_layout);

  // case 1: k in the map WT
  for (int i = mem._map_table._wt.key.size() - 1; i >= 0; i--) {
    f = f && z3::implies( (addr_map == mem._map_table._wt.addr_map[i]) && // the same map
                          latest_write_element(i, mem._map_table._wt.key) && // latest write of key-value pair in the map
                          (k == mem._map_table._wt.key[i]), // the same key
                          addr_map_v == mem._map_table._wt.addr_v[i]);
  }

  // case 2: k not in the map WT
  // add the constrain of the new element in map URT
  // constrain 1: the addr_map_v is next address of the map value in the specific map or NULL
  // constrain 1 is also for case 1, so do not need to add case 1 condition
  for (int i = 0; i < m_layout._maps.size(); i++) {
    f = f && z3::implies(addr_map == m_layout._maps[i].start,
                         (addr_map_v == NULL_ADDR) ||
                         (addr_map_v == mem.get_and_update_addr_v_next(i)));
  }
  // constrain: (k not in map WT) && (map == map_i) && (k == k_i) => addr_v == addr_vi
  z3::expr f1 = key_not_in_map_wt(addr_map, k, mem._map_table._wt);
  for (int i = mem._map_table._urt.key.size() - 1; i >= 0; i--) {
    z3::expr f2 = (addr_map == mem._map_table._urt.addr_map[i]) &&
                  (k == mem._map_table._urt.key[i]);
    f = f && z3::implies(f1 && f2, addr_map_v == mem._map_table._urt.addr_v[i]);
  }
  f = f && f1;
  mem._map_table._urt.add(addr_map, k, addr_map_v);

  return f;
}

z3::expr predicate_map_update_helper(z3::expr addr_map, z3::expr addr_k, z3::expr addr_v,
                                     smt_var& sv, mem_layout& m_layout) {
  smt_mem& mem = sv.mem_var;
  z3::expr k = sv.update_key();
  z3::expr v = sv.update_val();
  z3::expr addr_map_v = sv.update_addr_v();
  z3::expr f = predicate_ld_byte(addr_k, to_expr(0), mem, k, m_layout) &&
               predicate_ld_byte(addr_v, to_expr(0), mem, v, m_layout);
  // constrains on "addr_map_v".
  // constrain 1: for each element in map WT,
  // if the key has been added into the same map, the value address is the same as before
  // i.e., (addr_map == addr_map_i) && (k == k_i) && (addr_map_v_i != NULL) => addr_map_v == addr_map_v_i
  for (int i = mem._map_table._wt.key.size() - 1; i >= 0; i--) {
    smt_map_wt& m_wt = mem._map_table._wt;
    f = f && z3::implies((addr_map == m_wt.addr_map[i]) && (k == m_wt.key[i]) &&
                         (m_wt.addr_v[i] == NULL_ADDR),
                         addr_map_v == m_wt.addr_v[i]);
  }
  // constrain 2: if the key has not been added into the same map,
  // assign a different address to the value address
  z3::expr f1 = key_not_in_map_wt(addr_map, k, mem._map_table._wt);
  for (int i = 0; i < m_layout._maps.size(); i++) {
    f = f && z3::implies(f1 && (addr_map == m_layout._maps[i].start),
                         addr_map_v == mem.get_and_update_addr_v_next(i));
  }
  mem._map_table._wt.add(addr_map, k, addr_map_v);
  predicate_st_byte(v, addr_map_v, to_expr(0), mem);
  return f;
}

z3::expr predicate_map_delete_helper(z3::expr addr_map, z3::expr addr_k,
                                     smt_var& sv, mem_layout& m_layout) {
  smt_mem& mem = sv.mem_var;
  z3::expr k = sv.update_key();
  // FOL formula of the constrain on "k"
  z3::expr f = predicate_ld_byte(addr_k, to_expr(0), sv.mem_var, k, m_layout);

  mem._map_table._wt.add(addr_map, k, NULL_ADDR);
  return f;
}
