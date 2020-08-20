#include "inst_codegen.h"

using namespace std;

default_random_engine gen_codegen;
uniform_real_distribution<double> unidist_codegen(0.0, 1.0);

z3::expr latest_write_element(int idx, vector<z3::expr>& is_valid_list, vector<z3::expr>& x);
z3::expr addr_in_addrs(z3::expr& a, vector<z3::expr>& is_valid_list, vector<z3::expr>& x);
z3::expr addr_in_addrs_map_mem(z3::expr& a, vector<z3::expr>& is_valid_list, vector<z3::expr>& x);
z3::expr key_not_found_after_idx(z3::expr key, int idx, smt_map_wt& m_wt);
z3::expr key_not_in_map_wt(z3::expr k, smt_map_wt& m_wt, smt_var& sv, bool same_pgms = false, unsigned int block = 0);

uint64_t compute_helper_function(int func_id, uint64_t r1, uint64_t r2, uint64_t r3,
                                 uint64_t r4, uint64_t r5, mem_t& m, simu_real& sr, prog_state& ps) {
  switch (func_id) {
    case BPF_FUNC_map_lookup: ps.reg_safety_chk(0, vector<int> {1, 2}); return compute_map_lookup_helper(r1, r2, m, sr);
    case BPF_FUNC_map_update: ps.reg_safety_chk(0, vector<int> {1, 2, 3}); return compute_map_update_helper(r1, r2, r3, m, sr);
    case BPF_FUNC_map_delete: ps.reg_safety_chk(0, vector<int> {1, 2}); return compute_map_delete_helper(r1, r2, m, sr);
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

z3::expr predicate_ldmapid(z3::expr map_id, z3::expr out, smt_var& sv, unsigned int block) {
  z3::expr path_cond = sv.mem_var.get_block_path_cond(block);
  sv.add_expr_map_id(out, map_id, path_cond);
  return (map_id == out);
}

z3::expr predicate_ld32(z3::expr addr, z3::expr off, smt_var& sv, z3::expr out, unsigned int block) {
  if (mem_t::get_pgm_input_type() == PGM_INPUT_pkt_ptrs) {
    int pkt_ptrs_mem_table = sv.mem_var.get_mem_table_id(MEM_TABLE_pkt_ptrs);
    vector<int> ids;
    vector<mem_ptr_info> info_list;
    sv.mem_var.get_mem_ptr_info(ids, info_list, addr);
    for (int i = 0; i < ids.size(); i++) {
      if (ids[i] != pkt_ptrs_mem_table) continue;
      z3::expr addr_off = info_list[i].off + off;
      // the first 4 bytes are the address of pkt start, the last are the address of pkt end
      z3::expr pc_addr_off = (addr_off == ZERO_ADDR_OFF_EXPR) || (addr_off == to_expr((int64_t)4));
      z3::expr pc = info_list[i].path_cond && sv.mem_var.get_block_path_cond(block);
      int pkt_mem_table = sv.mem_var.get_mem_table_id(MEM_TABLE_pkt);
      sv.mem_var.add_ptr(out, pkt_mem_table, addr_off, pc && pc_addr_off);
    }
  }
  return ((out.extract(63, 32) == to_expr(0, 32)) &&
          predicate_ld_byte(addr, off, sv, out.extract(7, 0), block) &&
          predicate_ld_byte(addr, off + 1, sv, out.extract(15, 8), block) &&
          predicate_ld_byte(addr, off + 2, sv, out.extract(23, 16), block) &&
          predicate_ld_byte(addr, off + 3, sv, out.extract(31, 24), block));
}

z3::expr predicate_st_byte(z3::expr in, z3::expr addr, z3::expr off, smt_var& sv, unsigned int block, z3::expr cond) {
  z3::expr path_cond = sv.mem_var.get_block_path_cond(block) && cond;
  z3::expr f = Z3_true;
  vector<int> ids;
  vector<mem_ptr_info> info_list;
  sv.mem_var.get_mem_ptr_info(ids, info_list, addr);
  // cout << "enter predicate_st_byte" << endl;
  // cout << "addr: " << addr << endl;
  // cout << "pc: " << path_cond.simplify() << endl;
  // for (int i = 0; i < ids.size(); i++) cout << ids[i] << " " << info_list[i].off << " " << info_list[i].path_cond << endl;
  if (ids.size() == 0) {string s = "error!!!"; throw (s); return Z3_true;} // todo: addr is not a pointer
  for (int i = 0; i < ids.size(); i++) {
    z3::expr is_valid = sv.update_is_valid();
    z3::expr cond = path_cond && info_list[i].path_cond;
    f = f && (is_valid == cond);
    bool is_addr_off_table = (ids[i] == sv.mem_var.get_mem_table_id(MEM_TABLE_stack)) ||
                             (ids[i] == sv.mem_var.get_mem_table_id(MEM_TABLE_pkt)) ||
                             (ids[i] == sv.mem_var.get_mem_table_id(MEM_TABLE_pkt_ptrs));
    if (is_addr_off_table) { // addr in the entry is offset
      z3::expr addr_off = off + info_list[i].off;
      // cout << "is_addr_off_table  addr_off: " << addr_off.simplify() << endl;
      sv.mem_var.add_in_mem_table_wt(ids[i], block, is_valid, addr_off, in.extract(7, 0));
    } else {
      sv.mem_var.add_in_mem_table_wt(ids[i], block, is_valid, addr + off, in.extract(7, 0));
    }
  }
  return f;
}

// this func is only called by map helper functions
z3::expr predicate_st_n_bytes(int n, z3::expr in, z3::expr addr, smt_var& sv,
                              unsigned int block, z3::expr cond) {
  z3::expr f = string_to_expr("true");
  for (int i = 0; i < n; i++) {
    f = f && predicate_st_byte(in.extract(8 * i + 7, 8 * i), addr, to_expr(i, 64), sv, block, cond);
  }
  return f;
}

inline z3::expr addr_in_range(z3::expr addr, z3::expr start, z3::expr end) {
  return (uge(addr, start) && uge(end, addr));
}

z3::expr urt_element_constrain_map_mem(z3::expr a, z3::expr v, smt_wt& urt) {
  z3::expr f = string_to_expr("true");
  // add constrains on the new symbolic value "v" according to the following cases:
  // case 1: "a" can be found in wt(addr1), the case is processed in
  // function "predicate_ld_byte", that is urt.add(new_addr, out)

  // case 2: "a" cannot be found in wt, but urt(addr1).
  // if there is no address equal to a in wt and addr1 in urt is equal to
  // a, it implies v is equal to the value of addr1
  for (int i = urt.addr.size() - 1; i >= 0; i--) {
    f = f && z3::implies((a != NULL_ADDR_EXPR) &&
                         (a == urt.addr[i]) &&
                         urt.is_valid[i],
                         v == urt.val[i]);
  }
  // case 3: "a" cannot be found in wt or urt.
  // there is no constrains on the new symbolic value "v"
  return f;
}

z3::expr urt_element_constrain(z3::expr a, z3::expr v, smt_wt& urt) {
  z3::expr f = Z3_true;
  for (int i = 0; i < urt.addr.size(); i++) {
    f = f && z3::implies((a == urt.addr[i]) &&
                         urt.is_valid[i],
                         v == urt.val[i]);
  }
  return f;
}

z3::expr predicate_ld_byte_for_one_mem_table(int table_id, mem_ptr_info& ptr_info,
    z3::expr addr, z3::expr off, smt_var& sv, z3::expr out, unsigned int block, z3::expr cond = Z3_true) {
  cond = ptr_info.path_cond && sv.mem_var.get_block_path_cond(block) && cond;
  smt_wt& wt = sv.mem_var._mem_tables[table_id]._wt;
  z3::expr a = addr + off;
  bool is_addr_off_table = (table_id == sv.mem_var.get_mem_table_id(MEM_TABLE_stack)) ||
                           (table_id == sv.mem_var.get_mem_table_id(MEM_TABLE_pkt)) ||
                           (table_id == sv.mem_var.get_mem_table_id(MEM_TABLE_pkt_ptrs));
  if (is_addr_off_table) {
    a = ptr_info.off + off;
  }
  z3::expr f_found_in_wt_after_i = string_to_expr("false");
  z3::expr f = string_to_expr("true");
  // cout << "wt.addr.size(): " << wt.addr.size() << endl;
  for (int i = wt.addr.size() - 1; i >= 0; i--) {
    int is_pc_match = sv.pgm_dag.is_b_on_root2a_path(block, wt.block[i]);
    // cout << "is_pc_match: " << is_pc_match << endl;
    if (is_pc_match == INT_false) continue;
    z3::expr f_same = (a != NULL_ADDR_EXPR) && (a == wt.addr[i]) && wt.is_valid[i];
    if (is_addr_off_table) f_same = (a == wt.addr[i]) && wt.is_valid[i];

    f = f && z3::implies((!f_found_in_wt_after_i) && f_same,
                         out == wt.val[i]);
    f_found_in_wt_after_i = f_found_in_wt_after_i || f_same;
  }
  // stack does not have urt
  if (table_id == sv.mem_var.get_mem_table_id(MEM_TABLE_stack)) {
    return f;
  }
  // add constrains on the element(a, out)
  smt_wt& urt = sv.mem_var._mem_tables[table_id]._urt;
  z3::expr not_found_in_wt = sv.update_is_valid();
  f = f && (not_found_in_wt == (!f_found_in_wt_after_i));
  if (is_addr_off_table) f = f && z3::implies(not_found_in_wt, urt_element_constrain(a, out, urt));
  else f = f && z3::implies(not_found_in_wt, urt_element_constrain_map_mem(a, out, urt));
  // add element in urt
  // An example that will cause a problem in equvialence check if just add (a, out) in urt
  // and add an entry in map URT using the same approach
  // example: pgm1: update k v m, lookup k m; pgm2: lookup k m
  // expected: pgm1 != pgm2, but if add (a, out) in urt, pgm1 == pgm2
  // Reason why pgm1 == pgm2: for the lookup in pgm1, because of the update, "k" can be found
  // in map WT, then the constrain on "addr_v" is that addr_v == addr_v' found in map WT,
  // and mem[addr_v'] == v because of the update. Thus, for pgm1, the update v is the same
  // as uninitialized lookup v.
  z3::expr is_valid = sv.update_is_valid();
  f = f && (is_valid == (not_found_in_wt && cond));
  urt.add(block, is_valid, a, out);
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

z3::expr predicate_ld_byte(z3::expr addr, z3::expr off, smt_var& sv, z3::expr out, unsigned int block, z3::expr cond) {
  // cout << "predicate_ld_byte: " << endl;
  // cout << "addr: " << addr << endl;
  z3::expr f = Z3_true;
  vector<int> ids;
  vector<mem_ptr_info> info_list;
  sv.mem_var.get_mem_ptr_info(ids, info_list, addr);
  // cout << "enter predicate_ld_byte" << endl;
  // cout << "addr: " << addr << endl;
  // for (int i = 0; i < ids.size(); i++) cout << ids[i] << " " << info_list[i].off << " " << info_list[i].path_cond << endl;
  // cout << "ids.size(): " << ids.size() << endl;
  if (ids.size() == 0) {string s = "error!!!"; throw (s); return Z3_true;} // todo: addr is not a pointer
  for (int i = 0; i < ids.size(); i++) {
    f = f && predicate_ld_byte_for_one_mem_table(ids[i], info_list[i], addr, off, sv, out, block, cond);
  }
  return f;
}

z3::expr predicate_ld_n_bytes(int n, z3::expr addr, smt_var& sv, z3::expr out, unsigned int block, z3::expr cond) {
  z3::expr f = predicate_ld_byte(addr, to_expr(0, 64), sv, out.extract(7, 0), block, cond);
  for (int i = 1; i < n; i++) {
    f = f && predicate_ld_byte(addr, to_expr(i, 64), sv, out.extract(8 * i + 7, 8 * i), block, cond);
  }
  return f;
}

z3::expr predicate_xadd64(z3::expr in, z3::expr addr, z3::expr off, smt_var& sv, unsigned int block) {
  z3::expr v64_1 = sv.new_var(64);
  z3::expr f = predicate_ld64(addr, off, sv, v64_1, block);
  z3::expr v64_2 = sv.new_var(64);
  f = f && (v64_2 == (v64_1 + in));
  f = f && predicate_st64(v64_2, addr, off, sv, block);
  return f;
}

z3::expr predicate_xadd32(z3::expr in, z3::expr addr, z3::expr off, smt_var& sv, unsigned int block) {
  z3::expr v64_1 = sv.new_var(64);
  z3::expr f = predicate_ld32(addr, off, sv, v64_1, block);
  z3::expr v64_2 = sv.new_var(64);
  f = f && (v64_2 == (v64_1 + in));
  f = f && predicate_st32(v64_2, addr, off, sv, block);
  return f;
}

// return the FOL formula that x[idx] is the latest write in x
// that is, for any i > idx, x[idx] != x[i]
z3::expr latest_write_element(int idx, vector<z3::expr>& is_valid_list, vector<z3::expr>& x) {
  z3::expr f = string_to_expr("true");
  for (int i = x.size() - 1; i > idx; i--) {
    f = f && ((x[idx] != x[i]) || (! is_valid_list[i]));
  }
  return f;
}

// return the FOL formula that a (a!=null) can be found in x
// that is, for each x[i], a != x[i]
z3::expr addr_in_addrs_map_mem(z3::expr& a, vector<z3::expr>& is_valid_list, vector<z3::expr>& x) {
  z3::expr f = string_to_expr("false");
  for (int i = 0; i < x.size(); i++) {
    f = f || ((a != NULL_ADDR_EXPR) && (a == x[i]) && is_valid_list[i]);
  }
  return f;
}

z3::expr addr_in_addrs(z3::expr& a, vector<z3::expr>& is_valid_list, vector<z3::expr>& x) {
  z3::expr f = string_to_expr("false");
  for (int i = 0; i < x.size(); i++) {
    f = f || ((a == x[i]) && is_valid_list[i]);
  }
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
    z3::expr iv_out = wt1.is_valid[i];
    z3::expr a_out = wt1.addr[i];
    z3::expr v_out = wt1.val[i];
    z3::expr f_a_out = iv_out && latest_write_element(i, wt1.is_valid, wt1.addr);
    z3::expr f_a_not_in_wt2 = !addr_in_addrs(a_out, wt2.is_valid, wt2.addr);
    z3::expr f_a_in_urt1 = addr_in_addrs(a_out, urt1.is_valid, urt1.addr);
    f = f && z3::implies(f_a_out && f_a_not_in_wt2, f_a_in_urt1);

    for (int j = 0; j < urt1.size(); j++) {
      z3::expr iv_in = urt1.is_valid[j];
      z3::expr a_in = urt1.addr[j];
      z3::expr v_in = urt1.val[j];
      f = f && z3::implies(iv_in && f_a_out &&
                           f_a_not_in_wt2 && (a_out == a_in),
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
  // cout << "smt_pkt_eq_chk: " << "id1:" << id1 << "id2:" << id2 << endl;
  assert(id1 != -1);
  assert(id2 != -1);
  z3::expr f = Z3_true;
  smt_wt& wt1 = sv1.mem_var._mem_tables[id1]._wt;
  smt_wt& wt2 = sv2.mem_var._mem_tables[id2]._wt;
  // case 1: pkt address in both wts, latest write should be the same
  if ((wt1.size() > 0) && (wt2.size() > 0)) {
    for (int i = wt1.size() - 1; i >= 0; i--) {
      z3::expr iv1 = wt1.is_valid[i];
      z3::expr a1 = wt1.addr[i];
      z3::expr v1 = wt1.val[i];
      z3::expr f_a1 = latest_write_element(i, wt1.is_valid, wt1.addr);

      for (int j = wt2.size() - 1; j >= 0; j--) {
        z3::expr iv2 = wt2.is_valid[j];
        z3::expr a2 = wt2.addr[j];
        z3::expr v2 = wt2.val[j];
        z3::expr f_a2 = latest_write_element(j, wt2.is_valid, wt2.addr);
        f = f && z3::implies(iv1 && iv2 && f_a1 && f_a2 && (a1 == a2), v1 == v2);
      }
    }
  }
  // case 2: pkt address in one of wts
  // f = f && pkt_addr_in_one_wt(sv1, sv2) && pkt_addr_in_one_wt(sv2, sv1);
  z3::expr f1 = pkt_addr_in_one_wt(sv1, sv2);
  z3::expr f2 = pkt_addr_in_one_wt(sv2, sv1);
  z3::expr pkt_eq = sv1.update_is_valid();
  z3::expr pkt_eq1 = sv1.update_is_valid();
  z3::expr pkt_eq2 = sv1.update_is_valid();
  // cout << "smt_pkt_eq_chk, pkt_eq:" << pkt_eq << " " << pkt_eq1 << " " << pkt_eq2 << endl;
  return z3::implies((pkt_eq == f) && (pkt_eq1 == f1) && (pkt_eq2 == f2), pkt_eq && pkt_eq1 && pkt_eq2);
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

z3::expr key_not_in_map_wt(z3::expr k, smt_map_wt& m_wt, smt_var& sv, bool same_pgms, unsigned int block) {
  z3::expr f = string_to_expr("false");
  for (int i = 0; i < m_wt.key.size(); i++) {
    if (same_pgms) {
      int is_pc_match = sv.pgm_dag.is_b_on_root2a_path(block, m_wt.block[i]);
      // cout << "is_pc_match: " << is_pc_match << endl;
      if (is_pc_match == INT_false) continue;
    }

    f = f || ((m_wt.is_valid[i] == Z3_true) && (k == m_wt.key[i]));
  }
  return (!f);
}

z3::expr ld_byte_from_wt(z3::expr addr, smt_wt& wt, z3::expr out) {
  z3::expr a = addr;
  z3::expr f = string_to_expr("true");
  z3::expr f_found_after_i = string_to_expr("false");
  for (int i = wt.addr.size() - 1; i >= 0; i--) {
    z3::expr f_found_i = wt.is_valid[i] && (a == wt.addr[i]) && (a != NULL_ADDR_EXPR);
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

void get_k_addr_v_and_constraints_list(vector<z3::expr>& k_list, vector<z3::expr>& f_k_list,
                                       vector<z3::expr>& addr_list, int map_id, smt_var& sv) {
  k_list.clear();
  addr_list.clear();
  f_k_list.clear();
  map_wt& map = sv.mem_var._map_tables[map_id];
  for (int i = 0; i < map._wt.size(); i++) {
    k_list.push_back(map._wt.key[i]);
    addr_list.push_back(map._wt.addr_v[i]);
  }
  for (int i = 0; i < map._urt.size(); i++) {
    k_list.push_back(map._urt.key[i]);
    addr_list.push_back(map._urt.addr_v[i]);
  }

  // generate constraints for each key in map wt
  // constraints: 1. key is the latest key in wt; 2. key entry is valid
  for (int i = 0; i < map._wt.size(); i++) {
    z3::expr f_k = key_not_found_after_idx(map._wt.key[i], i, map._wt) && map._wt.is_valid[i];
    f_k_list.push_back(f_k);
  }
  // generate constraints for each key in map urt
  // constraints: 1. key is not in the map wt; 2. key entry is valid
  for (int i = 0; i < map._urt.size(); i++) {
    z3::expr f_k = key_not_in_map_wt(map._urt.key[i], map._wt, sv) && map._urt.is_valid[i];
    f_k_list.push_back(f_k);
  }
}

// for map equivalence check
z3::expr ld_byte_from_map_mem_table(z3::expr addr, mem_table& mem_tbl, z3::expr out) {
  z3::expr f_wt = ld_byte_from_wt(addr, mem_tbl._wt, out);
  z3::expr f_not_in_wt = ! addr_in_addrs_map_mem(addr, mem_tbl._wt.is_valid, mem_tbl._wt.addr);
  z3::expr f_urt = ld_byte_from_urt(addr, mem_tbl._urt, out);
  return f_wt && z3::implies(f_not_in_wt, f_urt);
}

z3::expr ld_n_bytes_from_map_mem_table(int n, z3::expr addr, mem_table& mem_tbl, z3::expr out) {
  z3::expr f = Z3_true;
  for (int i = 0; i < n; i++) {
    f = f && ld_byte_from_map_mem_table(addr + i, mem_tbl, out.extract(8 * i + 7, 8 * i));
  }
  return f;
}

void get_v_out_and_constraints_list(vector<z3::expr>& v_out_list, vector<z3::expr>& f_v_out_list,
                                    int map_id, smt_var& sv) {
  v_out_list.clear();
  f_v_out_list.clear();
  int v_sz = mem_t::map_val_sz(map_id);
  map_wt& map = sv.mem_var._map_tables[map_id];
  int mem_id = sv.mem_var.get_mem_table_id(MEM_TABLE_map, map_id);
  mem_table& mem = sv.mem_var._mem_tables[mem_id];
  // load final output map value from both map mem wt and urt table.
  for (int i = 0; i < map._wt.size(); i++) {
    z3::expr addr = map._wt.addr_v[i];
    z3::expr v = sv.update_val(v_sz);
    v_out_list.push_back(v);
    z3::expr f_v_out = ld_n_bytes_from_map_mem_table(v_sz / NUM_BYTE_BITS, addr, mem, v);
    f_v_out_list.push_back(f_v_out);
  }
  for (int i = 0; i < map._urt.size(); i++) {
    z3::expr addr = map._urt.addr_v[i];
    z3::expr v = sv.update_val(v_sz);
    v_out_list.push_back(v);
    z3::expr f_v_out = ld_n_bytes_from_map_mem_table(v_sz / NUM_BYTE_BITS, addr, mem, v);
    f_v_out_list.push_back(f_v_out);
  }
}

void get_v_in_and_constraints_list(vector<z3::expr>& v_in_list, vector<z3::expr>& f_v_in_list,
                                   int map_id, smt_var& sv) {
  v_in_list.clear();
  f_v_in_list.clear();
  map_wt& map = sv.mem_var._map_tables[map_id];
  int v_sz = mem_t::map_val_sz(map_id);
  int table_id = sv.mem_var.get_mem_table_id(MEM_TABLE_map, map_id);
  smt_wt& mem_urt = sv.mem_var._mem_tables[table_id]._urt;
  for (int i = 0; i < map._urt.size(); i++) {
    z3::expr v_in = sv.update_val(v_sz);
    v_in_list.push_back(v_in);
    z3::expr f_v_in = ld_n_bytes_from_urt(v_sz / NUM_BYTE_BITS, map._urt.addr_v[i], mem_urt, v_in);
    f_v_in_list.push_back(f_v_in);
  }
}

// key in map1, not map2; the corresponding v_in == v_out
z3::expr smt_one_map_eq_chk_k_in_one_map(vector<z3::expr>& k1_list, vector<z3::expr>& f_k1_list,
    vector<z3::expr>& addr_v1_list, vector<z3::expr>& v1_list, vector<z3::expr>& f_v1_list,
    int map_id, smt_var& sv1, smt_var& sv2) {
  z3::expr f = Z3_true;
  vector<z3::expr> v1_in_list, f_v1_in_list;
  // get v_in and v's constraints from map urt and mem urt
  get_v_in_and_constraints_list(v1_in_list, f_v1_in_list, map_id, sv1);
  map_wt& map1 = sv1.mem_var._map_tables[map_id];
  map_wt& map2 = sv2.mem_var._map_tables[map_id];
  for (int i = 0; i < k1_list.size(); i++) {
    z3::expr addr_v1 = addr_v1_list[i];
    z3::expr f_k1_not_in_map2 = key_not_in_map_wt(k1_list[i], map2._wt, sv2) && // k1 not in map2 wt
                                key_not_in_map_wt(k1_list[i], map2._urt, sv2);   // k1 not in map2 urt
    z3::expr f_k1_in_map1_urt = !key_not_in_map_wt(k1_list[i], map1._urt, sv1);
    f = f && z3::implies(f_k1_not_in_map2 && f_k1_list[i], f_k1_in_map1_urt);

    for (int j = 0; j < map1._urt.size(); j++) {
      z3::expr k1_in = map1._urt.key[j];
      z3::expr addr_v1_in = map1._urt.addr_v[j];
      z3::expr f_found_same_key = (k1_list[i] == k1_in) && f_k1_list[i] && f_k1_not_in_map2;
      z3::expr f_k_both_inexist = (addr_v1 == NULL_ADDR_EXPR) && (addr_v1_in == NULL_ADDR_EXPR);
      z3::expr v_eq = sv1.update_is_valid();
      // cout << "one_map: " << v_eq << " " << v1_list[i] << " " << v1_in_list[j] << endl;
      z3::expr f_k_both_exist = z3::implies(f_v1_list[i] && f_v1_in_list[j] &&
                                            (v_eq == (v1_list[i] == v1_in_list[j])),
                                            (addr_v1 != NULL_ADDR_EXPR) &&
                                            (addr_v1_in != NULL_ADDR_EXPR) && v_eq);
      f = f && z3::implies(f_found_same_key, f_k_both_inexist || f_k_both_exist);
    }
  }
  return f;
}

z3::expr smt_one_map_eq_chk_k_in_both_map(vector<z3::expr>& k1_list, vector<z3::expr>& k2_list,
    vector<z3::expr>& f_k1_list, vector<z3::expr>& f_k2_list,
    vector<z3::expr>& addr_v1_list, vector<z3::expr>& addr_v2_list,
    vector<z3::expr>& v1_list, vector<z3::expr>& v2_list,
    vector<z3::expr>& f_v1_list, vector<z3::expr>& f_v2_list,
    smt_var& sv) {
  z3::expr f = Z3_true;
  for (int i = 0; i < k1_list.size(); i++) {
    z3::expr addr_v1 = addr_v1_list[i];
    for (int j = 0; j < k2_list.size(); j++) {
      z3::expr addr_v2 = addr_v2_list[j];
      z3::expr f_found_same_key = (k1_list[i] == k2_list[j]) && f_k1_list[i] && f_k2_list[j];
      z3::expr f_k_both_inexist = (addr_v1 == NULL_ADDR_EXPR) && (addr_v2 == NULL_ADDR_EXPR);
      z3::expr v_eq = sv.update_is_valid();
      // cout << "both_map: " << v_eq << " " << v1_list[i] << " " << v2_list[j] << endl;
      z3::expr f_k_both_exist = z3::implies(f_v1_list[i] && f_v2_list[j] &&
                                            (v_eq == (v1_list[i] == v2_list[j])),
                                            (addr_v1 != NULL_ADDR_EXPR) &&
                                            (addr_v2 != NULL_ADDR_EXPR) && v_eq);
      f = f && z3::implies(f_found_same_key, f_k_both_inexist || f_k_both_exist);
    }
  }
  return f;
}

z3::expr smt_one_map_eq_chk(int map_id, smt_var& sv1, smt_var& sv2) {
  // cout << "smt_one_map_eq_chk: " << map_id << endl;
  // generate and store all keys in map1/map2 wt/urt and keys' corresponding addr_v and constraints
  vector<z3::expr> k1_list, k2_list, addr_v1_list, addr_v2_list, f_k1_list, f_k2_list;
  get_k_addr_v_and_constraints_list(k1_list, f_k1_list, addr_v1_list, map_id, sv1);
  get_k_addr_v_and_constraints_list(k2_list, f_k2_list, addr_v2_list, map_id, sv2);
  // all values of keys in map1/map2 wt/urt
  vector<z3::expr> v1_out_list, v2_out_list, f_v1_out_list, f_v2_out_list;
  get_v_out_and_constraints_list(v1_out_list, f_v1_out_list, map_id, sv1);
  get_v_out_and_constraints_list(v2_out_list, f_v2_out_list, map_id, sv2);
  z3::expr f = Z3_true;
  // cout << "list1 print" << endl;
  // for (int i = 0; i < k1_list.size(); i++) {
  //   cout << k1_list[i] << " " << addr_v1_list[i] << " " << f_k1_list[i] << endl;
  // }
  // cout << "list2 print" << endl;
  // for (int i = 0; i < k2_list.size(); i++) {
  //   cout << k2_list[i] << " " << addr_v2_list[i] << " " << f_k2_list[i] << endl;
  // }
  z3::expr map_eq1 = sv1.update_is_valid();
  z3::expr map_eq2 = sv1.update_is_valid();
  z3::expr map_eq3 = sv1.update_is_valid();
  // cout << "smt_one_map_eq_chk: " << map_eq1 << " " << map_eq2 << " " << map_eq3 << endl;
  z3::expr f1 = smt_one_map_eq_chk_k_in_both_map(k1_list, k2_list, f_k1_list, f_k2_list,
                addr_v1_list, addr_v2_list, v1_out_list, v2_out_list, f_v1_out_list, f_v2_out_list, sv1);
  z3::expr f2 = smt_one_map_eq_chk_k_in_one_map(k1_list, f_k1_list, addr_v1_list,
                v1_out_list, f_v1_out_list, map_id, sv1, sv2); // k in map1 not in map2
  z3::expr f3 = smt_one_map_eq_chk_k_in_one_map(k2_list, f_k2_list, addr_v2_list,
                v2_out_list, f_v2_out_list, map_id, sv2, sv1); // k in map2 not in map1
  f = z3::implies(map_eq1 == f1, map_eq1) &&
      z3::implies(map_eq2 == f2, map_eq2) &&
      z3::implies(map_eq3 == f3, map_eq3);
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
    z3::expr iv1 = mem1_urt.is_valid[i];
    z3::expr a1 = mem1_urt.addr[i];
    z3::expr v1 = mem1_urt.val[i];

    for (int j = 0; j < mem2_urt.size(); j++) {
      z3::expr iv2 = mem2_urt.is_valid[j];
      z3::expr a2 = mem2_urt.addr[j];
      z3::expr v2 = mem2_urt.val[j];

      f = f && z3::implies(iv1 && iv2 && (a1 == a2), v1 == v2);
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
z3::expr predicate_map_lookup_k_in_map_wt(z3::expr k, z3::expr addr_map_v, smt_map_wt& m_wt,
    smt_var& sv, unsigned int block) {
  z3::expr f = string_to_expr("true");
  z3::expr key_found_after_i = string_to_expr("false");
  for (int i = m_wt.key.size() - 1; i >= 0; i--) {
    int is_pc_match = sv.pgm_dag.is_b_on_root2a_path(block, m_wt.block[i]);
    // cout << "is_pc_match: " << is_pc_match << endl;
    if (is_pc_match == INT_false) continue;
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
z3::expr predicate_map_lookup_k_in_map_urt(z3::expr k, z3::expr addr_map_v, map_wt& map_table,
    smt_var& sv, unsigned int block) {
  z3::expr f = string_to_expr("true");
  z3::expr f1 = key_not_in_map_wt(k, map_table._wt, sv, true, block);
  for (int i = map_table._urt.key.size() - 1; i >= 0; i--) {
    int is_pc_match = sv.pgm_dag.is_b_on_root2a_path(block, map_table._urt.block[i]);
    // cout << "is_pc_match: " << is_pc_match << endl;
    if (is_pc_match == INT_false) continue;

    z3::expr f2 = (map_table._urt.is_valid[i] == Z3_true) &&
                  (k == map_table._urt.key[i]);
    f = f && z3::implies(f1 && f2, addr_map_v == map_table._urt.addr_v[i]);
  }
  return f;
}

// this func is only called by map lookup helper functions
z3::expr predicate_st_byte_in_mem_urt(int table_id, z3::expr addr, smt_var& sv,
                                      unsigned int block, z3::expr is_valid) {
  smt_wt& urt = sv.mem_var._mem_tables[table_id]._urt;
  z3::expr v = sv.update_val();
  z3::expr f = urt_element_constrain_map_mem(addr, v, urt);
  urt.add(block, is_valid, addr, v);
  return f;
}

// this func is only called by map lookup helper functions
z3::expr predicate_st_n_bytes_in_mem_urt(int n, int table_id, z3::expr addr, smt_var& sv,
    unsigned int block, z3::expr cond) {
  z3::expr f = string_to_expr("true");
  z3::expr is_valid = sv.update_is_valid();
  f = f && (is_valid == cond);
  for (int i = 0; i < n; i++) {
    f = f && predicate_st_byte_in_mem_urt(table_id, addr + i, sv, block, is_valid);
  }
  return f;
}

// "addr_map_v" is the return value
z3::expr predicate_map_lookup_helper(z3::expr addr_map, z3::expr addr_k, z3::expr addr_map_v,
                                     smt_var& sv, unsigned int block) {
  z3::expr f_ret = string_to_expr("true");
  smt_mem& mem = sv.mem_var;
  // get map id according to addr_map
  vector<int> map_ids;
  vector<z3::expr> map_id_path_conds;
  // sv.get_map_id(map_ids, map_id_path_conds);
  sv.get_map_id(map_ids, map_id_path_conds, addr_map);
  // cout << "enter predicate_map_lookup_helper" << endl;
  // cout << "addr_map: " << addr_map << endl;
  // for (int i = 0; i < map_ids.size(); i++) cout << map_ids[i] << " " << map_id_path_conds[i] << endl;
  if (map_ids.size() == 0) {string s = "error!!!"; throw (s); return f_ret;} // todo: addr_map is not a pointer
  for (int i = 0; i < map_ids.size(); i++) {
    // cout << map_ids[i] << " " << map_id_path_conds[i] << endl;
    int map_id = map_ids[i];
    int k_sz = mem_t::map_key_sz(map_id);
    z3::expr k = sv.update_key(k_sz);

    /* add constrains on k */
    z3::expr f = predicate_ld_n_bytes(k_sz / NUM_BYTE_BITS, addr_k, sv, k, block, map_id_path_conds[i]);

    /* add constrains on addr_map_v for the following cases
       if key is in the target map, addr_map_v is the same as the corresponding
       value address in the map; else addr_map_v is either NULL or a new address
       to simulate the uncertainty of input map.
    */
    // case 1: check k in the map WT
    f = f && predicate_map_lookup_k_in_map_wt(k, addr_map_v, mem._map_tables[map_id]._wt, sv, block);
    // case 2: k not in the map WT, check k in the map URT
    f = f && predicate_map_lookup_k_in_map_urt(k, addr_map_v, mem._map_tables[map_id], sv, block);
    // case 3: k is neither in the map WT nor the map URT
    z3::expr f1 = key_not_in_map_wt(k, mem._map_tables[map_id]._wt, sv, true, block);
    z3::expr f2 = key_not_in_map_wt(k, mem._map_tables[map_id]._urt, sv, true, block);
    f = f && z3::implies(f1 && f2,
                         (addr_map_v == NULL_ADDR_EXPR) ||
                         (addr_map_v == mem.get_and_update_addr_v_next(map_id)));

    z3::expr is_valid = sv.update_is_valid(); // z3 boolean const
    /* add the constrains on "is_valid" */
    // if k is in map WT, set is_valid is false to
    // indicate this entry to be added in the map URT is invalid.
    z3::expr cond = sv.mem_var.get_block_path_cond(block) && map_id_path_conds[i];
    f_ret = f_ret && z3::implies((!cond) || (!f1), is_valid == Z3_false);
    f_ret = f_ret && z3::implies(cond && f1, is_valid == Z3_true);
    f_ret = f_ret && z3::implies(map_id_path_conds[i], f);
    // cout << "add ptr in predicate_map_lookup_helper: " << addr_map_v << endl;
    mem.add_ptr_by_map_id(addr_map_v, map_id, cond); // todo: what if addr_map_v == NULL
    mem._map_tables[map_id]._urt.add(block, is_valid, k, addr_map_v);
    int table_id = sv.mem_var.get_mem_table_id(MEM_TABLE_map, map_id);
    int v_sz = mem_t::map_val_sz(map_id);
    predicate_st_n_bytes_in_mem_urt(v_sz / NUM_BYTE_BITS, table_id, addr_map_v, sv, block, map_id_path_conds[i]);
  }
  return f_ret;
}

// "out" is the return value
z3::expr predicate_map_update_helper_for_one_map(int map_id, z3::expr map_id_path_cond,
    z3::expr addr_k, z3::expr addr_v, z3::expr out, smt_var& sv, unsigned int block) {
  // cout << "predicate_map_update_helper_for_one_map: " << map_id << " " << map_id_path_cond << endl;
  z3::expr f_ret = string_to_expr("true");
  smt_mem& mem = sv.mem_var;
  int k_sz = mem_t::map_key_sz(map_id);
  int v_sz = mem_t::map_val_sz(map_id);
  z3::expr k = sv.update_key(k_sz);
  z3::expr v = sv.update_val(v_sz);
  z3::expr addr_map_v = sv.update_addr_v();
  /* add constrains on "out", "k", "v" */
  z3::expr f = (out == MAP_UPD_RET_EXPR) &&
               predicate_ld_n_bytes(k_sz / NUM_BYTE_BITS, addr_k, sv, k, block, map_id_path_cond) &&
               predicate_ld_n_bytes(v_sz / NUM_BYTE_BITS, addr_v, sv, v, block, map_id_path_cond);
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
    int is_pc_match = sv.pgm_dag.is_b_on_root2a_path(block, m_wt.block[i]);
    // cout << "is_pc_match: " << is_pc_match << endl;
    if (is_pc_match == INT_false) continue;

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
    int is_pc_match = sv.pgm_dag.is_b_on_root2a_path(block, m_urt.block[i]);
    // cout << "is_pc_match: " << is_pc_match << endl;
    if (is_pc_match == INT_false) continue;

    z3::expr key_found_i = (m_urt.is_valid[i] == Z3_true) && // valid entry
                           (k == m_urt.key[i]);// && // the same key
    z3::expr f1 = z3::implies(m_urt.addr_v[i] != NULL_ADDR_EXPR, addr_map_v == m_urt.addr_v[i]);
    f1 = f1 && z3::implies(m_urt.addr_v[i] == NULL_ADDR_EXPR, addr_map_v == next_addr_map_v);
    f = f && z3::implies(f_not_found_in_wt && key_found_i, f1);

    f_found_in_urt = f_found_in_urt || key_found_i;
  }
  // case 3: if the key is not in the target map
  f = f && z3::implies(f_not_found_in_wt && (!f_found_in_urt), addr_map_v == next_addr_map_v);
  /* add the constrains on "is_valid" */
  z3::expr is_valid = sv.update_is_valid();
  z3::expr cond = sv.mem_var.get_block_path_cond(block) && map_id_path_cond;
  f_ret = f_ret && z3::implies(!cond, is_valid == Z3_false);
  f_ret = f_ret && z3::implies(cond, is_valid == Z3_true);
  f_ret = f_ret && z3::implies(map_id_path_cond, f);
  // add the update entry in map WT
  mem._map_tables[map_id]._wt.add(block, is_valid, k, addr_map_v);
  // add the update entry in memory WT
  mem.add_ptr_by_map_id(addr_map_v, map_id, cond); // todo: what if addr_map_v == NULL
  f_ret = f_ret && predicate_st_n_bytes(v_sz / NUM_BYTE_BITS, v, addr_map_v, sv, block, map_id_path_cond);

  return f_ret;
}

// "out" is the return value
z3::expr predicate_map_update_helper(z3::expr addr_map, z3::expr addr_k, z3::expr addr_v,
                                     z3::expr out, smt_var& sv, unsigned int block) {
  z3::expr f_ret = Z3_true;
  // get map id according to addr_map
  vector<int> map_ids;
  vector<z3::expr> map_id_path_conds;
  // sv.get_map_id(map_ids, map_id_path_conds);
  sv.get_map_id(map_ids, map_id_path_conds, addr_map);
  // cout << "enter predicate_map_update_helper" << endl;
  // cout << "addr_map: " << addr_map << endl;
  // for (int i = 0; i < map_ids.size(); i++) cout << map_ids[i] << " " << map_id_path_conds[i] << endl;
  if (map_ids.size() == 0) {string s = "error!!!"; throw (s); return f_ret;} // todo: addr_map is not a pointer
  for (int i = 0; i < map_ids.size(); i++) {
    f_ret = f_ret && predicate_map_update_helper_for_one_map(map_ids[i], map_id_path_conds[i],
            addr_k, addr_v, out, sv, block);
  }
  return f_ret;
}

// "out" is the return value
// if key not in the map, out = 0xfffffffe, else out = 0
z3::expr predicate_map_delete_helper_for_one_map(int map_id, z3::expr map_id_path_cond,
    z3::expr addr_k, z3::expr out, smt_var& sv, unsigned int block) {
  z3::expr f_ret = Z3_true;
  smt_mem& mem = sv.mem_var;
  int k_sz = mem_t::map_key_sz(map_id);
  z3::expr k = sv.update_key(k_sz);
  /* add the constrains on "k" */
  z3::expr f = predicate_ld_n_bytes(k_sz / NUM_BYTE_BITS, addr_k, sv, k, block, map_id_path_cond);

  /* add the constrains on "addr_map_v" according to "k" */
  z3::expr addr_map_v = sv.update_addr_v();
  // if k is in the map WT, set constrains on addr_map_v
  f = f && predicate_map_lookup_k_in_map_wt(k, addr_map_v, mem._map_tables[map_id]._wt, sv, block);
  // if k is not in the map WT but in map URT, set constrains on addr_map_v
  f = f && predicate_map_lookup_k_in_map_urt(k, addr_map_v, mem._map_tables[map_id], sv, block);

  z3::expr f1 = key_not_in_map_wt(k, mem._map_tables[map_id]._wt, sv, true, block);
  z3::expr f2 = key_not_in_map_wt(k, mem._map_tables[map_id]._urt, sv, true, block);
  z3::expr f3 = f1 && f2;
  // if k is neither in the map WT nor the map URT
  f = f && z3::implies(f3, (addr_map_v == NULL_ADDR_EXPR) ||
                       (addr_map_v == mem.get_and_update_addr_v_next(map_id)));

  /* add the constrains on "out" according to "addr_map_v" */
  f = f && z3::implies(addr_map_v == NULL_ADDR_EXPR,
                       out == MAP_DEL_RET_IF_KEY_INEXIST_EXPR);
  f = f && z3::implies(addr_map_v != NULL_ADDR_EXPR,
                       out == MAP_DEL_RET_IF_KEY_EXIST_EXPR);
  f_ret = f_ret && z3::implies(map_id_path_cond, f);

  z3::expr is_valid = sv.update_is_valid();
  z3::expr cond = sv.mem_var.get_block_path_cond(block) && map_id_path_cond;
  // add an entry in map WT to delete this key
  f_ret = f_ret && z3::implies(!cond, is_valid == Z3_false);
  f_ret = f_ret && z3::implies(cond, is_valid == Z3_true);
  mem._map_tables[map_id]._wt.add(block, is_valid, k, NULL_ADDR_EXPR);
  // add an entry in map URT to show lookup
  // only it is the target map, and k cannot be found in map WT is valid
  is_valid = sv.update_is_valid();
  f_ret = f_ret && z3::implies(!(cond && f1), is_valid == Z3_false);
  f_ret = f_ret && z3::implies(cond && f1, is_valid == Z3_true);

  mem.add_ptr_by_map_id(addr_map_v, map_id, cond); // todo: what if addr_map_v == NULL
  mem._map_tables[map_id]._urt.add(block, is_valid, k, addr_map_v);

  return f_ret;
}

z3::expr predicate_map_delete_helper(z3::expr addr_map, z3::expr addr_k, z3::expr out,
                                     smt_var& sv, unsigned int block) {
  z3::expr f_ret = Z3_true;
  // get map id according to addr_map
  vector<int> map_ids;
  vector<z3::expr> map_id_path_conds;
  // sv.get_map_id(map_ids, map_id_path_conds);
  sv.get_map_id(map_ids, map_id_path_conds, addr_map);
  // cout << "enter predicate_map_delete_helper" << endl;
  // cout << "addr_map: " << addr_map << endl;
  // for (int i = 0; i < map_ids.size(); i++) cout << map_ids[i] << " " << map_id_path_conds[i] << endl;
  if (map_ids.size() == 0) {string s = "error!!!"; throw (s); return f_ret;} // todo: addr_map is not a pointer
  for (int i = 0; i < map_ids.size(); i++) {
    f_ret = f_ret && predicate_map_delete_helper_for_one_map(map_ids[i], map_id_path_conds[i],
            addr_k, out, sv, block);
  }
  return f_ret;
}

z3::expr predicate_helper_function(int func_id, z3::expr r1, z3::expr r2, z3::expr r3,
                                   z3::expr r4, z3::expr r5, z3::expr r0,
                                   smt_var &sv, unsigned int block) {
  if (func_id == BPF_FUNC_map_lookup) {
    return predicate_map_lookup_helper(r1, r2, r0, sv, block);
  } else if (func_id == BPF_FUNC_map_update) {
    return predicate_map_update_helper(r1, r2, r3, r0, sv, block);
  } else if (func_id == BPF_FUNC_map_delete) {
    return predicate_map_delete_helper(r1, r2, r0, sv, block);
  } else {
    cout << "Error: unknown function id " << func_id << endl; return string_to_expr("true");
  }
}

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

// get an addr-val list for the given memory table
void get_mem_from_mdl(vector<pair<uint64_t, uint8_t>>& mem_addr_val,
                      z3::model& mdl, smt_var& sv, int mem_id,
                      uint64_t addr_start = 0) {
  smt_wt& mem_urt = sv.mem_var._mem_tables[mem_id]._urt;
  for (int i = 0; i < mem_urt.size(); i++) {
    z3::expr z3_is_valid = mem_urt.is_valid[i];
    int is_valid = mdl.eval(z3_is_valid).bool_value();
    if (is_valid != 1) continue; // -1 means z3 false, 1 means z3 true, 0: z3 const (not know it is true or false)

    z3::expr z3_addr = mem_urt.addr[i];
    z3::expr z3_addr_eval = mdl.eval(z3_addr);
    if (!z3_addr_eval.is_numeral()) continue;
    uint64_t addr = z3_addr_eval.get_numeral_uint64() + addr_start;
    // addr whose value is NULL means the entry is invalid
    if (addr == NULL_ADDR) continue;

    z3::expr z3_val = mdl.eval(mem_urt.val[i]); // z3 bv8
    // That z3_val is not numeral means z3 does not care about "z3_val"'s val
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

void counterex_urt_2_input_map(inout_t& input, z3::model& mdl, smt_var& sv, int mem_id, int map_id) {
  vector<pair< uint64_t, uint8_t>> mem_addr_val;
  get_mem_from_mdl(mem_addr_val, mdl, sv, mem_id);

  smt_map_wt& map_urt = sv.mem_var._map_tables[map_id]._urt;
  smt_wt& mem_urt = sv.mem_var._mem_tables[mem_id]._urt;
  unsigned int val_sz = mem_t::map_val_sz(map_id) / NUM_BYTE_BITS;

  for (int i = 0; i < map_urt.size(); i++) {
    z3::expr z3_is_valid = map_urt.is_valid[i];
    int is_valid = mdl.eval(z3_is_valid).bool_value();
    if (is_valid != 1) continue; // -1 means z3 false, 1 means z3 true, 0: z3 const (not know it is true or false)

    z3::expr z3_addr_v = mdl.eval(map_urt.addr_v[i]);
    if (! z3_addr_v.is_numeral()) continue;
    uint64_t addr_v = z3_addr_v.get_numeral_uint64();
    if (addr_v == 0) continue;

    z3::expr z3_k = mdl.eval(map_urt.key[i]);
    if (! z3_k.is_numeral()) continue;
    string k = z3_bv_2_hex_str(z3_k);
    vector<uint8_t> v(val_sz);
    // get the corresponding "v" according to "addr_v"
    get_v_from_addr_v(v, addr_v, mem_addr_val);
    input.update_kv(map_id, k, v);
  }
}

// set input memory, for now, only set pkt
// 1. get mem_addr_val list according to the pkt mem urt;
// 2. traverse mem_addr_val list, if the addr is in input memory address range, update "input"
void counterex_urt_2_input_mem(inout_t& input, z3::model& mdl, smt_var& sv) {
  if (mem_t::_layout._pkt_sz == 0) return;


  z3::expr z3_pkt_start = mdl.eval(sv.get_pkt_start_addr());
  z3::expr z3_pkt_end = mdl.eval(sv.get_pkt_end_addr());
  uint64_t pkt_start = get_uint64_from_bv64(z3_pkt_start, true);
  uint64_t pkt_end = get_uint64_from_bv64(z3_pkt_end, true);

  int pkt_mem_id = sv.mem_var.get_mem_table_id(MEM_TABLE_pkt);
  assert(pkt_mem_id != -1);
  vector<pair< uint64_t, uint8_t>> mem_addr_val;
  get_mem_from_mdl(mem_addr_val, mdl, sv, pkt_mem_id, pkt_start);

  // set pkt with random values
  input.set_pkt_random_val();
  for (int i = 0; i < mem_addr_val.size(); i++) {
    uint64_t addr = mem_addr_val[i].first;
    uint8_t val = mem_addr_val[i].second;
    int idx = addr - pkt_start;
    input.pkt[idx] = val;
  }
}

void counterex_2_input_simu_r10(inout_t& input, z3::model& mdl, smt_var& sv) {
  z3::expr z3_stack_bottom = mdl.eval(sv.get_stack_bottom_addr());
  input.input_simu_r10 = get_uint64_from_bv64(z3_stack_bottom, true); // r10: stack bottom
}

void counterex_2_input_simu_pkt_ptrs(inout_t& input, z3::model& mdl, smt_var& sv) {
  if (mem_t::get_pgm_input_type() == PGM_INPUT_pkt_ptrs) {
    z3::expr z3_pkt_start = mdl.eval(sv.get_pkt_start_addr());
    input.input_simu_pkt_ptrs[0] = (uint32_t)get_uint64_from_bv64(z3_pkt_start, true);
    z3::expr z3_pkt_end = mdl.eval(sv.get_pkt_end_addr());
    input.input_simu_pkt_ptrs[1] = (uint32_t)get_uint64_from_bv64(z3_pkt_end, true);
  }
}

void counterex_urt_2_input_mem_for_one_sv(inout_t& input, z3::model& mdl, smt_var& sv) {
  counterex_2_input_simu_r10(input, mdl, sv);
  counterex_2_input_simu_pkt_ptrs(input, mdl, sv);
  counterex_urt_2_input_mem(input, mdl, sv);
  for (int i = 0; i < mem_t::maps_number(); i++) {
    int mem_id = sv.mem_var.get_mem_table_id(MEM_TABLE_map, i);
    assert(mem_id != -1);
    counterex_urt_2_input_map(input, mdl, sv, mem_id, i);
  }
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
