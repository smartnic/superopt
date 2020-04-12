#include "inst_codegen.h"

using namespace std;

z3::expr latest_write_element(int idx, vector<z3::expr>& x);
z3::expr addr_not_in_wt(z3::expr& a, vector<z3::expr>& x);

// designed for little endian
// convert uint8_t array to hex string
// e.g. addr[2] = {0x1, 0xff}, hex string: "ff01"
string ld_n_bytes_from_addr(const uint8_t* a, const size_t n) {
  stringstream ss;
  ss << hex << setfill('0');

  for (int i = n - 1; i >= 0; i--) {
    ss << hex << setw(2) << static_cast<int>(a[i]);
  }

  return ss.str();
}

// return addr_v
uint64_t compute_map_lookup_helper(int addr_map, uint64_t addr_k, mem_t& m) {
  int map_id = addr_map;
  int k_sz = mem_t::_layout._maps_attr[map_id].key_sz / NUM_BYTE_BITS;
  // get key from memory
  string k = ld_n_bytes_from_addr((uint8_t*)addr_k, k_sz);
  map_t& mp = m._maps[map_id];
  auto it = mp._k2idx.find(k);
  if (it == mp._k2idx.end()) return NULL_ADDR;
  int v_idx_in_map = it->second;
  int v_mem_off = m.get_mem_off_by_idx_in_map(map_id, v_idx_in_map);
  return (uint64_t)&m._mem[v_mem_off];
}

uint64_t compute_map_update_helper(int addr_map, uint64_t addr_k, uint64_t addr_v, mem_t& m) {
  int map_id = addr_map;
  int k_sz = mem_t::_layout._maps_attr[map_id].key_sz / NUM_BYTE_BITS;
  int v_sz = mem_t::_layout._maps_attr[map_id].val_sz / NUM_BYTE_BITS;
  // get key and value from memory
  string k = ld_n_bytes_from_addr((uint8_t*)addr_k, k_sz);
  map_t& mp = m._maps[map_id];
  auto it = mp._k2idx.find(k);
  unsigned int v_idx_in_map;
  if (it == mp._k2idx.end()) {
    v_idx_in_map = mp.get_next_idx();
    mp._k2idx[k] = v_idx_in_map;
  } else {
    v_idx_in_map = it->second;
  }
  int v_mem_off = m.get_mem_off_by_idx_in_map(map_id, v_idx_in_map);
  uint64_t addr_v_dst = (uint64_t)&m._mem[v_mem_off];
  memcpy((void*)addr_v_dst, (void*)addr_v, v_sz);
  return MAP_UPD_RET;
}

uint64_t compute_map_delete_helper(int addr_map, uint64_t addr_k, mem_t& m) {
  int map_id = addr_map;
  int k_sz = mem_t::_layout._maps_attr[map_id].key_sz / NUM_BYTE_BITS;
  string k = ld_n_bytes_from_addr((uint8_t*)addr_k, k_sz);
  map_t& mp = m._maps[map_id];
  auto it = mp._k2idx.find(k);
  if (it == mp._k2idx.end()) {
    return MAP_DEL_RET_IF_KEY_INEXIST;
  }
  mp.add_next_idx(it->second);
  mp._k2idx.erase(it);
  return MAP_DEL_RET_IF_KEY_EXIST;
}

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

z3::expr predicate_ld_byte(z3::expr addr, smt_mem& m, z3::expr out, smt_mem_layout& m_layout) {
  smt_wt *s = &m._mem_table._wt;
  z3::expr a = addr;
  z3::expr c = string_to_expr("false");
  z3::expr f = string_to_expr("true");
  for (int i = s->addr.size() - 1; i >= 0; i--) {
    f = f && z3::implies((!c) && (a == s->addr[i]) && (a != NULL_ADDR_EXPR), out == s->val[i]);
    c = c || (a == s->addr[i]);
  }

  // add constrains on the element(a, out)
  f = f && urt_element_constrain(a, out, m._mem_table);
  // add element in urt
  m._mem_table._urt.add(a, out);

  // TODO: find another way to process safety check
  // // safety check
  // // address "a" within the memory range that does not allow ur
  // // if "a" cannot be found in memory WT, the result is false
  // f = f && z3::implies(addr_in_range(a, m_layout._stack.start, m_layout._stack.end) &&
  //                      addr_not_in_wt(a, m._mem_table._wt.addr),
  //                      string_to_expr("false"));
  // // if addr = 0, the result is false
  // f = f && z3::implies(addr == NULL_ADDR_EXPR, string_to_expr("false"));
  return f;
}

