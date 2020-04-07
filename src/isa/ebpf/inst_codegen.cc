#include "inst_codegen.h"

using namespace std;

z3::expr latest_write_element(int idx, vector<z3::expr>& x);
z3::expr addr_not_in_wt(z3::expr& a, vector<z3::expr>& x);

void predicate_st_byte(z3::expr in, z3::expr addr, smt_mem& m) {
  m._mem_table._wt.add(addr, in.extract(7, 0));
}

void predicate_st_n_bytes(int n, z3::expr in, z3::expr addr, smt_mem& m) {
  for (int i = 0; i < n; i++) {
    predicate_st_byte(in.extract(8 * i + 7, 8 * i), addr + i, m);
  }
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
  z3::expr f1 = addr_not_in_wt(a, x._wt.addr);
  for (int i = x._urt.addr.size() - 1; i >= 0; i--) {
    f = f && z3::implies(f1 && (a == x._urt.addr[i]), v == x._urt.val[i]);
  }
  // case 3: "a" cannot be found in x._wt or x._urt.
  // there is no constrains on the new symbolic value "v"
  return f;
}

z3::expr predicate_ld_byte(z3::expr addr, smt_mem& m, z3::expr out, mem_layout& m_layout) {
  smt_wt *s = &m._mem_table._wt;
  z3::expr a = addr;
  z3::expr c = string_to_expr("false");
  z3::expr f = string_to_expr("true");
  for (int i = s->addr.size() - 1; i >= 0; i--) {
    f = f && z3::implies((!c) && (a == s->addr[i]), out == s->val[i]);
    c = c || (a == s->addr[i]);
  }

  // add constrains on the element(a, out)
  f = f && urt_element_constrain(a, out, m._mem_table);
  // add element in urt
  m._mem_table._urt.add(a, out);

  // safety check
  // address "a" within the memory range that does not allow ur
  // if "a" cannot be found in memory WT, the result is false
  f = f && z3::implies(addr_in_range(a, m_layout._stack.start, m_layout._stack.end) &&
                       addr_not_in_wt(a, m._mem_table._wt.addr),
                       string_to_expr("false"));
  // if addr = 0, the result is false
  f = f && z3::implies(addr == NULL_ADDR, string_to_expr("false"));
  return f;
}

z3::expr predicate_ld_n_bytes(int n, z3::expr addr, smt_mem& m, z3::expr out, mem_layout& m_layout) {
  z3::expr f = predicate_ld_byte(addr, m, out.extract(7, 0), m_layout);
  for (int i = 1; i < n; i++) {
    f = f && predicate_ld_byte(addr + i, m, out.extract(8 * i + 7, 8 * i), m_layout);
  }
  return f;
}

// return the FOL formula that x[idx] is the latest write in x
// that is, for any i > idx, x[idx] != x[i]
z3::expr latest_write_element(int idx, vector<z3::expr>& x) {
  z3::expr f = string_to_expr("true");
  for (int i = x.size() - 1; i > idx; i--) {
    f = f && (x[idx] != x[i]);
  }
  return f;
}

// return the FOL formula that a cannot be found in x
// that is, for each x[i], a != x[i]
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

z3::expr key_not_found_after_idx(z3::expr key, int idx, int addr_map, smt_map_wt& m_wt) {
  z3::expr f = string_to_expr("true");
  for (int i = m_wt.size() - 1; i > idx; i--) {
    if (addr_map != m_wt.addr_map[i]) continue;
    f = f && (key != m_wt.key[i]);
  }
  return f;
}

