#include "inst_codegen.h"

using namespace std;

default_random_engine gen_codegen;
uniform_real_distribution<double> unidist_codegen(0.0, 1.0);

z3::expr latest_write_element(int idx, vector<z3::expr>& x);
z3::expr addr_in_addrs(z3::expr& a, vector<z3::expr>& x);
z3::expr key_not_found_after_idx(z3::expr key, int idx, z3::expr addr_map, smt_map_wt& m_wt);
z3::expr key_not_in_map_wt(z3::expr k, smt_map_wt& m_wt);

uint64_t compute_helper_function(int func_id, uint64_t r1, uint64_t r2, uint64_t r3,
                                 uint64_t r4, uint64_t r5, mem_t& m, simu_real& sr) {
  switch (func_id) {
    case BPF_FUNC_map_lookup: return compute_map_lookup_helper(r1, r2, m, sr);
    case BPF_FUNC_map_update: return compute_map_update_helper(r1, r2, r3, m, sr);
    case BPF_FUNC_map_delete: return compute_map_delete_helper(r1, r2, m, sr);
    default: cout << "Error: unknown function id " << func_id << endl; return -1;
  }
}

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
uint64_t compute_map_lookup_helper(int addr_map, uint64_t addr_k, mem_t& m,
                                   simu_real& sr) {
  int map_id = addr_map;
  int k_sz = mem_t::map_key_sz(map_id) / NUM_BYTE_BITS;
  uint64_t real_addr_k = get_real_addr_by_simu(addr_k, m, sr);
  // safety check to avoid segmentation fault
  m.memory_access_check(real_addr_k, k_sz);
  // get key from memory
  string k = ld_n_bytes_from_addr((uint8_t*)real_addr_k, k_sz);
  map_t& mp = m._maps[map_id];
  auto it = mp._k2idx.find(k);
  if (it == mp._k2idx.end()) return NULL_ADDR;
  int v_idx_in_map = it->second;
  int v_mem_off = m.get_mem_off_by_idx_in_map(map_id, v_idx_in_map);
  uint64_t real_addr_v = (uint64_t)&m._mem[v_mem_off];
  return get_simu_addr_by_real(real_addr_v, m, sr);
}

uint64_t compute_map_update_helper(int addr_map, uint64_t addr_k, uint64_t addr_v, mem_t& m,
                                   simu_real& sr) {
  int map_id = addr_map;
  int k_sz = mem_t::map_key_sz(map_id) / NUM_BYTE_BITS;
  uint64_t real_addr_k = get_real_addr_by_simu(addr_k, m, sr);
  m.memory_access_check(real_addr_k, k_sz);
  // get key and value from memory
  string k = ld_n_bytes_from_addr((uint8_t*)real_addr_k, k_sz);
  uint64_t real_addr_v = get_real_addr_by_simu(addr_v, m, sr);
  m.update_kv_in_map(map_id, k, (uint8_t*)real_addr_v);
  return MAP_UPD_RET;
}

uint64_t compute_map_delete_helper(int addr_map, uint64_t addr_k, mem_t& m,
                                   simu_real& sr) {
  int map_id = addr_map;
  int k_sz = mem_t::map_key_sz(map_id) / NUM_BYTE_BITS;
  uint64_t real_addr_k = get_real_addr_by_simu(addr_k, m, sr);
  m.memory_access_check(real_addr_k, k_sz);
  string k = ld_n_bytes_from_addr((uint8_t*)real_addr_k, k_sz);
  map_t& mp = m._maps[map_id];
  auto it = mp._k2idx.find(k);
  if (it == mp._k2idx.end()) {
    return MAP_DEL_RET_IF_KEY_INEXIST;
  }
  mp.add_available_idx(it->second);
  mp._k2idx.erase(it);
  return MAP_DEL_RET_IF_KEY_EXIST;
}

z3::expr predicate_ldmapid(z3::expr map_id, z3::expr out, smt_var& sv) {
  sv.mem_var.add_ptr_by_map_id(out, map_id);
  return (map_id == out);
}

z3::expr predicate_st_byte(z3::expr in, z3::expr addr, z3::expr off, smt_var& sv, z3::expr cond) {
  z3::expr a = sv.update_mem_addr();
  z3::expr f = z3::implies(!cond, a == NULL_ADDR) &&
               z3::implies(cond, a == addr + off);
  int id = sv.mem_var.get_mem_table_id(addr);
  if (id == -1) return Z3_true; // todo: addr is not a pointer

  sv.mem_var.add_in_mem_table_wt(id, a, in.extract(7, 0));
  return f;
}

// this func is only called by map helper functions
z3::expr predicate_st_n_bytes(int n, z3::expr in, z3::expr addr, smt_var& sv,
                              z3::expr cond) {
  z3::expr f = string_to_expr("true");
  for (int i = 0; i < n; i++) {
    f = f && predicate_st_byte(in.extract(8 * i + 7, 8 * i), addr, to_expr(i, 64), sv, cond);
  }
  return f;
}

inline z3::expr addr_in_range(z3::expr addr, z3::expr start, z3::expr end) {
  return (uge(addr, start) && uge(end, addr));
}

z3::expr urt_element_constrain(z3::expr a, z3::expr v, smt_wt& urt) {
  z3::expr f = string_to_expr("true");
  // add constrains on the new symbolic value "v" according to the following cases:
  // case 1: "a" can be found in wt(addr1), the case is processed in
  // function "predicate_ld_byte", that is urt.add(new_addr, out)

  // case 2: "a" cannot be found in wt, but urt(addr1).
  // if there is no address equal to a in wt and addr1 in urt is equal to
  // a, it implies v is equal to the value of addr1
  for (int i = urt.addr.size() - 1; i >= 0; i--) {
    f = f && z3::implies((a != NULL_ADDR_EXPR) &&
                         (a == urt.addr[i]),
                         v == urt.val[i]);
  }
  // case 3: "a" cannot be found in wt or urt.
  // there is no constrains on the new symbolic value "v"
  return f;
}