z3::expr predicate_ld_n_bytes(int n, z3::expr addr, smt_mem& m, z3::expr out, smt_mem_layout& m_layout) {
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

z3::expr stack_addr_in_one_wt(smt_wt& x, smt_wt& y, smt_mem_range& r) {
  z3::expr f = string_to_expr("true");
  for (int i = x.addr.size() - 1; i >= 0; i--) {
    f = f && z3::implies(addr_in_range(x.addr[i], r.start, r.end) &&
                         addr_not_in_wt(x.addr[i], y.addr),
                         false);
  }
  return f;
}

// need memory WTs, stack memory range
z3::expr smt_stack_eq_chk(smt_wt& x, smt_wt& y, smt_mem_range& r) {
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

z3::expr ld_byte_from_wt(z3::expr addr, smt_wt& wt, z3::expr out) {
  z3::expr a = addr;
  z3::expr f = string_to_expr("true");
  z3::expr f_found_after_i = string_to_expr("false");
  for (int i = wt.addr.size() - 1; i >= 0; i--) {
    f = f && z3::implies((!f_found_after_i) && // latest write
                         (a == wt.addr[i]) && (a != NULL_ADDR_EXPR),
                         out == wt.val[i]);
    f_found_after_i = f_found_after_i || (a == wt.addr[i]);
  }
  return f;
}

z3::expr ld_n_bytes_from_wt(int n, z3::expr addr, smt_wt& wt, z3::expr out) {
  z3::expr f = string_to_expr("true");
  for (int i = 0; i < n; i++) {
    f = f && ld_byte_from_wt(addr + i, wt, out.extract(8 * i + 7, 8 * i));
  }
  return f;
}

z3::expr ld_byte_from_urt(z3::expr addr, smt_wt& urt, z3::expr out) {
  z3::expr a = addr;
  z3::expr f = string_to_expr("true");
  for (int i = 0; i < urt.addr.size(); i++) {
    f = f && z3::implies((a == urt.addr[i]) && (a != NULL_ADDR_EXPR), out == urt.val[i]);
  }
  return f;
}

z3::expr ld_n_bytes_from_urt(int n, z3::expr addr, smt_wt& urt, z3::expr out) {
  z3::expr f = string_to_expr("true");
  for (int i = 0; i < n; i++) {
    f = f && ld_byte_from_urt(addr + i, urt, out.extract(8 * i + 7, 8 * i));
  }
  return f;
}

// keys found in map of sv1, not in map of sv2
// for each key(k) in these keys, the latest write m[k] should be equal to the input
// thus, before writing, this k/v should be read from the map, that is, the element can be found in map URT
z3::expr keys_found_in_one_map(int addr_map, smt_var& sv1, smt_map_wt& map2_wt, smt_mem_layout& m_layout) {
  z3::expr f = string_to_expr("true");
  smt_map_wt& map_wt = sv1.mem_var._map_table._wt;
  smt_map_wt& map_urt = sv1.mem_var._map_table._urt;
  smt_wt& mem_wt = sv1.mem_var._mem_table._wt;
  smt_wt& mem_urt = sv1.mem_var._mem_table._urt;
  int map_id = addr_map;
  int v_sz = m_layout._maps_attr[map_id].val_sz;
  int v_sz_byte = v_sz / NUM_BYTE_BITS;

  // compute and store memory load constrains on addr_v and v for map URT
  // to avoid repetitive computation
  vector<z3::expr> v_in, f_v_in;
  for (int i = 0; i < map_urt.size(); i++) {
    z3::expr v = sv1.update_val(v_sz);
    z3::expr addr_v = map_urt.addr_v[i];
    z3::expr f_v = ld_n_bytes_from_urt(v_sz_byte, addr_v, mem_urt, v);
    v_in.push_back(v);
    f_v_in.push_back(f_v);
  }

  for (int i = map_wt.size() - 1; i >= 0; i--) {
    if (addr_map != map_wt.addr_map[i]) continue; // check the specific map in map1 WT
    z3::expr k_out = map_wt.key[i];
    z3::expr f_k_not_in_map2_wt = key_not_found_after_idx(k_out, -1, addr_map, map2_wt);
    z3::expr f_k_wt_latest_write = key_not_found_after_idx(k_out, i, addr_map, map_wt);
    z3::expr v_out = sv1.update_val(v_sz);
    z3::expr addr_v_out = map_wt.addr_v[i];
    z3::expr f_v_out = ld_n_bytes_from_wt(v_sz_byte, addr_v_out, mem_wt, v_out);
    // k_out should be found in map URT
    // (if output the same as input, there is uninitialized lookup)
    if (map_urt.size() <= 0) {
      f = f && z3::implies(f_k_not_in_map2_wt && f_k_wt_latest_write,
                           string_to_expr("false"));
    }
    for (int j = map_urt.size() - 1; j >= 0; j--) {
      if (addr_map != map_urt.addr_map[j]) continue; // check the specific map in map1 URT
      z3::expr k_in = map_urt.key[j];
      z3::expr f_found_same_k = f_k_not_in_map2_wt && f_k_wt_latest_write &&
                                (k_out == k_in);
      z3::expr addr_v_in = map_urt.addr_v[j];
      z3::expr f_k_both_inexist = (addr_v_out == NULL_ADDR_EXPR) && (addr_v_in == NULL_ADDR_EXPR);
      z3::expr f_k_both_exist = (addr_v_out != NULL_ADDR_EXPR) && (addr_v_in != NULL_ADDR_EXPR);

      f = f && z3::implies(f_found_same_k && f_v_out && f_v_in[j],
                           f_k_both_inexist || (f_k_both_exist && (v_out == v_in[j])));
    }
  }
  return f;
}

z3::expr smt_one_map_eq_chk(int addr_map, smt_var& sv1, smt_var& sv2, smt_mem_layout& m_layout) {
  z3::expr f = string_to_expr("true");
  int map_id = addr_map;
  int v_sz = m_layout._maps_attr[map_id].val_sz;
  // case 1: keys that are in pgm1's map WT and pgm2's map WT
  // if the key is found && load v1 from mem1 WT && load v2 from mem2 WT => 1 or 2
  // 1. addr_v1 == addr_v2 == NULL
  // 2. addr_v1 != NULL && addr_v2 != NULL && v1 == v2
  map_wt& map1 = sv1.mem_var._map_table, map2 = sv2.mem_var._map_table;
  // compute and store memory load constrains on addr_v and v for map2 WT
  // to avoid repetitive computation
  vector<z3::expr> v2, f_v2;
  for (int i = 0; i < map2._wt.size(); i++) {
    z3::expr v = sv2.update_val(v_sz);
    z3::expr addr_v = map2._wt.addr_v[i];
    smt_wt& mem_wt = sv2.mem_var._mem_table._wt;
    z3::expr f_v = ld_n_bytes_from_wt(v_sz / NUM_BYTE_BITS, addr_v, mem_wt, v);
    v2.push_back(v);
    f_v2.push_back(f_v);
  }
  for (int i = map1._wt.size() - 1; i >= 0; i--) {
    if (addr_map != map1._wt.addr_map[i]) continue; // check the specific map in map1 WT
    z3::expr k1 = map1._wt.key[i];
    z3::expr v1 = sv1.update_val(v_sz);
    z3::expr addr_v1 = map1._wt.addr_v[i];
    z3::expr f_k1_map1_latest_write = key_not_found_after_idx(k1, i, addr_map, map1._wt);
    z3::expr f_v1 = ld_n_bytes_from_wt(v_sz / NUM_BYTE_BITS, map1._wt.addr_v[i],
                                       sv1.mem_var._mem_table._wt, v1);

    z3::expr f_k1_found_after_j = string_to_expr("false");
    for (int j = map2._wt.size() - 1; j >= 0; j--) {
      if (addr_map != map2._wt.addr_map[j]) continue; // check the specific map in map1 WT
      z3::expr f_k1_map2_latest_write = (k1 == map2._wt.key[j]) && (!f_k1_found_after_j); // the same key
      z3::expr f_found_same_key = f_k1_map1_latest_write && f_k1_map2_latest_write;
      z3::expr addr_v2 = map2._wt.addr_v[j];
      z3::expr f_k1_both_exist = (addr_v1 != NULL_ADDR_EXPR) && (addr_v2 != NULL_ADDR_EXPR);
      z3::expr f_k1_both_inexist = (addr_v1 == NULL_ADDR_EXPR) && (addr_v2 == NULL_ADDR_EXPR);

      f = f && z3::implies(f_found_same_key && f_v1 && f_v2[j],
                           f_k1_both_inexist || (f_k1_both_exist && (v1 == v2[j])));

      f_k1_found_after_j = f_k1_found_after_j || (k1 == map2._wt.key[j]);
    }
  }

  // case 2: keys that are only in one of pgm1's map WT and pgm2's map WT
  // if the key is found, the ouput should be the same as the input
  // take map1 for an example,
  // if the key not found in map2 && k_out == k_in &&load v_out from mem1 WT &&
  // load v_in from mem1 URT => 1 or 2
  // 1. addr_v_out == addr_v_in == NULL
  // 2. addr_v_out != NULL && addr_v_in != NULL && v_in == v_out
  f = f && keys_found_in_one_map(addr_map, sv1, map2._wt, m_layout) &&
      keys_found_in_one_map(addr_map, sv2, map1._wt, m_layout);
  return f;
}

// add the constrains on the input equivalence setting
// the same k/v in map_p1 URT == map_p2 URT
// each key "k" in map_p1 URT, if "k" is in map_p2 URT => 1 or 2
// 1. "k" is not in the input map, i.e., the corresponding addr_v both NULL
// 2. "k" is in the input map, load v_p1 and v_p2 from mem URT, the corresponding v_p1 == v_p2
z3::expr smt_one_map_set_same_input(int addr_map, smt_var& sv1, smt_var& sv2, smt_mem_layout& m_layout) {
  z3::expr f = string_to_expr("true");
  int map_id = addr_map;
  int v_sz = m_layout._maps_attr[map_id].val_sz;
  smt_map_wt& map1_urt = sv1.mem_var._map_table._urt, map2_urt = sv2.mem_var._map_table._urt;
  // compute and store memory load constrains on addr_v and v for map2 URT
  // to avoid repetitive computation
  vector<z3::expr> v2, f_v2;
  for (int i = 0; i < map2_urt.size(); i++) {
    z3::expr addr_v = map2_urt.addr_v[i];
    z3::expr v = sv2.update_val(v_sz);
    smt_wt& mem_urt = sv2.mem_var._mem_table._urt;
    z3::expr f_v = ld_n_bytes_from_urt(v_sz / NUM_BYTE_BITS, addr_v, mem_urt, v);
    v2.push_back(v);
    f_v2.push_back(f_v);
  }

  for (int i = 0; i < map1_urt.size(); i++) {
    if (addr_map != map1_urt.addr_map[i]) continue;
    z3::expr k1 = map1_urt.key[i];
    z3::expr addr_v1 = map1_urt.addr_v[i];
    z3::expr v1 = sv1.update_val(v_sz);
    smt_wt& mem1_urt = sv1.mem_var._mem_table._urt;
    z3::expr f_v1 = ld_n_bytes_from_urt(v_sz / NUM_BYTE_BITS, addr_v1, mem1_urt, v1);
    for (int j = 0; j < map2_urt.size(); j++) {
      if (addr_map != map2_urt.addr_map[j]) continue;
      z3::expr k2 = map2_urt.key[j];
      z3::expr addr_v2 = map2_urt.addr_v[j];
      z3::expr f_k_both_inexist = (addr_v1 == NULL_ADDR_EXPR) && (addr_v2 == NULL_ADDR_EXPR);
      z3::expr f_k_both_exist = (addr_v1 != NULL_ADDR_EXPR) && (addr_v2 != NULL_ADDR_EXPR);
      // set the constrains on addr_v1, addr_v2, v1, v2
      f = f && z3::implies(k1 == k2,
                           f_k_both_inexist ||
                           (f_k_both_exist && f_v1 && f_v2[j] && (v1 == v2[j])));
    }
  }
  return f;
}

z3::expr smt_map_set_same_input(smt_var& sv1, smt_var& sv2, smt_mem_layout& m_layout) {
  z3::expr f = string_to_expr("true");
  for (int i = 0; i < m_layout._maps.size(); i++) {
    int addr_map = i;
    f = f && smt_one_map_set_same_input(addr_map, sv1, sv2, m_layout);
  }
  return f;
}

z3::expr smt_map_eq_chk(smt_var& sv1, smt_var& sv2, smt_mem_layout& m_layout) {
  z3::expr f = string_to_expr("true");
  for (int i = 0; i < m_layout._maps.size(); i++) {
    int addr_map = i;
    f = f && smt_one_map_eq_chk(addr_map, sv1, sv2, m_layout);
  }
  return f;
}

z3::expr smt_mem_eq_chk(smt_var& sv1, smt_var& sv2, smt_mem_layout& m_layout) {
  return (smt_stack_eq_chk(sv1.mem_var._mem_table._wt,
                           sv2.mem_var._mem_table._wt, m_layout._stack) &&
          smt_map_eq_chk(sv1, sv2, m_layout));
}

z3::expr smt_pgm_mem_eq_chk(vector<z3::expr>& pc1, vector<smt_var>& sv1,
                            vector<z3::expr>& pc2, vector<smt_var>& sv2,
                            smt_mem_layout& m_layout) {
  z3::expr f = string_to_expr("true");
  for (int i = 0; i < pc1.size(); i++) {
    for (int j = 0; j < pc2.size(); j++) {
      f = f && z3::implies(pc1[i] && pc2[j], smt_mem_eq_chk(sv1[i], sv2[j], m_layout));
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

z3::expr predicate_map_lookup_k_in_map_wt(int addr_map, z3::expr k, z3::expr addr_map_v, smt_map_wt& m_wt) {
  z3::expr f = string_to_expr("true");
  z3::expr key_found_after_i = string_to_expr("false");
  for (int i = m_wt.key.size() - 1; i >= 0; i--) {
    if (addr_map != m_wt.addr_map[i]) continue; // the same map
    z3::expr key_found_i = (k == m_wt.key[i]); // the same key
    f = f && z3::implies((!key_found_after_i) && key_found_i, // latest write of key-value pair in the map
                         addr_map_v == m_wt.addr_v[i]);
    key_found_after_i = key_found_after_i || key_found_i;
  }
  return f;
}

// constrain: (k not in map WT) && (map == map_i) && (k == k_i) => addr_v == addr_vi
z3::expr predicate_map_lookup_k_in_map_urt(int addr_map, z3::expr k, z3::expr addr_map_v, map_wt& map_table) {
  z3::expr f = string_to_expr("true");
  z3::expr f1 = key_not_in_map_wt(addr_map, k, map_table._wt);
  for (int i = map_table._urt.key.size() - 1; i >= 0; i--) {
    if (addr_map != map_table._urt.addr_map[i]) continue;
    z3::expr f2 = (k == map_table._urt.key[i]);
    f = f && z3::implies(f1 && f2, addr_map_v == map_table._urt.addr_v[i]);
  }
  return f;
}

// "addr_map_v" is the return value
z3::expr predicate_map_lookup_helper(int addr_map, z3::expr addr_k, z3::expr addr_map_v,
                                     smt_var& sv, smt_mem_layout& m_layout) {
  smt_mem& mem = sv.mem_var;
  int map_id = addr_map;
  int k_sz = m_layout._maps_attr[map_id].key_sz;
  z3::expr k = sv.update_key(k_sz);
  z3::expr f = predicate_ld_n_bytes(k_sz / NUM_BYTE_BITS, addr_k, mem, k, m_layout);

  // case 1: k in the map WT
  f = f && predicate_map_lookup_k_in_map_wt(addr_map, k, addr_map_v, mem._map_table._wt);

  // case 2: k not in the map WT
  // add the constrain of the new element in map URT
  f = f && predicate_map_lookup_k_in_map_urt(addr_map, k, addr_map_v, mem._map_table);

  // constrain: (k not in map WT) && (k not in map URT)
  // => the addr_map_v is next address of the map value in the specific map or NULL
  z3::expr f1 = key_not_in_map_wt(addr_map, k, mem._map_table._wt);
  z3::expr f2 = key_not_in_map_wt(addr_map, k, mem._map_table._urt);
  f = f && z3::implies(f1 && f2,
                       (addr_map_v == NULL_ADDR_EXPR) ||
                       (addr_map_v == mem.get_and_update_addr_v_next(map_id)));

  mem._map_table._urt.add(addr_map, k, addr_map_v);

  return f;
}

// "out" is the return value
z3::expr predicate_map_update_helper(int addr_map, z3::expr addr_k, z3::expr addr_v,
                                     z3::expr out, smt_var& sv, smt_mem_layout& m_layout) {
  smt_mem& mem = sv.mem_var;
  int map_id = addr_map;
  int k_sz = m_layout._maps_attr[map_id].key_sz;
  int v_sz = m_layout._maps_attr[map_id].val_sz;
  z3::expr k = sv.update_key(k_sz);
  z3::expr v = sv.update_val(v_sz);
  z3::expr addr_map_v = sv.update_addr_v();
  z3::expr f = (out == MAP_UPD_RET_EXPR) &&
               predicate_ld_n_bytes(k_sz / NUM_BYTE_BITS, addr_k, mem, k, m_layout) &&
               predicate_ld_n_bytes(v_sz / NUM_BYTE_BITS, addr_v, mem, v, m_layout);
  // constrains on "addr_map_v".
  // constrain 1: for each element in map WT,
  // if the key has been added into the same map, the value address is the same as before
  // i.e., (addr_map == addr_map_i) && (k == k_i) && (addr_map_v_i != NULL) => addr_map_v == addr_map_v_i
  for (int i = mem._map_table._wt.key.size() - 1; i >= 0; i--) {
    smt_map_wt& m_wt = mem._map_table._wt;
    if (addr_map != m_wt.addr_map[i]) continue;
    f = f && z3::implies((k == m_wt.key[i]) && (m_wt.addr_v[i] != NULL_ADDR_EXPR),
                         addr_map_v == m_wt.addr_v[i]);
  }
  // constrain 2: if the key has not been added into the same map,
  // assign a different address to the value address
  z3::expr f_key_in_the_map = string_to_expr("false");
  for (int i = mem._map_table._wt.key.size() - 1; i >= 0; i--) {
    smt_map_wt& m_wt = mem._map_table._wt;
    if (addr_map != m_wt.addr_map[i]) continue;
    f_key_in_the_map = f_key_in_the_map ||
                       ((k == m_wt.key[i]) && (m_wt.addr_v[i] != NULL_ADDR_EXPR));
  }
  f = f && z3::implies((!f_key_in_the_map), addr_map_v == mem.get_and_update_addr_v_next(map_id));

  mem._map_table._wt.add(addr_map, k, addr_map_v);
  predicate_st_n_bytes(v_sz / NUM_BYTE_BITS, v, addr_map_v, mem);
  return f;
}

// "out" is the return value
// if key not in the map, out = 0xfffffffe, else out = 0
// delete cannot use predicate_map_lookup_helper directly, since lookup helper will add an element in map URT,
// while in the map equivalence check, it is assumed that the element in map URT only added by lookup operation
z3::expr predicate_map_delete_helper(int addr_map, z3::expr addr_k, z3::expr out,
                                     smt_var& sv, smt_mem_layout& m_layout) {
  smt_mem& mem = sv.mem_var;
  int map_id = addr_map;
  int k_sz = m_layout._maps_attr[map_id].key_sz;
  z3::expr k = sv.update_key(k_sz);
  z3::expr f = predicate_ld_n_bytes(k_sz / NUM_BYTE_BITS, addr_k, mem, k, m_layout);
  z3::expr addr_map_v = sv.update_addr_v();
  // if k is in the map WT
  f = f && predicate_map_lookup_k_in_map_wt(addr_map, k, addr_map_v, mem._map_table._wt);
  // if k is not in the map WT
  f = f && predicate_map_lookup_k_in_map_urt(addr_map, k, addr_map_v, mem._map_table);

  z3::expr f1 = key_not_in_map_wt(addr_map, k, mem._map_table._wt);
  z3::expr f2 = key_not_in_map_wt(addr_map, k, mem._map_table._urt);
  z3::expr f3 = f1 && f2;
  f = f && z3::implies((!f3) && (addr_map_v == NULL_ADDR_EXPR), out == MAP_DEL_RET_IF_KEY_INEXIST_EXPR);
  f = f && z3::implies((!f3) && (addr_map_v != NULL_ADDR_EXPR), out == MAP_DEL_RET_IF_KEY_EXIST_EXPR);

  f = f && z3::implies(f3, (out == MAP_DEL_RET_IF_KEY_INEXIST_EXPR) || (out == MAP_DEL_RET_IF_KEY_EXIST_EXPR));
  // if key in the map, insert an element in map table to delete this key
  mem._map_table._wt.add(addr_map, k, NULL_ADDR_EXPR);
  return f;
}