z3::expr smt_one_map_eq_chk(int addr_map, smt_var& sv1, smt_var& sv2, mem_layout& m_layout) {
  z3::expr f = string_to_expr("true");
  int map_id = addr_map;
  int v_sz = m_layout._maps_attr[addr_map].val_sz;
  // case 1: keys that are in pgm1's map WT and pgm2's map WT
  // if the key is found, m_pgm1[k] == m_pgm2[k]
  map_wt& map1 = sv1.mem_var._map_table, map2 = sv2.mem_var._map_table;
  for (int i = map1._wt.size() - 1; i >= 0; i--) {
    if (addr_map != map1._wt.addr_map[i]) continue; // check the specific map in map1 WT
    z3::expr k1 = map1._wt.key[i];
    z3::expr v1 = sv1.update_val(v_sz);
    z3::expr addr_v1 = map1._wt.addr_v[i];
    z3::expr f_k1_map1_latest_write = key_not_found_after_idx(k1, i, addr_map, map1._wt);
    z3::expr f_v1 = (addr_v1 != NULL_ADDR) &&
                    predicate_ld_n_bytes(v_sz / NUM_BYTE_BITS, map1._wt.addr_v[i],
                                         sv1.mem_var, v1, m_layout);

    z3::expr f_k1_found_after_j = string_to_expr("false");
    for (int j = map2._wt.size() - 1; j >= 0; j--) {
      if (addr_map != map2._wt.addr_map[j]) continue; // check the specific map in map1 WT
      z3::expr f_k1_map2_latest_write = (k1 == map2._wt.key[j]) && (!f_k1_found_after_j); // the same key
      z3::expr f_found_same_key = f_k1_map1_latest_write && f_k1_map2_latest_write;
      z3::expr v2 = sv2.update_val(v_sz);
      z3::expr addr_v2 = map2._wt.addr_v[j];
      z3::expr f_v2 = (addr_v2 != NULL_ADDR) &&
                      predicate_ld_n_bytes(v_sz / NUM_BYTE_BITS, map2._wt.addr_v[j],
                                           sv2.mem_var, v2, m_layout);
      f = f && z3::implies(f_found_same_key && f_v1 && f_v2, v1 == v2);
      z3::expr f_k1_del_in_one_map = ((addr_v1 == NULL_ADDR) && (addr_v2 != NULL_ADDR)) ||
                                     ((addr_v1 != NULL_ADDR) && (addr_v2 == NULL_ADDR));
      f = f && z3::implies(f_found_same_key && f_k1_del_in_one_map,
                           string_to_expr("false"));
      f_k1_found_after_j = f_k1_found_after_j || (k1 == map2._wt.key[j]);
    }
  }

  return f;
  // case 2: keys that are only in one of pgm1's map WT and pgm2's map WT
  // if the key is found, this pgm must read the k/v first and then write back (latest write) the same k/v.
  // it indicate this key should be found in map URT, and the latest m[k] in WT == m[k] in the map URT
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

z3::expr key_not_in_map_wt(int addr_map, z3::expr k, smt_map_wt& m_wt) {
  z3::expr f = string_to_expr("false");
  for (int i = 0; i < m_wt.key.size(); i++) {
    if (addr_map != m_wt.addr_map[i]) continue;
    f = f || (k == m_wt.key[i]);
  }
  return (!f);
}

// "addr_map_v" is the return value
z3::expr predicate_map_lookup_helper(int addr_map, z3::expr addr_k, z3::expr addr_map_v,
                                     smt_var& sv, mem_layout& m_layout) {
  smt_mem& mem = sv.mem_var;
  int map_id = addr_map;
  int k_sz = m_layout._maps_attr[map_id].key_sz;
  z3::expr k = sv.update_key(k_sz);
  z3::expr f = predicate_ld_n_bytes(k_sz / NUM_BYTE_BITS, addr_k, mem, k, m_layout);

  // case 1: k in the map WT
  z3::expr key_found_after_i = string_to_expr("false");
  for (int i = mem._map_table._wt.key.size() - 1; i >= 0; i--) {
    if (addr_map != mem._map_table._wt.addr_map[i]) continue; // the same map
    z3::expr key_found_i = (k == mem._map_table._wt.key[i]); // the same key
    f = f && z3::implies((!key_found_after_i) && key_found_i, // latest write of key-value pair in the map
                         addr_map_v == mem._map_table._wt.addr_v[i]);
    key_found_after_i = key_found_after_i || key_found_i;
  }

  // case 2: k not in the map WT
  // add the constrain of the new element in map URT
  // constrain: (k not in map WT) && (map == map_i) && (k == k_i) => addr_v == addr_vi or
  z3::expr f1 = key_not_in_map_wt(addr_map, k, mem._map_table._wt);
  for (int i = mem._map_table._urt.key.size() - 1; i >= 0; i--) {
    if (addr_map != mem._map_table._urt.addr_map[i]) continue;
    z3::expr f2 = (k == mem._map_table._urt.key[i]);
    f = f && z3::implies(f1 && f2, addr_map_v == mem._map_table._urt.addr_v[i]);
  }

  // constrain: (k not in map WT) && (k not in map URT)
  // => the addr_map_v is next address of the map value in the specific map or NULL
  z3::expr f2 = key_not_in_map_wt(addr_map, k, mem._map_table._urt);
  f = f && z3::implies(f1 && f2,
                       (addr_map_v == NULL_ADDR) ||
                       (addr_map_v == mem.get_and_update_addr_v_next(map_id)));

  mem._map_table._urt.add(addr_map, k, addr_map_v);

  return f;
}

// "out" is the return value
z3::expr predicate_map_update_helper(int addr_map, z3::expr addr_k, z3::expr addr_v,
                                     z3::expr out, smt_var& sv, mem_layout& m_layout) {
  smt_mem& mem = sv.mem_var;
  int map_id = addr_map;
  int k_sz = m_layout._maps_attr[map_id].key_sz;
  int v_sz = m_layout._maps_attr[map_id].val_sz;
  z3::expr k = sv.update_key(k_sz);
  z3::expr v = sv.update_val(v_sz);
  z3::expr addr_map_v = sv.update_addr_v();
  z3::expr f = (out == MAP_UPD_RET) &&
               predicate_ld_n_bytes(k_sz / NUM_BYTE_BITS, addr_k, mem, k, m_layout) &&
               predicate_ld_n_bytes(v_sz / NUM_BYTE_BITS, addr_v, mem, v, m_layout);
  // constrains on "addr_map_v".
  // constrain 1: for each element in map WT,
  // if the key has been added into the same map, the value address is the same as before
  // i.e., (addr_map == addr_map_i) && (k == k_i) && (addr_map_v_i != NULL) => addr_map_v == addr_map_v_i
  for (int i = mem._map_table._wt.key.size() - 1; i >= 0; i--) {
    smt_map_wt& m_wt = mem._map_table._wt;
    if (addr_map != m_wt.addr_map[i]) continue;
    f = f && z3::implies((k == m_wt.key[i]) && (m_wt.addr_v[i] != NULL_ADDR),
                         addr_map_v == m_wt.addr_v[i]);
  }
  // constrain 2: if the key has not been added into the same map,
  // assign a different address to the value address
  z3::expr f_key_in_the_map = string_to_expr("false");
  for (int i = mem._map_table._wt.key.size() - 1; i >= 0; i--) {
    smt_map_wt& m_wt = mem._map_table._wt;
    if (addr_map != m_wt.addr_map[i]) continue;
    f_key_in_the_map = f_key_in_the_map ||
                       ((k == m_wt.key[i]) && (m_wt.addr_v[i] != NULL_ADDR));
  }
  f = f && z3::implies((!f_key_in_the_map), addr_map_v == mem.get_and_update_addr_v_next(map_id));

  mem._map_table._wt.add(addr_map, k, addr_map_v);
  predicate_st_n_bytes(v_sz / NUM_BYTE_BITS, v, addr_map_v, mem);
  return f;
}

// "out" is the return value
// if key not in the map, out = 0xfffffffe, else out = 0
z3::expr predicate_map_delete_helper(int addr_map, z3::expr addr_k, z3::expr out,
                                     smt_var& sv, mem_layout& m_layout) {
  z3::expr addr_map_v = sv.update_addr_v();
  z3::expr f = predicate_map_lookup_helper(addr_map, addr_k, addr_map_v, sv, m_layout);
  f = f && z3::implies(addr_map_v == NULL_ADDR, out == MAP_DEL_RET_IF_KEY_INEXIST);
  f = f && z3::implies(addr_map_v != NULL_ADDR, out == MAP_DEL_RET_IF_KEY_EXIST);

  // if key in the map, insert an element in map table to delete this key
  smt_mem& mem = sv.mem_var;
  int map_id = addr_map;
  int k_sz = m_layout._maps_attr[map_id].key_sz;
  z3::expr k = sv.update_key(k_sz);
  // FOL formula of the constrain on "k"
  f = f && predicate_ld_n_bytes(k_sz / NUM_BYTE_BITS, addr_k, sv.mem_var, k, m_layout);

  mem._map_table._wt.add(addr_map, k, NULL_ADDR);
  return f;
}