z3::expr predicate_ld_byte(z3::expr addr, z3::expr off, smt_var& sv, z3::expr out, z3::expr cond) {
  int table_id = sv.mem_var.get_mem_table_id(addr);
  if (table_id == -1) return Z3_true; // todo: addr is not a pointer
  smt_wt& wt = sv.mem_var._mem_tables[table_id]._wt;
  z3::expr a = addr + off;
  z3::expr f_found_in_wt_after_i = string_to_expr("false");
  z3::expr f = string_to_expr("true");
  for (int i = wt.addr.size() - 1; i >= 0; i--) {
    f = f && z3::implies((!f_found_in_wt_after_i) &&
                         (a == wt.addr[i]) && (a != NULL_ADDR_EXPR),
                         out == wt.val[i]);
    f_found_in_wt_after_i = f_found_in_wt_after_i ||
                            ((a == wt.addr[i]) && (a != NULL_ADDR_EXPR));
  }

  // add constrains on the element(a, out)
  smt_wt& urt = sv.mem_var._mem_tables[table_id]._urt;
  z3::expr not_found_in_wt = sv.update_is_valid();
  f = f && (not_found_in_wt == (!f_found_in_wt_after_i));
  f = f && z3::implies(not_found_in_wt, urt_element_constrain(a, out, urt));
  // add element in urt
  z3::expr new_addr = sv.update_mem_addr();
  // if "a" can be found in wt, set "new_addr" as NULL_ADDR
  // to indicate that this entry to be added in the urt is invalid. else,
  // "new_addr" is "a"
  // An example that will cause a problem in equvialence check if just add (a, out) in urt
  // and add an entry in map URT using the same approach
  // example: pgm1: update k v m, lookup k m; pgm2: lookup k m
  // expected: pgm1 != pgm2, but if add (a, out) in urt, pgm1 == pgm2
  // Reason why pgm1 == pgm2: for the lookup in pgm1, because of the update, "k" can be found
  // in map WT, then the constrain on "addr_v" is that addr_v == addr_v' found in map WT,
  // and mem[addr_v'] == v because of the update. Thus, for pgm1, the update v is the same
  // as uninitialized lookup v.
  f = f && z3::implies((!not_found_in_wt) || (!cond), new_addr == NULL_ADDR_EXPR);
  f = f && z3::implies(not_found_in_wt && cond, new_addr == a);
  urt.add(new_addr, out);
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

z3::expr predicate_ld_n_bytes(int n, z3::expr addr, smt_var& sv, z3::expr out, z3::expr cond) {
  z3::expr f = predicate_ld_byte(addr, to_expr(0, 64), sv, out.extract(7, 0), cond);
  for (int i = 1; i < n; i++) {
    f = f && predicate_ld_byte(addr, to_expr(i, 64), sv, out.extract(8 * i + 7, 8 * i), cond);
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

// return the FOL formula that a (a!=null) can be found in x
// that is, for each x[i], a != x[i]
z3::expr addr_in_addrs(z3::expr& a, vector<z3::expr>& x) {
  z3::expr f = string_to_expr("false");
  for (int i = 0; i < x.size(); i++) {
    f = f || ((a != NULL_ADDR_EXPR) && (a == x[i]));
  }
  return f;
}

z3::expr stack_addr_in_one_wt(smt_wt& x, smt_wt& y,
                              z3::expr stack_start_addr, z3::expr stack_end_addr) {
  z3::expr f = string_to_expr("true");
  for (int i = x.addr.size() - 1; i >= 0; i--) {
    f = f && z3::implies(addr_in_range(x.addr[i], stack_start_addr, stack_end_addr) &&
                         (!addr_in_addrs(x.addr[i], y.addr)),
                         false);
  }
  return f;
}

// need memory WTs, stack memory range
z3::expr smt_stack_eq_chk(smt_wt& x, smt_wt& y,
                          z3::expr stack_start_addr, z3::expr stack_end_addr) {
  z3::expr f = string_to_expr("true");
  // case 1: addrs can be found in both WTs
  z3::expr f_addr_x = string_to_expr("true");
  z3::expr f_addr_y = string_to_expr("true");
  for (int i = x.addr.size() - 1; i >= 0; i--) {
    f_addr_x = addr_in_range(x.addr[i], stack_start_addr, stack_end_addr) &&
               latest_write_element(i, x.addr);
    for (int j = y.addr.size() - 1; j >= 0; j--) {
      f_addr_y = addr_in_range(y.addr[j], stack_start_addr, stack_end_addr) &&
                 latest_write_element(j, y.addr);
      f = f && z3::implies(f_addr_x && f_addr_y && (x.addr[i] == y.addr[j]),
                           x.val[i] == y.val[j]);
    }
  }
  // case 2: addrs can only be found in one WT.
  // If these addrs are found, the result is false.
  f = f && stack_addr_in_one_wt(x, y, stack_start_addr, stack_end_addr) &&
      stack_addr_in_one_wt(y, x, stack_start_addr, stack_end_addr);
  return f;
}

// pkt address only in mem_p1 wt => pkt address in mem_p1 urt && same value in wt and urt
z3::expr pkt_addr_in_one_wt(smt_var& sv1, smt_var& sv2) {
  z3::expr f = Z3_true;
  if (mem_t::_layout._pkt_sz == 0) return f;
  int id1 = sv1.mem_var.get_mem_table_id(MEM_TABLE_pkt);
  int id2 = sv2.mem_var.get_mem_table_id(MEM_TABLE_pkt);
  assert(id1 != -1);
  assert(id2 != -1);
  smt_wt& wt1 = sv1.mem_var._mem_tables[id1]._wt;
  smt_wt& wt2 = sv2.mem_var._mem_tables[id1]._wt;
  smt_wt& urt1 = sv1.mem_var._mem_tables[id2]._urt;
  for (int i = wt1.size() - 1; i >= 0; i--) {
    z3::expr a_out = wt1.addr[i];
    z3::expr v_out = wt1.val[i];
    z3::expr f_a_out = latest_write_element(i, wt1.addr);
    z3::expr f_a_not_in_wt2 = !addr_in_addrs(a_out, wt2.addr);
    z3::expr f_a_in_urt1 = addr_in_addrs(a_out, urt1.addr);
    f = f && z3::implies(f_a_out && f_a_not_in_wt2, f_a_in_urt1);

    for (int j = 0; j < urt1.size(); j++) {
      z3::expr a_in = urt1.addr[j];
      z3::expr v_in = urt1.val[j];
      f = f && z3::implies(f_a_out && f_a_not_in_wt2 && (a_out == a_in),
                           v_out == v_in);
    }
  }
  return f;
}

// 1. pkt address in both mem_p1 wt and mem_p2 wt => same value (latest write)
// 2. pkt address in one of wts => value (latest write) == input
z3::expr smt_pkt_eq_chk(smt_var& sv1, smt_var& sv2) {
  if (mem_t::_layout._pkt_sz == 0) return Z3_true;
  int id1 = sv1.mem_var.get_mem_table_id(MEM_TABLE_pkt);
  int id2 = sv2.mem_var.get_mem_table_id(MEM_TABLE_pkt);
  assert(id1 != -1);
  assert(id2 != -1);
  z3::expr f = Z3_true;
  smt_wt& wt1 = sv1.mem_var._mem_tables[id1]._wt;
  smt_wt& wt2 = sv2.mem_var._mem_tables[id2]._wt;
  // case 1: pkt address in both wts, latest write should be the same
  if ((wt1.size() > 0) && (wt2.size() > 0)) {
    for (int i = wt1.size() - 1; i >= 0; i--) {
      z3::expr a1 = wt1.addr[i];
      z3::expr v1 = wt1.val[i];
      z3::expr f_a1 = latest_write_element(i, wt1.addr);

      for (int j = wt2.size() - 1; j >= 0; j--) {
        z3::expr a2 = wt2.addr[j];
        z3::expr v2 = wt2.val[j];
        z3::expr f_a2 = latest_write_element(j, wt2.addr);
        f = f && z3::implies(f_a1 && f_a2 && (a1 == a2), v1 == v2);
      }
    }
  }
  // case 2: pkt address in one of wts
  f = f && pkt_addr_in_one_wt(sv1, sv2) && pkt_addr_in_one_wt(sv2, sv1);
  return f;
}

z3::expr key_not_found_after_idx(z3::expr key, int idx, smt_map_wt& m_wt) {
  z3::expr f_found = string_to_expr("false");
  int k_sz = key.get_sort().bv_size();
  for (int i = m_wt.size() - 1; i > idx; i--) {
    f_found = f_found || ((m_wt.is_valid[i] == Z3_true) &&
                          (key == m_wt.key[i]));
  }
  return (!f_found);
}

z3::expr key_not_in_map_wt(z3::expr k, smt_map_wt& m_wt) {
  z3::expr f = string_to_expr("false");
  for (int i = 0; i < m_wt.key.size(); i++) {
    f = f || ((m_wt.is_valid[i] == Z3_true) &&
              (k == m_wt.key[i]));
  }
  return (!f);
}

z3::expr ld_byte_from_wt(z3::expr addr, smt_wt& wt, z3::expr out) {
  z3::expr a = addr;
  z3::expr f = string_to_expr("true");
  z3::expr f_found_after_i = string_to_expr("false");
  for (int i = wt.addr.size() - 1; i >= 0; i--) {
    z3::expr f_found_i = (a == wt.addr[i]) && (a != NULL_ADDR_EXPR);
    f = f && z3::implies((!f_found_after_i) && f_found_i, // latest write
                         out == wt.val[i]);
    f_found_after_i = f_found_after_i || f_found_i;
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
// premise: 1. key not in map2 wt 2. !(key in map2 URT and mem2 WT)
// for each key(k) in these keys, the latest write m[k] should be equal to the input
// thus, before writing, this k/v should be read from the map, that is, the element can be found in map URT
z3::expr keys_found_in_one_map(int map_id, smt_var& sv1, smt_var& sv2) {
  z3::expr f = string_to_expr("true");
  int k_sz = mem_t::map_key_sz(map_id);

  smt_map_wt& map_wt = sv1.mem_var._map_tables[map_id]._wt;
  smt_map_wt& map2_wt = sv2.mem_var._map_tables[map_id]._wt;
  smt_map_wt& map_urt = sv1.mem_var._map_tables[map_id]._urt;
  smt_map_wt& map2_urt = sv2.mem_var._map_tables[map_id]._urt;
  int mem_id1 = sv1.mem_var.get_mem_table_id(MEM_TABLE_map, map_id);
  int mem_id2 = sv2.mem_var.get_mem_table_id(MEM_TABLE_map, map_id);
  smt_wt& mem_wt = sv1.mem_var._mem_tables[mem_id1]._wt;
  smt_wt& mem2_wt = sv2.mem_var._mem_tables[mem_id2]._wt;
  smt_wt& mem_urt = sv1.mem_var._mem_tables[mem_id1]._urt;
  int v_sz = mem_t::map_val_sz(map_id);
  int v_sz_byte = v_sz / NUM_BYTE_BITS;

  // compute and store memory load constrains on v for map URT
  // to avoid repetitive computation
  vector<z3::expr> v_in, f_v_in;
  for (int i = 0; i < map_urt.size(); i++) {
    z3::expr v = sv1.update_val(v_sz);
    z3::expr addr_v = map_urt.addr_v[i];
    z3::expr f_v = ld_n_bytes_from_urt(v_sz_byte, addr_v, mem_urt, v);
    v_in.push_back(v);
    f_v_in.push_back(f_v);
  }
  vector<int> map2_urt_entry_idx;
  vector<z3::expr> f_map2_urt_mem2_wt;
  for (int i = 0; i < map2_urt.size(); i++) {
    z3::expr k2 = map2_urt.key[i];
    map2_urt_entry_idx.push_back(i);
    z3::expr addr_v2 = map2_urt.addr_v[i];
    f_map2_urt_mem2_wt.push_back(addr_in_addrs(addr_v2, mem2_wt.addr));
  }

  for (int i = map_wt.size() - 1; i >= 0; i--) {
    z3::expr k_out = map_wt.key[i];
    // 1. key not in map2 wt 2. !(key in map2 URT and mem2 WT)
    z3::expr f_k_not_in_map2_wt = key_not_found_after_idx(k_out, -1, map2_wt);
    z3::expr f_k_in_map2_urt_mem2_wt = Z3_false;
    for (int j = 0; j < map2_urt_entry_idx.size(); j++) {
      int idx = map2_urt_entry_idx[j];
      z3::expr k2 = map2_urt.key[idx];
      z3::expr is_valid = map2_urt.is_valid[idx];
      z3::expr addr_v2 = map2_urt.addr_v[idx];

      z3::expr f_k_in_map2_urt = (is_valid == Z3_true) && (k_out == k2);
      z3::expr f_addr_v_in_mem2_wt = f_map2_urt_mem2_wt[j];
      f_k_in_map2_urt_mem2_wt = f_k_in_map2_urt_mem2_wt || (f_k_in_map2_urt && f_addr_v_in_mem2_wt);
    }

    z3::expr f_k_wt_latest_write = key_not_found_after_idx(k_out, i, map_wt) &&
                                   (map_wt.is_valid[i] == Z3_true);
    z3::expr v_out = sv1.update_val(v_sz);
    z3::expr addr_v_out = map_wt.addr_v[i];
    z3::expr f_v_out = ld_n_bytes_from_wt(v_sz_byte, addr_v_out, mem_wt, v_out);

    // k_out should be found in map URT
    // (if output the same as input, there is uninitialized lookup)
    z3::expr f_found_same_k_in_urt = string_to_expr("false");
    for (int j = map_urt.size() - 1; j >= 0; j--) {
      z3::expr k_in = map_urt.key[j];

      z3::expr f_found_same_k_j = (map_urt.is_valid[j] == Z3_true) &&
                                  (k_out == k_in);
      z3::expr f_found_same_k = f_k_not_in_map2_wt && (!f_k_in_map2_urt_mem2_wt) &&
                                f_k_wt_latest_write && f_found_same_k_j;

      z3::expr addr_v_in = map_urt.addr_v[j];
      z3::expr f_k_both_inexist = (addr_v_out == NULL_ADDR_EXPR) && (addr_v_in == NULL_ADDR_EXPR);
      z3::expr f_k_both_exist = (addr_v_out != NULL_ADDR_EXPR) && (addr_v_in != NULL_ADDR_EXPR);
      f = f && z3::implies(f_found_same_k && f_v_out && f_v_in[j],
                           f_k_both_inexist || (f_k_both_exist && (v_out == v_in[j])));

      f_found_same_k_in_urt = f_found_same_k_in_urt || f_found_same_k_j;
    }
    // add the constrains for the case that not found the same key
    f = f && z3::implies(f_k_not_in_map2_wt && (!f_k_in_map2_urt_mem2_wt) &&
                         (map_wt.is_valid[i] == Z3_true) &&
                         (!f_found_same_k_in_urt),
                         string_to_expr("false"));
  }
  return f;
}

// map_tbl can be map WT or map URT, only load v from mem WT
void constrains_on_vs_by_map_tbl_addr_vs(vector<z3::expr>& vs,
    vector<z3::expr>& f_vs, smt_map_wt& map_tbl, smt_var& sv, int v_sz_bit, int mem_id) {
  vs.clear();
  f_vs.clear();
  for (int i = 0; i < map_tbl.size(); i++) {
    z3::expr v = sv.update_val(v_sz_bit);
    z3::expr addr_v = map_tbl.addr_v[i];
    smt_wt& mem_wt = sv.mem_var._mem_tables[mem_id]._wt;
    z3::expr f_v = ld_n_bytes_from_wt(v_sz_bit / NUM_BYTE_BITS, addr_v, mem_wt, v);
    vs.push_back(v);
    f_vs.push_back(f_v);
  }
}

// constrains on one key that has write record in both maps
// k1 is from map1 and has write record in map1
z3::expr one_key_found_in_both_maps(int map_id, bool is_map1_wt, z3::expr k1, z3::expr addr_v1, z3::expr v1,
                                    smt_var& sv2, vector<z3::expr>& v2_list, vector<z3::expr>& f_v2_list,
                                    vector<z3::expr>& v2_urt_list, vector<z3::expr>& f_v2_urt_list,
                                    vector<z3::expr>& f_addr_v2_in_mem2_wt_list) {
  z3::expr f = Z3_true;
  /* case k1 in the map2 WT */
  z3::expr f_k1_found_after_i = string_to_expr("false");
  map_wt& map2 = sv2.mem_var._map_tables[map_id];
  for (int i = map2._wt.size() - 1; i >= 0; i--) {
    z3::expr k2 = map2._wt.key[i];

    z3::expr f_k1_found_i = (map2._wt.is_valid[i] == Z3_true) && (k1 == k2);
    z3::expr f_found_same_key = (!f_k1_found_after_i) && f_k1_found_i;
    z3::expr addr_v2 = map2._wt.addr_v[i];
    z3::expr f_k1_both_exist = (addr_v1 != NULL_ADDR_EXPR) && (addr_v2 != NULL_ADDR_EXPR);
    z3::expr f_k1_both_inexist = (addr_v1 == NULL_ADDR_EXPR) && (addr_v2 == NULL_ADDR_EXPR);
    if (is_map1_wt) {
      f = f && z3::implies(f_found_same_key && f_v2_list[i],
                           f_k1_both_inexist || (f_k1_both_exist && (v1 == v2_list[i])));
    } else {
      // is_map1_urt, only consider the case that k is in the map1, and addr_v in the mem1 WT
      // the latter constrain has been added before calling this function
      f = f && z3::implies(f_found_same_key && f_v2_list[i] && f_k1_both_exist,
                           v1 == v2_list[i]);
    }
    f_k1_found_after_i = f_k1_found_after_i || f_k1_found_i;
  }

  /* case k1 not in the map2 WT, but in the map2 URT, and addr_v is in mem2 WT */
  z3::expr f1 = Z3_true;
  int mem_table_id2 = sv2.mem_var.get_mem_table_id(MEM_TABLE_map, map_id);
  mem_table& mem2 = sv2.mem_var._mem_tables[mem_table_id2];
  for (int i = 0; i < map2._urt.size(); i++) {
    z3::expr k2 = map2._urt.key[i];

    z3::expr f_found_same_key = (map2._urt.is_valid[i] == Z3_true) && (k1 == k2);
    z3::expr addr_v2 = map2._urt.addr_v[i];
    z3::expr f_k1_both_exist = (addr_v1 != NULL_ADDR_EXPR) && (addr_v2 != NULL_ADDR_EXPR);
    z3::expr f_k1_both_inexist = (addr_v1 == NULL_ADDR_EXPR) && (addr_v2 == NULL_ADDR_EXPR);
    // only consider the case that k is in the map and addr_v is in the mem WT
    f1 = f1 && z3::implies(f_found_same_key && f_v2_urt_list[i] &&
                           f_addr_v2_in_mem2_wt_list[i] && f_k1_both_exist,
                           v1 == v2_urt_list[i]);
  }
  z3::expr f_k1_not_in_map2_wt = key_not_in_map_wt(k1, map2._wt);
  f = f && z3::implies(f_k1_not_in_map2_wt, f1);
  return f;
}

z3::expr smt_one_map_eq_chk(int map_id, smt_var& sv1, smt_var& sv2) {
  z3::expr f = string_to_expr("true");
  int k_sz = mem_t::map_key_sz(map_id);
  int v_sz = mem_t::map_val_sz(map_id);
  int mem_table_id1 = sv1.mem_var.get_mem_table_id(MEM_TABLE_map, map_id);
  int mem_table_id2 = sv2.mem_var.get_mem_table_id(MEM_TABLE_map, map_id);
  // case 1: keys that are in pgm1's map WT and pgm2's map WT
  // if the key is found && load v1 from mem1 WT && load v2 from mem2 WT => 1 or 2
  // 1. addr_v1 == addr_v2 == NULL
  // 2. addr_v1 != NULL && addr_v2 != NULL && v1 == v2
  map_wt& map1 = sv1.mem_var._map_tables[map_id], map2 = sv2.mem_var._map_tables[map_id];
  // compute and store memory load constrains on v for map2 WT
  // to avoid repetitive computation
  vector<z3::expr> v2_list, f_v2_list, v2_urt_list, f_v2_urt_list, f_addr_v2_in_mem2_wt_list;
  constrains_on_vs_by_map_tbl_addr_vs(v2_list, f_v2_list, map2._wt, sv2, v_sz, mem_table_id2);
  constrains_on_vs_by_map_tbl_addr_vs(v2_urt_list, f_v2_urt_list, map2._urt, sv2, v_sz, mem_table_id2);
  mem_table& mem2 = sv2.mem_var._mem_tables[mem_table_id2];
  for (int i = 0; i < map2._urt.size(); i++) {
    z3::expr addr_v2 = map2._urt.addr_v[i];
    f_addr_v2_in_mem2_wt_list.push_back(addr_in_addrs(addr_v2, mem2._wt.addr));
  }
  /* For each key(k) in the map1, check (k in map2) => map1[k] == map2[k] */
  /* case 1: key in the map1 WT
     1. k write records in map2:
        1.1 records in map2 WT:
          1.1.1 addr_v1 == addr_v2 == NULL (both delete in two map WTs)
          1.1.2 map1[k] == map2[k] (update or lookup+st)
        2.1 no records in map2 WT, but records in map2 URT and mem2 WT
          map1[k] == map2[k] (pgm1: update or lookup+st, pgm2: uinitialized lookup + st)
     2. no k write records in map2: pgm2 did not update/delete k
        v1_out == v1_in, i.e., pgm1 did not change v1
   */
  for (int i = 0; i < map1._wt.size(); i++) {
    z3::expr k1 = map1._wt.key[i];

    /* add constrains on v1, i.e., map1[k1]: load v1 from addr_v1 in mem WT (mem URT does not
       need to be considered, since it's only for memory uinitialized read, not for write)
     */
    z3::expr v1 = sv1.update_val(v_sz);
    z3::expr addr_v1 = map1._wt.addr_v[i];
    z3::expr f_kv_constrain = key_not_found_after_idx(k1, i, map1._wt) && // latest write
                              (map1._wt.is_valid[i] == Z3_true);
    f_kv_constrain = f_kv_constrain &&
                     ld_n_bytes_from_wt(v_sz / NUM_BYTE_BITS, addr_v1,
                                        sv1.mem_var._mem_tables[mem_table_id1]._wt, v1);
    bool is_map1_wt = true;
    f = f && z3::implies(f_kv_constrain,
                         one_key_found_in_both_maps(map_id, is_map1_wt, k1, addr_v1, v1,
                             sv2, v2_list, f_v2_list, v2_urt_list, f_v2_urt_list,
                             f_addr_v2_in_mem2_wt_list));
  }

  mem_table& mem1 = sv1.mem_var._mem_tables[mem_table_id1];
  for (int i = 0; i < map1._urt.size(); i++) {
    z3::expr k1 = map1._urt.key[i];

    z3::expr v1 = sv1.update_val(v_sz);
    z3::expr addr_v1 = map1._urt.addr_v[i];
    z3::expr f_kv_constrain = (map1._urt.is_valid[i] == Z3_true);
    f_kv_constrain = f_kv_constrain &&
                     ld_n_bytes_from_wt(v_sz / NUM_BYTE_BITS, addr_v1,
                                        sv1.mem_var._mem_tables[mem_table_id1]._wt, v1);
    z3::expr f_k1_not_in_map1_wt = key_not_in_map_wt(k1, map1._wt);
    z3::expr f_addr_v1_in_mem1_wt = addr_in_addrs(addr_v1, mem1._wt.addr);
    bool is_map1_wt = false;
    f = f && z3::implies(f_k1_not_in_map1_wt && f_addr_v1_in_mem1_wt && f_kv_constrain,
                         one_key_found_in_both_maps(map_id, is_map1_wt, k1, addr_v1, v1,
                             sv2, v2_list, f_v2_list, v2_urt_list, f_v2_urt_list,
                             f_addr_v2_in_mem2_wt_list));
  }
  // case 2: keys that are only in one of pgm1's map and pgm2's map
  // if the key is found, the output should be the same as the input
  // take map1 for an example,
  // premise: 1. key not in map2 wt 2. !(key in map2 URT and mem2 WT)
  // if the key not found in map2 && k_out == k_in &&load v_out from mem1 WT &&
  // load v_in from mem1 URT => 1 or 2
  // 1. addr_v_out == addr_v_in == NULL
  // 2. addr_v_out != NULL && addr_v_in != NULL && v_in == v_out
  f = f && keys_found_in_one_map(map_id, sv1, sv2) &&
      keys_found_in_one_map(map_id, sv2, sv1);
  return f;
}

// add the constrains on the input equivalence setting
// the same k/v in map_p1 URT == map_p2 URT
// each key "k" in map_p1 URT, if "k" is in map_p2 URT => 1 or 2
// 1. "k" is not in the input map, i.e., the corresponding addr_v both NULL
// 2. "k" is in the input map, load v_p1 and v_p2 from mem URT, the corresponding v_p1 == v_p2
z3::expr smt_one_map_set_same_input(int map_id, smt_var& sv1, smt_var& sv2) {
  z3::expr f = string_to_expr("true");
  int k_sz = mem_t::map_key_sz(map_id);
  int v_sz = mem_t::map_val_sz(map_id);
  smt_map_wt& map1_urt = sv1.mem_var._map_tables[map_id]._urt;
  smt_map_wt& map2_urt = sv2.mem_var._map_tables[map_id]._urt;
  int mem_table_id1 = sv1.mem_var.get_mem_table_id(MEM_TABLE_map, map_id);
  int mem_table_id2 = sv2.mem_var.get_mem_table_id(MEM_TABLE_map, map_id);

  // compute and store memory load constrains on v for map2 URT
  // to avoid repetitive computation
  vector<z3::expr> v2, f_v2;
  for (int i = 0; i < map2_urt.size(); i++) {
    z3::expr addr_v = map2_urt.addr_v[i];
    z3::expr v = sv2.update_val(v_sz);
    smt_wt& mem_urt = sv2.mem_var._mem_tables[mem_table_id2]._urt;
    z3::expr f_v = ld_n_bytes_from_urt(v_sz / NUM_BYTE_BITS, addr_v, mem_urt, v);
    v2.push_back(v);
    f_v2.push_back(f_v);
  }

  for (int i = 0; i < map1_urt.size(); i++) {
    z3::expr k1 = map1_urt.key[i];

    z3::expr addr_v1 = map1_urt.addr_v[i];
    z3::expr v1 = sv1.update_val(v_sz);
    smt_wt& mem1_urt = sv1.mem_var._mem_tables[mem_table_id1]._urt;

    z3::expr f_v1 = ld_n_bytes_from_urt(v_sz / NUM_BYTE_BITS, addr_v1, mem1_urt, v1);
    for (int j = 0; j < map2_urt.size(); j++) {
      z3::expr k2 = map2_urt.key[j];
      z3::expr addr_v2 = map2_urt.addr_v[j];

      z3::expr f_k_both_inexist = (addr_v1 == NULL_ADDR_EXPR) && (addr_v2 == NULL_ADDR_EXPR);
      z3::expr f_k_both_exist = (addr_v1 != NULL_ADDR_EXPR) && (addr_v2 != NULL_ADDR_EXPR);
      // set the constrains on addr_v1, addr_v2, v1, v2
      f = f && z3::implies((map1_urt.is_valid[i] == Z3_true) &&
                           (map2_urt.is_valid[j] == Z3_true) &&
                           (k1 == k2),
                           f_k_both_inexist ||
                           (f_k_both_exist && f_v1 && f_v2[j] && (v1 == v2[j])));
    }
  }
  return f;
}

z3::expr smt_map_set_same_input(smt_var& sv1, smt_var& sv2) {
  z3::expr f = string_to_expr("true");
  for (int map_id = 0; map_id < mem_t::maps_number(); map_id++) {
    f = f && smt_one_map_set_same_input(map_id, sv1, sv2);
  }
  return f;
}

z3::expr smt_map_eq_chk(smt_var& sv1, smt_var& sv2) {
  z3::expr f = string_to_expr("true");
  for (int map_id = 0; map_id < mem_t::maps_number(); map_id++) {
    f = f && smt_one_map_eq_chk(map_id, sv1, sv2);
  }
  return f;
}

z3::expr smt_pgm_mem_eq_chk(smt_var& sv1, smt_var& sv2) {
  return smt_map_eq_chk(sv1, sv2) && smt_pkt_eq_chk(sv1, sv2);
}

// add the constrains on the input equivalence setting
// the same content in the same pkt address in mem_p1 URT == mem_p2 URT
z3::expr smt_pkt_set_same_input(smt_var& sv1, smt_var& sv2) {
  if (mem_t::_layout._pkt_sz == 0) return Z3_true;
  int id1 = sv1.mem_var.get_mem_table_id(MEM_TABLE_pkt);
  int id2 = sv2.mem_var.get_mem_table_id(MEM_TABLE_pkt);
  assert(id1 != -1);
  assert(id2 != -1);
  z3::expr f = Z3_true;
  smt_wt& mem1_urt = sv1.mem_var._mem_tables[id1]._urt;
  smt_wt& mem2_urt = sv2.mem_var._mem_tables[id2]._urt;
  bool cond = (mem1_urt.size() > 0) && (mem2_urt.size() > 0);
  if (!cond) return f;

  for (int i = 0; i < mem1_urt.size(); i++) {
    z3::expr a1 = mem1_urt.addr[i];
    z3::expr v1 = mem1_urt.val[i];
    z3::expr f_a1 = (a1 != NULL_ADDR_EXPR);

    for (int j = 0; j < mem2_urt.size(); j++) {
      z3::expr a2 = mem2_urt.addr[j];
      z3::expr v2 = mem2_urt.val[j];

      f = f && z3::implies(f_a1 && (a1 == a2), v1 == v2);
    }
  }
  return f;
}

z3::expr smt_pgm_set_same_input(smt_var& sv1, smt_var& sv2) {
  z3::expr f = smt_map_set_same_input(sv1, sv2) &&
               smt_pkt_set_same_input(sv1, sv2);
  return f;
}

// set constrains on addr_map_v
z3::expr predicate_map_lookup_k_in_map_wt(z3::expr k, z3::expr addr_map_v, smt_map_wt& m_wt) {
  z3::expr f = string_to_expr("true");
  z3::expr key_found_after_i = string_to_expr("false");
  for (int i = m_wt.key.size() - 1; i >= 0; i--) {
    // the same key in the same map
    z3::expr key_found_i = (m_wt.is_valid[i] == Z3_true) && // valid entry
                           (k == m_wt.key[i]);
    f = f && z3::implies((!key_found_after_i) && key_found_i, // latest write of key-value pair in the map
                         addr_map_v == m_wt.addr_v[i]);
    key_found_after_i = key_found_after_i || key_found_i;
  }
  return f;
}

// constrain: (k not in map WT) && (map == map_i) && (k == k_i) => addr_v == addr_vi
z3::expr predicate_map_lookup_k_in_map_urt(z3::expr k, z3::expr addr_map_v, map_wt& map_table) {
  z3::expr f = string_to_expr("true");
  z3::expr f1 = key_not_in_map_wt(k, map_table._wt);
  for (int i = map_table._urt.key.size() - 1; i >= 0; i--) {
    z3::expr f2 = (map_table._urt.is_valid[i] == Z3_true) &&
                  (k == map_table._urt.key[i]);
    f = f && z3::implies(f1 && f2, addr_map_v == map_table._urt.addr_v[i]);
  }
  return f;
}

// "addr_map_v" is the return value
z3::expr predicate_map_lookup_helper(z3::expr addr_map, z3::expr addr_k, z3::expr addr_map_v,
                                     smt_var& sv, z3::expr cond) {
  z3::expr f_ret = string_to_expr("true");
  smt_mem& mem = sv.mem_var;
  // get map id according to addr_map
  int map_id = mem.get_map_id_by_ptr(addr_map);
  if (map_id == -1) return f_ret; // todo: addr_map is not a pointer

  int k_sz = mem_t::map_key_sz(map_id);
  z3::expr k = sv.update_key(k_sz);

  /* add constrains on k */
  z3::expr f = predicate_ld_n_bytes(k_sz / NUM_BYTE_BITS, addr_k, sv, k, cond);

  /* add constrains on addr_map_v for the following cases
     if key is in the target map, addr_map_v is the same as the corresponding
     value address in the map; else addr_map_v is either NULL or a new address
     to simulate the uncertainty of input map.
  */
  // case 1: check k in the map WT
  f = f && predicate_map_lookup_k_in_map_wt(k, addr_map_v, mem._map_tables[map_id]._wt);
  // case 2: k not in the map WT, check k in the map URT
  f = f && predicate_map_lookup_k_in_map_urt(k, addr_map_v, mem._map_tables[map_id]);
  // case 3: k is neither in the map WT nor the map URT
  z3::expr f1 = key_not_in_map_wt(k, mem._map_tables[map_id]._wt);
  z3::expr f2 = key_not_in_map_wt(k, mem._map_tables[map_id]._urt);
  f = f && z3::implies(f1 && f2,
                       (addr_map_v == NULL_ADDR_EXPR) ||
                       (addr_map_v == mem.get_and_update_addr_v_next(map_id)));

  z3::expr is_valid = sv.update_is_valid(); // z3 boolean const
  /* add the constrains on "is_valid" */
  // if k is in map WT, set is_valid is false to
  // indicate this entry to be added in the map URT is invalid.
  f_ret = f_ret && z3::implies((!cond) || (!f1), is_valid == Z3_false);
  f_ret = f_ret && z3::implies(cond && f1, is_valid == Z3_true);
  f_ret = f_ret && f;

  mem.add_ptr_by_map_id(addr_map_v, map_id); // todo: what if addr_map_v == NULL
  mem._map_tables[map_id]._urt.add(is_valid, k, addr_map_v);
  return f_ret;
}

// "out" is the return value
z3::expr predicate_map_update_helper(z3::expr addr_map, z3::expr addr_k, z3::expr addr_v,
                                     z3::expr out, smt_var& sv, z3::expr cond) {
  z3::expr f_ret = string_to_expr("true");
  smt_mem& mem = sv.mem_var;
  // get map id according to addr_map
  int map_id = mem.get_map_id_by_ptr(addr_map);
  if (map_id == -1) return f_ret; // todo: addr_map is not a pointer

  z3::expr cur_addr_map = to_expr((int64_t)map_id);
  int k_sz = mem_t::map_key_sz(map_id);
  int v_sz = mem_t::map_val_sz(map_id);
  z3::expr k = sv.update_key(k_sz);
  z3::expr v = sv.update_val(v_sz);
  z3::expr addr_map_v = sv.update_addr_v();
  /* add constrains on "out", "k", "v" */
  z3::expr f = (out == MAP_UPD_RET_EXPR) &&
               predicate_ld_n_bytes(k_sz / NUM_BYTE_BITS, addr_k, sv, k, cond) &&
               predicate_ld_n_bytes(v_sz / NUM_BYTE_BITS, addr_v, sv, v, cond);
  z3::expr next_addr_map_v = mem.get_and_update_addr_v_next(map_id);
  /* add constrains on "addr_map_v".
     if the key is in the target map WT. if the corresponding value address is NULL,
     the value address is the same as the corresponding value address.
     else, assign a new address to the value address.
  */
  // case 1, key is in the map WT
  z3::expr f_key_found_after_i = string_to_expr("false");
  for (int i = mem._map_tables[map_id]._wt.key.size() - 1; i >= 0; i--) { // latest entry
    smt_map_wt& m_wt = mem._map_tables[map_id]._wt;
    z3::expr key_found_i = (m_wt.is_valid[i] == Z3_true) && // valid entry
                           (k == m_wt.key[i]); //&& // the same key
    z3::expr f1 = z3::implies(m_wt.addr_v[i] != NULL_ADDR_EXPR, addr_map_v == m_wt.addr_v[i]);
    f1 = f1 && z3::implies(m_wt.addr_v[i] == NULL_ADDR_EXPR, addr_map_v == next_addr_map_v);
    f = f && z3::implies((!f_key_found_after_i) && key_found_i, f1);

    f_key_found_after_i = f_key_found_after_i || key_found_i;
  }
  // case 2: if key is not in the map WT but the map URT
  z3::expr f_not_found_in_wt = (!f_key_found_after_i);
  z3::expr f_found_in_urt = string_to_expr("false");
  smt_map_wt& m_urt = mem._map_tables[map_id]._urt;
  for (int i = 0; i < m_urt.size(); i++) {
    z3::expr key_found_i = (m_urt.is_valid[i] == Z3_true) && // valid entry
                           (k == m_urt.key[i]);// && // the same key
    z3::expr f1 = z3::implies(m_urt.addr_v[i] != NULL_ADDR_EXPR, addr_map_v == m_urt.addr_v[i]);
    f1 = f1 && z3::implies(m_urt.addr_v[i] == NULL_ADDR_EXPR, addr_map_v == m_urt.addr_v[i]);
    f = f && z3::implies(f_not_found_in_wt && key_found_i, f1);

    f_found_in_urt = f_found_in_urt || key_found_i;
  }
  // case 3: if the key is not in the target map
  f = f && z3::implies(f_not_found_in_wt && (!f_found_in_urt), addr_map_v == next_addr_map_v);
  /* add the constrains on "is_valid" */
  z3::expr is_valid = sv.update_is_valid();
  f_ret = f_ret && z3::implies(!cond, is_valid == Z3_false);
  f_ret = f_ret && z3::implies(cond, is_valid == Z3_true);
  f_ret = f_ret && f;
  // add the update entry in map WT
  mem._map_tables[map_id]._wt.add(is_valid, k, addr_map_v);
  // add the update entry in memory WT
  mem.add_ptr_by_map_id(addr_map_v, map_id); // todo: what if addr_map_v == NULL
  f_ret = f_ret && predicate_st_n_bytes(v_sz / NUM_BYTE_BITS, v, addr_map_v, sv, is_valid);

  return f_ret;
}

// "out" is the return value
// if key not in the map, out = 0xfffffffe, else out = 0
z3::expr predicate_map_delete_helper(z3::expr addr_map, z3::expr addr_k, z3::expr out,
                                     smt_var& sv, z3::expr cond) {
  z3::expr f_ret = string_to_expr("true");
  smt_mem& mem = sv.mem_var;
  // get map id according to addr_map
  int map_id = mem.get_map_id_by_ptr(addr_map);
  if (map_id == -1) return f_ret; // todo: addr_map is not a pointer

  z3::expr cur_addr_map = to_expr((int64_t)map_id);
  int k_sz = mem_t::map_key_sz(map_id);
  z3::expr k = sv.update_key(k_sz);
  /* add the constrains on "k" */
  z3::expr f = predicate_ld_n_bytes(k_sz / NUM_BYTE_BITS, addr_k, sv, k, cond);

  /* add the constrains on "addr_map_v" according to "k" */
  z3::expr addr_map_v = sv.update_addr_v();
  // if k is in the map WT, set constrains on addr_map_v
  f = f && predicate_map_lookup_k_in_map_wt(k, addr_map_v, mem._map_tables[map_id]._wt);
  // if k is not in the map WT but in map URT, set constrains on addr_map_v
  f = f && predicate_map_lookup_k_in_map_urt(k, addr_map_v, mem._map_tables[map_id]);

  z3::expr f1 = key_not_in_map_wt(k, mem._map_tables[map_id]._wt);
  z3::expr f2 = key_not_in_map_wt(k, mem._map_tables[map_id]._urt);
  z3::expr f3 = f1 && f2;
  // if k is neither in the map WT nor the map URT
  f = f && z3::implies(f3, (addr_map_v == NULL_ADDR_EXPR) ||
                       (addr_map_v == mem.get_and_update_addr_v_next(map_id)));

  /* add the constrains on "out" according to "addr_map_v" */
  f = f && z3::implies(addr_map_v == NULL_ADDR_EXPR,
                       out == MAP_DEL_RET_IF_KEY_INEXIST_EXPR);
  f = f && z3::implies(addr_map_v != NULL_ADDR_EXPR,
                       out == MAP_DEL_RET_IF_KEY_EXIST_EXPR);
  f_ret = f_ret && f;

  z3::expr is_valid = sv.update_is_valid();
  // add an entry in map WT to delete this key
  f_ret = f_ret && z3::implies(!cond, is_valid == Z3_false);
  f_ret = f_ret && z3::implies(cond, is_valid == Z3_true);
  mem._map_tables[map_id]._wt.add(is_valid, k, NULL_ADDR_EXPR);
  // add an entry in map URT to show lookup
  // only it is the target map, and k cannot be found in map WT is valid
  is_valid = sv.update_is_valid();
  f_ret = f_ret && z3::implies(!(cond && f1), is_valid == Z3_false);
  f_ret = f_ret && z3::implies(cond && f1, is_valid == Z3_true);

  mem.add_ptr_by_map_id(addr_map_v, map_id); // todo: what if addr_map_v == NULL
  mem._map_tables[map_id]._urt.add(is_valid, k, addr_map_v);

  return f_ret;
}

z3::expr predicate_helper_function(int func_id, z3::expr r1, z3::expr r2, z3::expr r3,
                                   z3::expr r4, z3::expr r5, z3::expr r0,
                                   smt_var &sv, z3::expr cond) {
  if (func_id == BPF_FUNC_map_lookup) {
    return predicate_map_lookup_helper(r1, r2, r0, sv, cond);
  } else if (func_id == BPF_FUNC_map_update) {
    return predicate_map_update_helper(r1, r2, r3, r0, sv, cond);
  } else if (func_id == BPF_FUNC_map_delete) {
    return predicate_map_delete_helper(r1, r2, r0, sv, cond);
  } else {
    cout << "Error: unknown function id " << func_id << endl; return string_to_expr("true");
  }
}
/*
// should make sure that the input "z3_bv8" is a z3 bv8 but not a formula
inline string z3_bv8_2_hex_str(z3::expr z3_bv8) {
  assert(z3_bv8.is_numeral());
  int i = z3_bv8.get_numeral_int();
  stringstream ss;
  ss << hex << setfill('0') << setw(2) << i;
  return ss.str();
}

string z3_bv_2_hex_str(z3::expr z3_bv) {
  int sz = z3_bv.get_sort().bv_size();
  string hex_str = "";
  for (int i = sz - 1; i >= 0; i -= 8) {
    // simplify() simplifies the "z3_bv.extract(i, i - 7)" formula to a z3 bv8
    hex_str += z3_bv8_2_hex_str(z3_bv.extract(i, i - 7).simplify());
  }
  return hex_str;
}

// return uint64 for the given z3 bv64 constant
// if z3 bv64 is not a constant: 1. assert flag is true, assert(false)
// 2. assert flag is false, return 0
uint64_t get_uint64_from_bv64(z3::expr& z3_val, bool assert) {
  bool is_num = z3_val.is_numeral();
  if (is_num) return z3_val.get_numeral_uint64();

  if (assert) {
    assert(false);
  } else {
    return 0;
  }
}

// get an addr-val list. The list only contains useful memory addresses (map, pkt)
void get_mem_from_mdl(vector<pair<uint64_t, uint8_t>>& mem_addr_val,
                      z3::model& mdl, smt_var& sv) {
  smt_wt& mem_urt = sv.mem_var._mem_table._urt;
  int map_sz = mem_t::maps_number();
  unsigned int pkt_sz = mem_t::_layout._pkt_sz;
  // only record the addr which is in map or pkt memory range
  if ((map_sz == 0) && (pkt_sz == 0)) return;
  // assume all maps are next to one another

  uint64_t map_start = 0, map_end = 0;
  if (map_sz != 0) {
    z3::expr z3_map_start = mdl.eval(sv.get_map_start_addr(0));
    z3::expr z3_map_end = mdl.eval(sv.get_map_end_addr(map_sz - 1));
    map_start = get_uint64_from_bv64(z3_map_start, false);
    map_end = get_uint64_from_bv64(z3_map_end, false);
  }

  uint64_t pkt_start = 0, pkt_end = 0;
  if (pkt_sz != 0) {
    z3::expr z3_pkt_start = mdl.eval(sv.get_pkt_start_addr());
    z3::expr z3_pkt_end = mdl.eval(sv.get_pkt_end_addr());
    pkt_start = get_uint64_from_bv64(z3_pkt_start, false);
    pkt_end = get_uint64_from_bv64(z3_pkt_end, false);
  }

  for (int i = 0; i < mem_urt.size(); i++) {
    z3::expr z3_addr = mem_urt.addr[i];
    z3::expr z3_addr_eval = mdl.eval(z3_addr);
    if (!z3_addr_eval.is_numeral()) continue;
    uint64_t addr = z3_addr_eval.get_numeral_uint64();
    // addr whose value is NULL means the item <addr, val> is invalid
    if (addr == NULL_ADDR) continue;
    bool addr_not_in_map = true;
    bool addr_not_in_pkt = true;
    if (map_sz != 0) addr_not_in_map = (addr < map_start) || (addr > map_end);
    if (pkt_sz != 0) addr_not_in_pkt = (addr < pkt_start) || (addr > pkt_end);
    if (addr_not_in_map && addr_not_in_pkt) continue;

    z3::expr z3_val = mdl.eval(mem_urt.val[i]); // z3 bv8
    // z3 does not care about "z3_val"'s val if z3_val is not numeral
    if (! z3_val.is_numeral()) continue;
    uint8_t val = (uint8_t)z3_val.get_numeral_int();
    mem_addr_val.push_back(make_pair(addr, val));
  }
}

// load v from addr_v in mem_addr_val;
// if v does not in mem_addr_val, generate a random value
void get_v_from_addr_v(vector<uint8_t>& v, uint64_t addr_v,
                       vector<pair<uint64_t, uint8_t>>& mem_addr_val) {
  bool found = true;
  for (int i = 0; i < v.size(); i++) {
    bool found_i = false;
    uint64_t addr = addr_v + i;
    for (int j = 0; j < mem_addr_val.size(); j++) {
      if (addr == mem_addr_val[j].first) {
        v[i] = mem_addr_val[j].second;
        found_i = true;
        break;
      }
    }
    if (!found_i) {
      found = false;
      break;
    }
  }
  // if v does not in mem_addr_val, generate a random value
  if (found) return;
  for (int i = 0; i < v.size(); i++) {
    v[i] = unidist_codegen(gen_codegen) * (double)0xff; // uint8_t: 0 - 0xff
  }
}

void counterex_urt_2_input_map(inout_t& input, z3::model& mdl, smt_var& sv,
                               vector<pair< uint64_t, uint8_t>>& mem_addr_val) {
  smt_map_wt& map_urt = sv.mem_var._map_table._urt;
  smt_wt& mem_urt = sv.mem_var._mem_table._urt;
  for (int i = 0; i < map_urt.size(); i++) {
    z3::expr z3_is_valid = map_urt.is_valid[i];
    int is_valid = mdl.eval(z3_is_valid).bool_value();
    if (is_valid != 1) continue; // -1 means z3 false, 1 means z3 true, 0: z3 const (not know it is true or false)

    z3::expr z3_addr_v = mdl.eval(map_urt.addr_v[i]);
    if (! z3_addr_v.is_numeral()) continue;
    uint64_t addr_v = z3_addr_v.get_numeral_uint64();
    if (addr_v == 0) continue;

    z3::expr z3_addr_map = mdl.eval(map_urt.addr_map[i]);
    assert(z3_addr_map.is_numeral());
    int map_id = z3_addr_map.get_numeral_int();
    z3::expr z3_k = mdl.eval(map_urt.key[i]);
    if (! z3_k.is_numeral()) continue;
    string k = z3_bv_2_hex_str(z3_k);
    unsigned int val_sz = mem_t::map_val_sz(map_id) / NUM_BYTE_BITS;
    vector<uint8_t> v(val_sz);
    // get the corresponding "v" according to "addr_v"
    get_v_from_addr_v(v, addr_v, mem_addr_val);
    input.update_kv(map_id, k, v);
  }
}

// set input memory (pkt) according to counter-example urt table
// traverse mem_addr_val list, if the addr is in input memory address range, update "input"
void counterex_urt_2_input_mem(inout_t& input, z3::model& mdl, smt_var& sv,
                               vector<pair< uint64_t, uint8_t>>& mem_addr_val) {
  if (mem_t::_layout._pkt_sz == 0) return;

  // set pkt with random values
  input.set_pkt_random_val();
  z3::expr z3_pkt_start = mdl.eval(sv.get_pkt_start_addr());
  z3::expr z3_pkt_end = mdl.eval(sv.get_pkt_end_addr());
  uint64_t pkt_start = get_uint64_from_bv64(z3_pkt_start, true);
  uint64_t pkt_end = get_uint64_from_bv64(z3_pkt_end, true);

  for (int i = 0; i < mem_addr_val.size(); i++) {
    uint64_t addr = mem_addr_val[i].first;
    uint8_t val = mem_addr_val[i].second;
    // only update addr in pkt range
    if ((addr < pkt_start) || (addr > pkt_end)) continue;

    int idx = addr - pkt_start;
    input.pkt[idx] = val;
  }
}

void counterex_2_input_simu_r10(inout_t& input, z3::model& mdl, smt_var& sv) {
  z3::expr z3_stack_bottom = mdl.eval(sv.get_stack_bottom_addr());
  input.input_simu_r10 = get_uint64_from_bv64(z3_stack_bottom, true); // r10: stack bottom
}

void counterex_urt_2_input_mem_for_one_sv(inout_t& input, z3::model& mdl, smt_var& sv) {
  counterex_2_input_simu_r10(input, mdl, sv);
  vector<pair< uint64_t, uint8_t>> mem_addr_val;
  get_mem_from_mdl(mem_addr_val, mdl, sv);
  counterex_urt_2_input_map(input, mdl, sv, mem_addr_val);
  counterex_urt_2_input_mem(input, mdl, sv, mem_addr_val);
}

// make sure sv1 is for the original program
void counterex_2_input_mem(inout_t& input, z3::model& mdl,
                           smt_var& sv1, smt_var& sv2) {
  input.clear();
  // update input memory for executing path condition later
  counterex_urt_2_input_mem_for_one_sv(input, mdl, sv2);
  // update sv1[sv1_id] finally, the same update before will be overwritten
  counterex_urt_2_input_mem_for_one_sv(input, mdl, sv1);
}
*/