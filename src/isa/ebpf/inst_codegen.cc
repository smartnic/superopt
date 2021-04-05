#include "inst_codegen.h"

using namespace std;

default_random_engine gen_codegen;
uniform_real_distribution<double> unidist_codegen(0.0, 1.0);

z3::expr latest_write_element(int idx, vector<z3::expr>& is_valid_list, vector<z3::expr>& x);
z3::expr addr_in_addrs(z3::expr a, vector<z3::expr>& is_valid_list, vector<z3::expr>& x);
z3::expr addr_in_addrs_map_mem(z3::expr& a, vector<z3::expr>& is_valid_list, vector<z3::expr>& x);
z3::expr key_not_found_after_idx(z3::expr key, int idx, smt_map_wt& m_wt);
z3::expr key_not_in_map_wt(z3::expr k, smt_map_wt& m_wt, smt_var& sv, bool same_pgms = false, unsigned int block = 0);
uint64_t compute_tail_call_helper(uint64_t ctx_ptr, uint64_t map_id, uint64_t index, prog_state& ps);
uint64_t compute_get_prandom_u32_helper(prog_state& ps);
int reg_ptr_type_2_mem_table_type(int reg_type);
int mem_table_type_2_reg_ptr_type(int mem_table_type);
z3::expr smt_array_mem_set_same_input(smt_var& sv1, smt_var& sv2, int mem_sz, int mem_table_type, int map_id = -1);

z3::expr predicate_get_prandom_u32_helper(z3::expr out, smt_var&sv);
uint64_t get_uint64_from_bv64(z3::expr & z3_val, bool assert);

uint64_t compute_helper_function(int func_id, uint64_t r1, uint64_t r2, uint64_t r3,
                                 uint64_t r4, uint64_t r5, simu_real& sr, prog_state& ps) {
  bool chk_safety = true;
  switch (func_id) {
    case BPF_FUNC_map_lookup_elem: ps.reg_safety_chk(0, vector<int> {1, 2}); return compute_map_lookup_helper(r1, r2, ps, sr, chk_safety);
    case BPF_FUNC_map_update_elem: ps.reg_safety_chk(0, vector<int> {1, 2, 3, 4}); return compute_map_update_helper(r1, r2, r3, ps, sr, chk_safety);
    case BPF_FUNC_map_delete_elem: ps.reg_safety_chk(0, vector<int> {1, 2}); return compute_map_delete_helper(r1, r2, ps, sr, chk_safety);
    case BPF_FUNC_tail_call: ps.reg_safety_chk(0, vector<int> {1, 2, 3}); return compute_tail_call_helper(r1, r2, r3, ps);
    case BPF_FUNC_get_prandom_u32: ps.reg_safety_chk(0, vector<int> {}); return compute_get_prandom_u32_helper(ps);
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
uint64_t compute_map_lookup_helper(int addr_map, uint64_t addr_k, prog_state& ps,
                                   simu_real& sr, bool chk_safety) {
  int map_id = addr_map;
  int k_sz = mem_t::map_key_sz(map_id) / NUM_BYTE_BITS;
  uint64_t real_addr_k = get_real_addr_by_simu(addr_k, ps._mem, sr, PTR_TO_STACK);
  // safety check to avoid segmentation fault
  bool is_mem_read = true;
  bool stack_aligned_chk = false;
  ps.memory_access_and_safety_chk(real_addr_k, k_sz, chk_safety, is_mem_read, stack_aligned_chk);
  // get key from memory
  string k = ld_n_bytes_from_addr((uint8_t*)real_addr_k, k_sz);
  map_t& mp = ps._mem._maps[map_id];
  auto it = mp._k2idx.find(k);
  if (it == mp._k2idx.end()) return NULL_ADDR;
  int v_idx_in_map = it->second;
  int v_mem_off = ps._mem.get_mem_off_by_idx_in_map(map_id, v_idx_in_map);
  uint64_t real_addr_v = (uint64_t)&ps._mem._mem[v_mem_off];
  return get_simu_addr_by_real(real_addr_v, ps._mem, sr);
}

uint64_t compute_map_update_helper(int addr_map, uint64_t addr_k, uint64_t addr_v, prog_state& ps,
                                   simu_real& sr, bool chk_safety) {
  int map_id = addr_map;
  int k_sz = mem_t::map_key_sz(map_id) / NUM_BYTE_BITS;
  uint64_t real_addr_k = get_real_addr_by_simu(addr_k, ps._mem, sr, PTR_TO_STACK);
  bool is_mem_read = true;
  bool stack_aligned_chk = false;
  ps.memory_access_and_safety_chk(real_addr_k, k_sz, chk_safety, is_mem_read, stack_aligned_chk);
  // get key and value from memory
  string k = ld_n_bytes_from_addr((uint8_t*)real_addr_k, k_sz);
  uint64_t real_addr_v = get_real_addr_by_simu(addr_v, ps._mem, sr, PTR_TO_STACK);
  ps._mem.update_kv_in_map(map_id, k, (uint8_t*)real_addr_v);
  return MAP_UPD_RET;
}

uint64_t compute_map_delete_helper(int addr_map, uint64_t addr_k, prog_state& ps,
                                   simu_real& sr, bool chk_safety) {
  int map_id = addr_map;
  int k_sz = mem_t::map_key_sz(map_id) / NUM_BYTE_BITS;
  uint64_t real_addr_k = get_real_addr_by_simu(addr_k, ps._mem, sr, PTR_TO_STACK);
  bool is_mem_read = true;
  bool stack_aligned_chk = false;
  ps.memory_access_and_safety_chk(real_addr_k, k_sz, chk_safety, is_mem_read, stack_aligned_chk);
  string k = ld_n_bytes_from_addr((uint8_t*)real_addr_k, k_sz);
  map_t& mp = ps._mem._maps[map_id];
  auto it = mp._k2idx.find(k);
  if (it == mp._k2idx.end()) {
    return MAP_DEL_RET_IF_KEY_INEXIST;
  }
  mp.add_available_idx(it->second);
  mp._k2idx.erase(it);
  return MAP_DEL_RET_IF_KEY_EXIST;
}

uint64_t compute_tail_call_helper(uint64_t ctx_ptr, uint64_t map_id, uint64_t index, prog_state& ps) {
  // 1. ctx_ptr should be input r1, else throw error
  if (ctx_ptr != ps._input_reg_val) {
    string err_msg = "ERROR: tail call parameter 1 wrong ctx_ptr";
  }
  // 2. map_id should be prog_array map id, else throw error
  int map_type = mem_t::map_type(map_id);
  if (map_type != MAP_TYPE_prog_array) {
    string err_msg = "ERROR: tail call parameter 2 map type is not MAP_TYPE_prog_array";
    throw (err_msg);
  }
  // 3. index should be in [0, prog_array map's max entries), else throw error
  unsigned int map_max_entries = mem_t::map_max_entries(map_id);
  if (map_id >= map_max_entries) {
    string err_msg = "ERROR: tail call parameter 3 index is outside [0, " + to_string(map_max_entries) + ")";
    throw (err_msg);
  }

  ps._tail_call_para = index;
  return 0; // 0 means successful
}

uint64_t compute_get_prandom_u32_helper(prog_state& ps) {
  return (uint64_t)ps.get_next_random_u32();
}

z3::expr predicate_ldmapid(z3::expr map_id, z3::expr out, smt_var& sv, unsigned int block) {
  z3::expr path_cond = sv.mem_var.get_block_path_cond(block);
  sv.add_expr_map_id(out, map_id, path_cond);
  return (map_id == out);
}

z3::expr predicate_ld32(z3::expr addr, z3::expr off, smt_var& sv, z3::expr out, unsigned int block,
                        bool enable_addr_off, bool is_win) {
  int pgm_input_type = mem_t::get_pgm_input_type();
  if (pgm_input_type == PGM_INPUT_pkt_ptrs) {
    int pkt_ptrs_mem_table = sv.mem_var.get_mem_table_id(MEM_TABLE_pkt_ptrs);
    vector<int> ids;
    vector<vector<mem_ptr_info>> info_list;
    sv.mem_var.get_mem_ptr_info(ids, info_list, addr);
    for (int i = 0; i < ids.size(); i++) {
      if (ids[i] != pkt_ptrs_mem_table) continue;
      for (int j = 0; j < info_list[i].size(); j++) {
        z3::expr addr_off = info_list[i][j].off + off;
        if (! enable_addr_off) {
          addr_off = addr + off - sv.get_pkt_start_addr();
        }
        // the first 4 bytes are the address of pkt start, the last are the address of pkt end
        z3::expr pc_addr_off = (addr_off == ZERO_ADDR_OFF_EXPR) || (addr_off == to_expr((int64_t)4));
        z3::expr pc = info_list[i][j].path_cond && sv.mem_var.get_block_path_cond(block);
        int pkt_mem_table = sv.mem_var.get_mem_table_id(MEM_TABLE_pkt);
        sv.mem_var.add_ptr(out, pkt_mem_table, addr_off, pc && pc_addr_off);
      }
    }
  } else if (pgm_input_type == PGM_INPUT_skb) {
    int pkt_mem_table = sv.mem_var.get_mem_table_id(MEM_TABLE_pkt);
    vector<int> ids;
    vector<vector<mem_ptr_info>> info_list;
    sv.mem_var.get_mem_ptr_info(ids, info_list, addr);
    for (int i = 0; i < ids.size(); i++) {
      if (ids[i] != pkt_mem_table) continue;
      for (int j = 0; j < info_list[i].size(); j++) {
        z3::expr addr_off = info_list[i][j].off + off;
        if (! enable_addr_off) {
          addr_off = addr + off - sv.mem_var._skb_start;
        }
        z3::expr pc_addr_off_s = (addr_off == to_expr(SKB_data_s_off, 64));
        z3::expr pc = info_list[i][j].path_cond && sv.mem_var.get_block_path_cond(block);
        int skb_mem_table = sv.mem_var.get_mem_table_id(MEM_TABLE_skb);
        z3::expr skb_off = NULL_ADDR_EXPR;
        sv.mem_var.add_ptr(out, skb_mem_table, skb_off, pc && pc_addr_off_s);
        // todo: add skb_data_end as a pointer
        // z3::expr pc_addr_off_e = (addr_off == to_expr(SKB_data_e_off, 64));
        // skb_off = sv.mem_var._skb_end - sv.mem_var._skb_start;
        // sv.mem_var.add_ptr(out, skb_mem_table, skb_off, pc && pc_addr_off_e);
      }
    }
  }
  return ((out.extract(63, 32) == to_expr(0, 32)) &&
          predicate_ld_byte(addr, off, sv, out.extract(7, 0), block, Z3_true, enable_addr_off, is_win) &&
          predicate_ld_byte(addr, off + 1, sv, out.extract(15, 8), block, Z3_true, enable_addr_off, is_win) &&
          predicate_ld_byte(addr, off + 2, sv, out.extract(23, 16), block, Z3_true, enable_addr_off, is_win) &&
          predicate_ld_byte(addr, off + 3, sv, out.extract(31, 24), block, Z3_true, enable_addr_off, is_win));
}

// check whether entry_start,... entry_start+7 combines a pointer
bool is_a_ptr_in_mem_table(smt_wt& mem_table, int entry_start) {
  const int ptr_sz = 8;
  int entry_end = entry_start + ptr_sz - 1;
  if ((entry_start < 0) || entry_end >= mem_table.size()) {
    return false;
  }
  for (int i = entry_start; i <= entry_end; i++) {
    if (! mem_table.ptr_info[i].is_ptr) return false;
  }

  // check the ptr_expr the same
  unsigned int ptr_expr_id = mem_table.ptr_info[entry_start].ptr_expr.id();
  for (int i = entry_start + 1; i <= entry_end; i++) {
    unsigned int id = mem_table.ptr_info[i].ptr_expr.id();
    if (ptr_expr_id != id) return false;
  }

  // check ptr_off, ptr_off should be 0, 1, ~ 7
  vector<int> off_count(ptr_sz);
  for (int i = 0; i < off_count.size(); i++) {
    off_count[i] = 0;
  }
  for (int i = entry_start; i <= entry_end; i++) {
    int ptr_off = mem_table.ptr_info[i].ptr_off;
    if ((ptr_off < 0) || (ptr_off >= off_count.size())) return false;
    off_count[ptr_off]++;
  }
  for (int i = 0; i < off_count.size(); i++) {
    if (off_count[i] != 1) return false;
  }
  return true;
}

// load pointers from off_start in the given memory table
void get_ptrs_from_off_based_mem_table(vector<z3::expr>& ptr_exprs,
                                       vector<z3::expr>& ptr_conds,
                                       uint64_t off_start, int mem_table_id,
                                       smt_var& sv) {
  ptr_exprs.clear();
  // now only supports stack memory stores pointers
  if (mem_table_id != sv.mem_var.get_mem_table_id(MEM_TABLE_stack)) {
    return;
  }
  // check whether stack_off is the off_start of a pointer
  // walk through memory wt backwards (latest write)
  smt_wt& wt = sv.mem_var._mem_tables[mem_table_id]._wt;
  vector<int> poss_entries;
  // filter entries that do not match by offset
  for (int i = wt.size() - 1; i >= 0; i--) {
    z3::expr mem_off_expr = wt.addr[i];
    uint64_t mem_off = get_uint64_from_bv64(mem_off_expr, true);
    if (mem_off != off_start) continue;
    poss_entries.push_back(i);
  }
  z3::expr f_not_in_writes_after_i = Z3_true; // for latest write
  for (int i = 0; i < poss_entries.size(); i++) {
    int entry = poss_entries[i];
    // check whether is a pointer
    if (! is_a_ptr_in_mem_table(wt, entry)) continue;

    ptr_exprs.push_back(wt.ptr_info[entry].ptr_expr);
    // since entry_i to entry_i+7 is a pointer, is_valid of these 8 entris are the same
    z3::expr cond = f_not_in_writes_after_i && wt.is_valid[entry];
    ptr_conds.push_back(cond);

    // update f_not_in_writes_after_i
    f_not_in_writes_after_i = f_not_in_writes_after_i && (! wt.is_valid[entry]);
  }

  // walk through memory urt
  z3::expr f_not_in_wt = f_not_in_writes_after_i;
  smt_wt& urt = sv.mem_var._mem_tables[mem_table_id]._urt;
  for (int i = 0; i < urt.size(); i++) {
    // check whether off_start matches off[i], since a pointer is stored as
    // 8 bytes from low address to high.
    z3::expr mem_off_expr = urt.addr[i];
    uint64_t mem_off = get_uint64_from_bv64(mem_off_expr, true);
    if (mem_off != off_start) continue;
    // check whether it's a pointer
    if (! is_a_ptr_in_mem_table(urt, i)) continue;

    // push pointer and it's condition in the return vectors
    // condition means: if condition, ptr_expr is a pointer
    ptr_exprs.push_back(urt.ptr_info[i].ptr_expr);
    // since entry_i to entry_i+7 is a pointer, is_valid of these 8 entris are the same
    z3::expr cond = f_not_in_wt && urt.is_valid[i];
    ptr_conds.push_back(cond);
  }
}

z3::expr predicate_ld64(z3::expr addr, z3::expr off, smt_var& sv, z3::expr out,
                        unsigned int block, bool enable_addr_off, bool is_win) {
  z3::expr f = Z3_true;
  if (is_win) {
    // add pointer into pointer table if ld64 loads a pointer from the stack
    // 1. memory table must be stack memory
    // 2. the stack memory stores a pointer
    bool flag_ptr_stack = true;
    int stack_mem_table = sv.mem_var.get_mem_table_id(MEM_TABLE_stack);
    vector<int> ids;
    vector<vector<mem_ptr_info>> info_list;
    sv.mem_var.get_mem_ptr_info(ids, info_list, addr);
    for (int i = 0; i < ids.size(); i++) {
      if (ids[i] != stack_mem_table) {
        flag_ptr_stack = false;
        break;
      }
    }
    if (flag_ptr_stack) {
      for (int i = 0; i < ids.size(); i++) {
        // check is a pointer
        for (int j = 0; j < info_list[i].size(); j++) {
          z3::expr z3_stack_off = info_list[i][j].off + off;
          // get the concrete value of z3_stack_off
          uint64_t stack_off = get_uint64_from_bv64(z3_stack_off, true);
          vector<z3::expr> ptr_exprs, ptr_conds;
          get_ptrs_from_off_based_mem_table(ptr_exprs, ptr_conds, stack_off, stack_mem_table, sv);
          for (int ptr_id = 0; ptr_id < ptr_exprs.size(); ptr_id++) {
            // get the ptr_info according to ptr_expr and add all possible pointers in
            // the pointer table
            vector<int> ptr_mem_table_ids;
            vector<vector<mem_ptr_info>> ptr_info_list;
            sv.mem_var.get_mem_ptr_info(ptr_mem_table_ids, ptr_info_list, ptr_exprs[ptr_id]);
            for (int m = 0; m < ptr_info_list.size(); m++) {
              for (int n = 0; n < ptr_info_list[m].size(); n++) {
                z3::expr ptr_off = ptr_info_list[m][n].off;
                z3::expr ptr_pc = ptr_conds[ptr_id] && ptr_info_list[m][n].path_cond;
                sv.mem_var.add_ptr(out, ptr_mem_table_ids[m], ptr_off, ptr_pc);
              }
            }
          }
        }
      }
    }
  }

  f = predicate_ld_byte(addr, off, sv, out.extract(7, 0), block, Z3_true, enable_addr_off, is_win);
  for (int i = 1; i < 8; i++) {
    f = f && predicate_ld_byte(addr, off + i, sv, out.extract(8 * i + 7, 8 * i), block, Z3_true, enable_addr_off, is_win);
  }
  return f;
}

z3::expr predicate_st_byte(z3::expr in, z3::expr addr, z3::expr off, smt_var& sv,
                           unsigned int block, z3::expr cond, bool bpf_st,
                           bool enable_addr_off) {
  z3::expr path_cond = sv.mem_var.get_block_path_cond(block) && cond;
  z3::expr f = Z3_true;
  vector<int> ids;
  vector<vector<mem_ptr_info>> info_list;
  sv.mem_var.get_mem_ptr_info(ids, info_list, addr);
  // cout << "enter predicate_st_byte" << endl;
  // cout << "addr: " << addr << endl;
  // cout << "pc: " << path_cond.simplify() << endl;
  // for (int i = 0; i < ids.size(); i++) cout << ids[i] << " " << info_list[i].off << " " << info_list[i].path_cond << endl;
  if (ids.size() == 0) {string s = "error!!!"; throw (s); return Z3_true;} // todo: addr is not a pointer
  for (int i = 0; i < ids.size(); i++) {
    for (int j = 0; j < info_list[i].size(); j++) {
      z3::expr is_valid = sv.update_is_valid();
      z3::expr cond = path_cond && info_list[i][j].path_cond;
      f = f && (is_valid == cond);
      // safety check of BPF_ST storing in the input memory is not allowed
      if (bpf_st) {
        bool is_input_mem_table = false;
        int input_mem_table = MEM_TABLE_pkt;
        if (mem_t::get_pgm_input_type() == PGM_INPUT_pkt_ptrs) {
          is_input_mem_table = (ids[i] == sv.mem_var.get_mem_table_id(MEM_TABLE_pkt_ptrs));
        } else {
          is_input_mem_table = (ids[i] == sv.mem_var.get_mem_table_id(MEM_TABLE_pkt));
        }
        if (is_input_mem_table && smt_var::enable_multi_mem_tables) {
          // todo: safety check of a single mem table has not been implemented, so skip here
          string err_msg = "BPF_ST stores into PTR_TO_CTX reg is not allowed";
          throw (err_msg);
        }
      }
      bool is_addr_off_table = false;
      if (enable_addr_off) {
        is_addr_off_table = (ids[i] == sv.mem_var.get_mem_table_id(MEM_TABLE_stack)) ||
                            (ids[i] == sv.mem_var.get_mem_table_id(MEM_TABLE_pkt)) ||
                            (ids[i] == sv.mem_var.get_mem_table_id(MEM_TABLE_pkt_ptrs)) ||
                            (ids[i] == sv.mem_var.get_mem_table_id(MEM_TABLE_skb));
        if (smt_var::is_win) is_addr_off_table = true;
      }
      if (is_addr_off_table) { // addr in the entry is offset
        z3::expr addr_off = off + info_list[i][j].off;
        // cout << "is_addr_off_table  addr_off: " << addr_off.simplify() << endl;
        sv.mem_var.add_in_mem_table_wt(ids[i], block, is_valid, addr_off, in.extract(7, 0));
      } else {
        sv.mem_var.add_in_mem_table_wt(ids[i], block, is_valid, addr + off, in.extract(7, 0));
      }
    }
  }
  return f;
}

// this func is only called by map helper functions
z3::expr predicate_st_n_bytes(int n, z3::expr in, z3::expr addr, smt_var& sv,
                              unsigned int block, z3::expr cond, bool enable_addr_off) {
  z3::expr f = string_to_expr("true");
  for (int i = 0; i < n; i++) {
    f = f && predicate_st_byte(in.extract(8 * i + 7, 8 * i), addr, to_expr(i, 64),
                               sv, block, cond, false, enable_addr_off);
  }
  return f;
}

z3::expr predicate_st64(z3::expr in, z3::expr addr, z3::expr off, smt_var& sv,
                        unsigned int block, bool bpf_st, bool enable_addr_off) {
  z3::expr f = predicate_st32(in.extract(31, 0), addr, off, sv, block, bpf_st, enable_addr_off);
  f = f && predicate_st32(in.extract(63, 32), addr, off + to_expr(4, 64), sv, block, bpf_st, enable_addr_off);
  // modify entries' pointer info if st64 stores a pointer on the stack
  if (smt_var::is_win && (! bpf_st)) { // flag bpf_st means store an immediate number, which is not a pointer
    // 1. memory table must be stack memory ("addr" must be stack memory)
    // 2. store a pointer in the memory (expression "in" must be a pointer)
    bool mem_must_be_stack = true;
    int stack_mem_table = sv.mem_var.get_mem_table_id(MEM_TABLE_stack);
    vector<int> ids;
    vector<vector<mem_ptr_info>> info_list;
    sv.mem_var.get_mem_ptr_info(ids, info_list, addr);
    for (int i = 0; i < ids.size(); i++) {
      if (ids[i] != stack_mem_table) {
        mem_must_be_stack = false;
        break;
      }
    }
    bool store_a_pointer = true;
    vector<int> store_ids;
    vector<vector<mem_ptr_info>> store_info_list;
    sv.mem_var.get_mem_ptr_info(store_ids, store_info_list, in);
    if (store_ids.size() == 0) store_a_pointer = false;
    if (mem_must_be_stack && store_a_pointer) {
      // modify pointer info of entries added by this st64
      const int ptr_sz = 8;
      assert(ids.size() == 1);  // since memory table must be stack
      int n_entries = info_list[0].size() * ptr_sz;
      smt_wt& wt = sv.mem_var._mem_tables[stack_mem_table]._wt;
      int entry_start = wt.size() - n_entries;
      for (int i = 0; i < n_entries; i += ptr_sz) {
        for (int j = 0; j < ptr_sz; j++) {
          mem_table_ptr_info ptr_info;
          ptr_info.is_ptr = true;
          ptr_info.ptr_expr = in;
          ptr_info.ptr_off = j; // add pointer byte by byte from low address to high
          wt.ptr_info[entry_start + i + j] = ptr_info;
        }
      }
    }
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
    z3::expr addr, z3::expr off, smt_var& sv, z3::expr out, unsigned int block, z3::expr cond = Z3_true,
    bool enable_addr_off = true, bool is_win = false) {
  cond = ptr_info.path_cond && sv.mem_var.get_block_path_cond(block) && cond;
  smt_wt& wt = sv.mem_var._mem_tables[table_id]._wt;
  z3::expr a = addr + off;
  bool is_addr_off_table = false;
  if (enable_addr_off) {
    is_addr_off_table = (table_id == sv.mem_var.get_mem_table_id(MEM_TABLE_stack)) ||
                        (table_id == sv.mem_var.get_mem_table_id(MEM_TABLE_pkt)) ||
                        (table_id == sv.mem_var.get_mem_table_id(MEM_TABLE_pkt_ptrs)) ||
                        (table_id == sv.mem_var.get_mem_table_id(MEM_TABLE_skb));
    if (smt_var::is_win) is_addr_off_table = true;
  }

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
  if (smt_var::smt_var::enable_multi_mem_tables) {
    if ((table_id == sv.mem_var.get_mem_table_id(MEM_TABLE_stack)) && (! is_win)) {
      f = z3::implies(cond, f);
      return f;
    }
  }
  // add constrains on the element(a, out)
  smt_wt& urt = sv.mem_var._mem_tables[table_id]._urt;
  z3::expr not_found_in_wt = sv.update_is_valid();
  f = f && (not_found_in_wt == (!f_found_in_wt_after_i));
  if (is_addr_off_table) f = f && z3::implies(not_found_in_wt, urt_element_constrain(a, out, urt));
  else f = f && z3::implies(not_found_in_wt, urt_element_constrain_map_mem(a, out, urt));
  f = z3::implies(cond, f);
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

z3::expr predicate_ld_byte(z3::expr addr, z3::expr off, smt_var& sv, z3::expr out, unsigned int block, z3::expr cond,
                           bool enable_addr_off, bool is_win) {
  // cout << "predicate_ld_byte: " << endl;
  // cout << "addr: " << addr << endl;
  z3::expr f = Z3_true;
  vector<int> ids;
  vector<vector<mem_ptr_info>> info_list;
  sv.mem_var.get_mem_ptr_info(ids, info_list, addr);
  // cout << "enter predicate_ld_byte" << endl;
  // cout << "addr: " << addr << endl;
  // for (int i = 0; i < ids.size(); i++) cout << ids[i] << " " << info_list[i].off << " " << info_list[i].path_cond << endl;
  // cout << "ids.size(): " << ids.size() << endl;
  if (ids.size() == 0) {string s = "error!!!"; throw (s); return Z3_true;} // todo: addr is not a pointer
  for (int i = 0; i < ids.size(); i++) {
    for (int j = 0; j < info_list[i].size(); j++) {
      f = f && predicate_ld_byte_for_one_mem_table(ids[i], info_list[i][j], addr, off,
          sv, out, block, cond, enable_addr_off, is_win);
    }
  }
  return f;
}

z3::expr predicate_ld_n_bytes(int n, z3::expr addr, smt_var& sv, z3::expr out, unsigned int block, z3::expr cond,
                              bool enable_addr_off, bool is_win) {
  z3::expr f = predicate_ld_byte(addr, to_expr(0, 64), sv, out.extract(7, 0), block, cond, enable_addr_off, is_win);
  for (int i = 1; i < n; i++) {
    f = f && predicate_ld_byte(addr, to_expr(i, 64), sv, out.extract(8 * i + 7, 8 * i), block, cond, enable_addr_off, is_win);
  }
  return f;
}

z3::expr predicate_xadd64(z3::expr in, z3::expr addr, z3::expr off, smt_var& sv, unsigned int block, bool enable_addr_off, bool is_win) {
  z3::expr v64_1 = sv.new_var(64);
  z3::expr f = predicate_ld64(addr, off, sv, v64_1, block, enable_addr_off, is_win);
  z3::expr v64_2 = sv.new_var(64);
  f = f && (v64_2 == (v64_1 + in));
  bool bpf_st = false; // xadd64 opcode is not BPF_ST
  f = f && predicate_st64(v64_2, addr, off, sv, block, bpf_st, enable_addr_off);
  return f;
}

z3::expr predicate_xadd32(z3::expr in, z3::expr addr, z3::expr off, smt_var& sv, unsigned int block, bool enable_addr_off, bool is_win) {
  z3::expr v64_1 = sv.new_var(64);
  z3::expr f = predicate_ld32(addr, off, sv, v64_1, block, enable_addr_off, is_win);
  z3::expr v64_2 = sv.new_var(64);
  f = f && (v64_2 == (v64_1 + in));
  bool bpf_st = false; // xadd32 opcode is not BPF_ST
  f = f && predicate_st32(v64_2, addr, off, sv, block, bpf_st, enable_addr_off);
  return f;
}

// out == *(u8*)skb[off], out: bv8
z3::expr predicate_ldskb_byte(z3::expr off, smt_var& sv, z3::expr out, unsigned int block = 0) {
  z3::expr f = Z3_true;
  int pkt_sz = mem_t::_layout._pkt_sz;
  if (pkt_sz == 0) return f; // mean no constraints
  f = f && ugt(pkt_sz, off);
  z3::expr cond = sv.mem_var.get_block_path_cond(block);
  // todo: check the off range
  int table_id = sv.mem_var.get_mem_table_id(MEM_TABLE_pkt);
  // skb stable has no wt, so only need to search off in the urt
  smt_wt& urt = sv.mem_var._mem_tables[table_id]._urt;
  f = f && urt_element_constrain(off, out, urt);
  z3::expr is_valid = sv.update_is_valid();
  f = f && (is_valid == cond);
  urt.add(block, is_valid, off, out);
  return f;
}

// out == *(u8*)skb[off], out: bv64
z3::expr predicate_ldskbh(z3::expr off, smt_var& sv, z3::expr out, unsigned int block) {
  return ((out.extract(63, 16) == to_expr(0, 48)) &&
          predicate_ldskb_byte(off, sv, out.extract(7, 0), block) &&
          predicate_ldskb_byte(off + 1, sv, out.extract(15, 8), block));
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

z3::expr addr_in_addrs(z3::expr a, vector<z3::expr>& is_valid_list, vector<z3::expr>& x) {
  z3::expr f = string_to_expr("false");
  for (int i = 0; i < x.size(); i++) {
    f = f || ((a == x[i]) && is_valid_list[i]);
  }
  return f;
}

z3::expr addr_in_mem_range(z3::expr addr, z3::expr mem_s, z3::expr mem_e) {
  // 1. mem_s <= addr; 2. addr <= mem_e
  return ule(mem_s, addr) && ule(addr, mem_e);
}

// pkt address only in mem_p1 wt => pkt address in mem_p1 urt && same value in wt and urt
z3::expr array_mem_addr_in_one_wt(smt_var& sv1, smt_var& sv2, int mem_sz, int mem_table_type, int map_id) {
  z3::expr f = Z3_true;
  if (mem_sz == 0) return f;
  int id1 = sv1.mem_var.get_mem_table_id(mem_table_type, map_id);
  int id2 = sv2.mem_var.get_mem_table_id(mem_table_type, map_id);
  assert(id1 != -1);
  assert(id2 != -1);
  smt_wt& wt1 = sv1.mem_var._mem_tables[id1]._wt;
  smt_wt& wt2 = sv2.mem_var._mem_tables[id2]._wt;
  smt_wt& urt1 = sv1.mem_var._mem_tables[id1]._urt;

  // get mem_s and mem_e according to mem_table_type and map_id
  // sv1 and sv2 have the same mem_s and mem_e
  z3::expr mem_s = sv1.get_mem_start(mem_table_type, map_id);
  z3::expr mem_e = sv1.get_mem_end(mem_table_type, map_id);

  for (int i = wt1.size() - 1; i >= 0; i--) {
    z3::expr iv_out = wt1.is_valid[i];
    z3::expr a_out = wt1.addr[i];
    z3::expr v_out = wt1.val[i];
    z3::expr f_a_out = iv_out && latest_write_element(i, wt1.is_valid, wt1.addr);
    z3::expr f_a_not_in_wt2 = !addr_in_addrs(a_out, wt2.is_valid, wt2.addr);
    z3::expr f_a_in_urt1 = addr_in_addrs(a_out, urt1.is_valid, urt1.addr);
    z3::expr a_out_in_mem = Z3_true;
    if (! smt_var::enable_multi_mem_tables) {
      a_out_in_mem = addr_in_mem_range(a_out, mem_s, mem_e);
    }

    f = f && z3::implies(a_out_in_mem && f_a_out && f_a_not_in_wt2, f_a_in_urt1);


    for (int j = 0; j < urt1.size(); j++) {
      z3::expr iv_in = urt1.is_valid[j];
      z3::expr a_in = urt1.addr[j];
      z3::expr v_in = urt1.val[j];

      f = f && z3::implies(a_out_in_mem && iv_in && f_a_out &&
                           f_a_not_in_wt2 && (a_out == a_in),
                           v_out == v_in);
    }
  }
  return f;
}

// offs = offs_1[reg_ptr_type] + offs_2[reg_ptr_type]
void get_offs_in_both_output_offs(unordered_set<int>&offs, int reg_ptr_type,
                                  unordered_map<int, unordered_set<int>>& offs_1,
                                  unordered_map<int, unordered_set<int>>& offs_2) {
  offs.clear();
  auto find1 = offs_1.find(reg_ptr_type);
  if (find1 != offs_1.end()) {
    for (auto off : find1->second) {
      offs.insert(off);
    }
  }
  auto find2 = offs_2.find(reg_ptr_type);
  if (find2 != offs_2.end()) {
    for (auto off : find2->second) {
      offs.insert(off);
    }
  }
}

// offs = offs_1[reg_ptr_type] - offs_2[reg_ptr_type]
void get_offs_in_one_output_offs(unordered_set<int>&offs, int reg_ptr_type,
                                 unordered_map<int, unordered_set<int>>& offs_1,
                                 unordered_map<int, unordered_set<int>>& offs_2) {
  offs.clear();
  auto find1 = offs_1.find(reg_ptr_type);
  auto find2 = offs_2.find(reg_ptr_type);
  if (find1 == offs_1.end()) return;
  if (find2 == offs_1.end()) {
    for (auto off : find1->second) {
      offs.insert(off);
    }
    return;
  }
  for (auto off : find1->second) {
    if (find2->second.find(off) == find2->second.end()) {
      offs.insert(off);
    }
  }
}

z3::expr ld_byte_from_wt_by_off(z3::expr off, smt_wt& wt, z3::expr out) {
  z3::expr a = off;
  z3::expr f = Z3_true;
  z3::expr f_found_after_i = Z3_false;
  for (int i = wt.addr.size() - 1; i >= 0; i--) {
    z3::expr f_found_i = wt.is_valid[i] && (a == wt.addr[i]);
    f = f && z3::implies((!f_found_after_i) && f_found_i, // latest write
                         out == wt.val[i]);
    f_found_after_i = f_found_after_i || f_found_i;
  }
  return f;
}

z3::expr ld_byte_from_urt_by_off(z3::expr off, smt_wt& urt, z3::expr out) {
  z3::expr a = off;
  z3::expr f = Z3_true;
  for (int i = 0; i < urt.addr.size(); i++) {
    z3::expr f_found_i = urt.is_valid[i] && (a == urt.addr[i]);
    f = f && z3::implies(f_found_i, out == urt.val[i]);
  }
  return f;
}

z3::expr smt_array_mem_eq_chk_win_one_mem(unordered_set<int>& offs, smt_var& sv, int table_id, int reg_ptr_type) {
  z3::expr f = Z3_true;
  smt_wt& wt = sv.mem_var._mem_tables[table_id]._wt;
  smt_wt& urt = sv.mem_var._mem_tables[table_id]._urt;
  // each off should be found in input(urt)
  for (auto off : offs) {
    z3::expr f_off_in_urt = addr_in_addrs(to_expr(off), urt.is_valid, urt.addr);
    f = f && f_off_in_urt;
  }
  // each off should be the same as input(urt)
  for (auto off : offs) {
    z3::expr off_expr = to_expr(off);
    z3::expr v_out = sv.smt_out.off_val_expr(reg_ptr_type, off);
    z3::expr v_in = smt_input::off_val_expr(reg_ptr_type, off);
    z3::expr f1 = ld_byte_from_wt_by_off(off_expr, wt, v_out);
    z3::expr f2 = ld_byte_from_urt_by_off(off_expr, urt, v_in);
    f = f && z3::implies(f1 && f2, v_out == v_in);
  }
  return f;
}

z3::expr smt_array_mem_eq_chk_win(smt_var& sv1, smt_var& sv2, int mem_sz, int mem_table_type, int map_id) {
  if (mem_sz == 0) return Z3_true;
  int reg_ptr_type = mem_table_type_2_reg_ptr_type(mem_table_type);
  assert(reg_ptr_type != -1);
  unordered_set<int> offs_12, offs_1, offs_2;
  get_offs_in_both_output_offs(offs_12, reg_ptr_type, sv1.smt_out.output_var.mem, sv2.smt_out.output_var.mem);
  get_offs_in_one_output_offs(offs_1, reg_ptr_type, sv1.smt_out.output_var.mem, sv2.smt_out.output_var.mem);
  get_offs_in_one_output_offs(offs_2, reg_ptr_type, sv2.smt_out.output_var.mem, sv1.smt_out.output_var.mem);

  int id1 = sv1.mem_var.get_mem_table_id(mem_table_type, map_id);
  int id2 = sv2.mem_var.get_mem_table_id(mem_table_type, map_id);

  assert(id1 != -1);
  assert(id2 != -1);
  z3::expr f = Z3_true;
  smt_wt& wt1 = sv1.mem_var._mem_tables[id1]._wt;
  smt_wt& wt2 = sv2.mem_var._mem_tables[id2]._wt;

  // case 1: offsets written in both programs
  for (auto off : offs_12) {
    z3::expr off_expr = to_expr(off);
    z3::expr v1 = sv1.smt_out.off_val_expr(reg_ptr_type, off);
    z3::expr v2 = sv2.smt_out.off_val_expr(reg_ptr_type, off);
    z3::expr f1 = ld_byte_from_wt_by_off(off_expr, wt1, v1);
    z3::expr f2 = ld_byte_from_wt_by_off(off_expr, wt2, v2);
    f = f && z3::implies(f1 && f2, v1 == v2);
  }
  // case 2: offsets in program 1 not program 2
  f = f && smt_array_mem_eq_chk_win_one_mem(offs_1, sv1, id1, reg_ptr_type);
  // case 3: offsets in program 2 not program 1
  f = f && smt_array_mem_eq_chk_win_one_mem(offs_2, sv2, id2, reg_ptr_type);
  return f;
}

// [mem_s, mem_e], addr-based memory start and end addresses
// 1. address in both mem_p1 wt and mem_p2 wt => same value (latest write)
// 2. address in one of wts => value (latest write) == input
z3::expr smt_array_mem_eq_chk(smt_var& sv1, smt_var& sv2, int mem_sz,
                              int mem_table_type, bool is_win, int map_id = -1) {
  if ((is_win) && (map_id == -1)) {
    // since win eq's memory table is off-based, don't need mem_s, mem_e
    z3::expr f = smt_array_mem_eq_chk_win(sv1, sv2, mem_sz, mem_table_type, map_id);
    return f;
  }
  if (mem_sz == 0) return Z3_true;
  int id1 = sv1.mem_var.get_mem_table_id(mem_table_type, map_id);
  int id2 = sv2.mem_var.get_mem_table_id(mem_table_type, map_id);
  assert(id1 != -1);
  assert(id2 != -1);
  z3::expr f = Z3_true;
  smt_wt& wt1 = sv1.mem_var._mem_tables[id1]._wt;
  smt_wt& wt2 = sv2.mem_var._mem_tables[id2]._wt;
  // get mem_s and mem_e according to mem_table_type and map_id
  // sv1 and sv2 have the same mem_s and mem_e
  z3::expr mem_s = sv1.get_mem_start(mem_table_type, map_id);
  z3::expr mem_e = sv1.get_mem_end(mem_table_type, map_id);
  // case 1: pkt address in both wts, latest write should be the same
  if ((wt1.size() > 0) && (wt2.size() > 0)) {
    for (int i = wt1.size() - 1; i >= 0; i--) {
      z3::expr iv1 = wt1.is_valid[i];
      z3::expr a1 = wt1.addr[i];
      z3::expr v1 = wt1.val[i];
      z3::expr f_a1 = latest_write_element(i, wt1.is_valid, wt1.addr);
      z3::expr a1_in_mem = Z3_true;
      if (! smt_var::enable_multi_mem_tables) {
        a1_in_mem = addr_in_mem_range(a1, mem_s, mem_e);
      }

      for (int j = wt2.size() - 1; j >= 0; j--) {
        z3::expr iv2 = wt2.is_valid[j];
        z3::expr a2 = wt2.addr[j];
        z3::expr v2 = wt2.val[j];
        z3::expr f_a2 = latest_write_element(j, wt2.is_valid, wt2.addr);
        z3::expr a2_in_mem = Z3_true;

        f = f && z3::implies(a1_in_mem && iv1 && iv2 && f_a1 && f_a2 && (a1 == a2), v1 == v2);
      }
    }
  }
  // case 2: pkt address in one of wts
  // f = f && pkt_addr_in_one_wt(sv1, sv2) && pkt_addr_in_one_wt(sv2, sv1);
  z3::expr f1 = array_mem_addr_in_one_wt(sv1, sv2, mem_sz, mem_table_type, map_id);
  z3::expr f2 = array_mem_addr_in_one_wt(sv2, sv1, mem_sz, mem_table_type, map_id);
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
      z3::expr iv_k1_in = map1._urt.is_valid[j];
      z3::expr f_found_same_key = iv_k1_in && (k1_list[i] == k1_in) && f_k1_list[i] && f_k1_not_in_map2;
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
    if (! smt_var::is_win) {
      f = f && smt_one_map_set_same_input(map_id, sv1, sv2);
    } else {
      // map table can only be modified by function call,
      // assume there is no function call in window program eq check
      int max_num = mem_t::map_max_entries(map_id) * mem_t::map_val_sz(map_id) / NUM_BYTE_BITS;
      f = f && smt_array_mem_set_same_input(sv1, sv2, max_num, MEM_TABLE_map, map_id);
    }
  }
  return f;
}

z3::expr smt_map_eq_chk(smt_var& sv1, smt_var& sv2) {
  z3::expr f = string_to_expr("true");
  for (int map_id = 0; map_id < mem_t::maps_number(); map_id++) {
    if (! smt_var::is_win) {
      f = f && smt_one_map_eq_chk(map_id, sv1, sv2);
    } else {
      // map table can only be modified by function call,
      // assume there is no function call in window program eq check
      int max_num = mem_t::map_max_entries(map_id) * mem_t::map_val_sz(map_id) / NUM_BYTE_BITS;
      f = f && smt_array_mem_eq_chk(sv1, sv2, max_num, MEM_TABLE_map, smt_var::is_win, map_id);
    }
  }
  return f;
}

z3::expr smt_pkt_eq_chk(smt_var& sv1, smt_var& sv2, bool is_win) {
  return smt_array_mem_eq_chk(sv1, sv2, mem_t::_layout._pkt_sz, MEM_TABLE_pkt, is_win);
}

z3::expr smt_pgm_mem_eq_chk(smt_var& sv1, smt_var& sv2, bool is_win) {
  z3::expr f = Z3_true;
  f = smt_map_eq_chk(sv1, sv2);
  f = f && smt_pkt_eq_chk(sv1, sv2, is_win);
  f = f && smt_array_mem_eq_chk(sv1, sv2, mem_t::_layout._skb_max_sz, MEM_TABLE_skb, is_win);
  int pgm_input_type = mem_t::get_pgm_input_type();
  if (pgm_input_type == PGM_INPUT_pkt_ptrs) {
    f = f && smt_array_mem_eq_chk(sv1, sv2, 8, MEM_TABLE_pkt_ptrs, is_win);
  }
  if (is_win) {
    f = f && smt_array_mem_eq_chk(sv1, sv2, STACK_SIZE, MEM_TABLE_stack, is_win);
  }
  return f;
}

z3::expr smt_pgm_eq_chk(smt_var& sv1, smt_var& sv2, bool is_win) {
  z3::expr f = smt_pgm_mem_eq_chk(sv1, sv2, is_win);

  smt_output& out1 = sv1.smt_out, out2 = sv2.smt_out;
  // registers equivalence check
  z3::expr same_regs = Z3_true;
  if (! is_win) {
    same_regs = (out1.ret_val == out2.ret_val);
  } else {
    unordered_set<int>& regs_1 = out1.output_var.regs;
    unordered_set<int>& regs_2 = out2.output_var.regs;
    for (int i = 0; i < NUM_REGS; i++) {
      bool is_find1 = (regs_1.find(i) != regs_1.end());
      bool is_find2 = (regs_2.find(i) != regs_2.end());
      if (is_find1 && is_find2) {
        same_regs = same_regs && (out1.reg_expr(i) == out2.reg_expr(i));
      } else if (is_find1 && (! is_find2)) {
        same_regs = same_regs && (out1.reg_expr(i) == smt_input::reg_expr(i));
      } else if ((! is_find1) && is_find2) {
        same_regs = same_regs && (out2.reg_expr(i) == smt_input::reg_expr(i));
      }
    }
  }
  if ( (! out1.pgm_has_tail_call) && (! out2.pgm_has_tail_call) ) {
    f = f && same_regs;
  } else {
    z3::expr same_exit_type = (out1.pgm_exit_type == out2.pgm_exit_type);
    z3::expr same_tail_call_paras = (out1.tail_call_args[0] == out2.tail_call_args[0]) &&
                                    (out1.tail_call_args[1] == out2.tail_call_args[1]) &&
                                    (out1.tail_call_args[2] == out2.tail_call_args[2]);
    f = f && same_exit_type &&
        z3::implies(out1.pgm_exit_type == PGM_EXIT_TYPE_default, same_regs) &&
        z3::implies(out1.pgm_exit_type == PGM_EXIT_TYPE_tail_call, same_tail_call_paras);
  }
  return f;
}

int reg_ptr_type_2_mem_table_type(int reg_type) {
  if (reg_type == PTR_TO_STACK) return MEM_TABLE_stack;
  if (reg_type == PTR_TO_MAP_VALUE) return MEM_TABLE_map;

  int pgm_input_type = mem_t::get_pgm_input_type();

  if (reg_type == PTR_TO_CTX) {
    if (pgm_input_type == PGM_INPUT_pkt_ptrs) {
      return MEM_TABLE_pkt_ptrs;
    } else {
      return MEM_TABLE_pkt;
    }
  }

  if (reg_type == PTR_TO_PKT) {
    if (pgm_input_type == PGM_INPUT_pkt_ptrs) {
      return MEM_TABLE_pkt;
    }
  }

  return -1; // -1 means not found
}

int mem_table_type_2_reg_ptr_type(int mem_table_type) {
  if (mem_table_type == MEM_TABLE_stack) return PTR_TO_STACK;
  if (mem_table_type == MEM_TABLE_map) return PTR_TO_MAP_VALUE;

  int pgm_input_type = mem_t::get_pgm_input_type();

  if (mem_table_type == MEM_TABLE_pkt) {
    if (pgm_input_type == PGM_INPUT_pkt_ptrs) {
      return PTR_TO_PKT;
    } else {
      return PTR_TO_CTX;
    }
  }

  if (mem_table_type == MEM_TABLE_pkt_ptrs) {
    if (pgm_input_type == PGM_INPUT_pkt_ptrs) {
      return PTR_TO_CTX;
    }
  }

  return -1; // -1 means not found
}

bool is_ptr(int type) {
  vector<int> ptr_array = {PTR_TO_STACK, PTR_TO_CTX, PTR_TO_PKT, PTR_TO_MAP_VALUE};
  for (int i = 0; i < ptr_array.size(); i++) {
    if (type == ptr_array[i]) {
      return true;
    }
  }
  return false;
}

z3::expr get_ptr_value_expr(int ptr_type, smt_var& sv, int map_id = -1) {
  int pgm_input_type = mem_t::get_pgm_input_type();
  z3::expr ptr_val_expr = ZERO_ADDR_OFF_EXPR;
  if (ptr_type == PTR_TO_STACK) {
    ptr_val_expr = sv.get_stack_start_addr();
  } else if (ptr_type == PTR_TO_CTX) {
    if (pgm_input_type == PGM_INPUT_pkt_ptrs) {
      ptr_val_expr = sv.get_pkt_start_ptr_addr();
    } else {
      ptr_val_expr = sv.get_pkt_start_addr();
    }
  } else if (ptr_type == PTR_TO_PKT) {
    ptr_val_expr = sv.get_pkt_start_addr();
  } else if (ptr_type == PTR_TO_MAP_VALUE) {
    assert(map_id >= 0);
    assert(map_id < mem_t::maps_number());
    ptr_val_expr = sv.get_map_start_addr(map_id);
  } else {
    // error here
    string err_msg = "Error: no pointer type matches";
    throw (err_msg);
  }
  return ptr_val_expr;
}

// set up sv.mem._stack_ptr_table only for pointers on the stack read by program
void smt_pgm_set_pre_stack_state_table(smt_var& sv, smt_input& input) {
  auto it_stack = input.prog_read.mem.find(PTR_TO_STACK);
  int stack_table_id = sv.mem_var.get_mem_table_id(MEM_TABLE_stack);
  // if there is no stack read, there is no need to set input of pointers stored on the stack
  if (it_stack != input.prog_read.mem.end()) {
    for (auto it = smt_input::stack_state.begin(); it != smt_input::stack_state.end(); it++) {
      int stack_off = it->first;
      // check whether stack_state is in prog_read
      assert(stack_off >= 0);
      const int ptr_size = 8;
      assert(stack_off <= STACK_SIZE - ptr_size);
      bool ptr_in_prog_read = true;
      for (int off_i = 0; off_i < ptr_size; off_i++) {
        if (it_stack->second.find(stack_off + off_i) == it_stack->second.end()) {
          ptr_in_prog_read = false;
          break;
        }
      }
      // if ptr not in prog read, check next ptr
      if (! ptr_in_prog_read) continue;

      vector<register_state>& states = it->second;
      for (int i = 0; i < states.size(); i++) {
        // add the ptr in the ptr table and add the memory value/pointer in the stack memory urt
        int ptr_type = states[i].type;
        int map_id = states[i].map_id;
        int ptr_off = states[i].off;
        z3::expr ptr_off_expr = to_expr((uint64_t)ptr_off);
        int table_type = reg_ptr_type_2_mem_table_type(ptr_type);
        assert(table_type != -1);
        int table_id = sv.mem_var.get_mem_table_id(table_type, map_id);
        assert(table_id != -1);
        z3::expr pc = input.ptr_on_stack_path_cond(stack_off, i);
        z3::expr stack_off_name_expr = smt_input::ptr_on_stack_expr(stack_off);
        // add pointer info in the pointer table
        sv.mem_var.add_ptr(stack_off_name_expr, table_id, ptr_off_expr, pc);

        // get value expression according to ptr_type and ptr_off
        z3::expr ptr_val_expr = Z3_true;
        z3::expr ptr_value = get_ptr_value_expr(ptr_type, sv, map_id) + ptr_off_expr;

        bool is_ptr = true;
        int block = 0;  // set as root block
        z3::expr is_valid = pc;  // set is_valid as path condition
        int pointer_sz = 8;
        // push each off-val/ptr entry into stack memory urt
        for (int v_id = 0; v_id < pointer_sz; v_id++) {
          mem_table_ptr_info ptr_info(is_ptr, stack_off_name_expr, v_id);
          z3::expr val_expr = ptr_value.extract(8 * v_id + 7, 8 * v_id);
          sv.mem_var._mem_tables[stack_table_id]._urt.add(block, is_valid,
              to_expr((uint64_t)(stack_off + v_id)), val_expr, ptr_info);
        }
      }
    }
  }
}

// Generate the precondition formula and set up the pointer registers,
// sv is the sv of program's root basic block
z3::expr smt_pgm_set_pre(smt_var& sv, smt_input& input) {
  // cout << "smt_pgm_set_pre" << endl;
  z3::expr f = Z3_true;
  // set up pointer registers only for registers read by program
  for (auto reg : input.prog_read.regs) {
    // get all possible register states from iss.reg_state
    z3::expr reg_expr = sv.get_init_reg_var(reg);
    // cout << "reg " << reg << ":" << reg_expr << endl;;
    f = f && (reg_expr == smt_input::reg_expr(reg));
    for (int i = 0; i < input.reg_state[reg].size(); i++) {
      register_state reg_state = input.reg_state[reg][i];
      if (! is_ptr(reg_state.type)) continue;
      z3::expr pc = input.reg_path_cond(reg, i);
      int table_type = reg_ptr_type_2_mem_table_type(reg_state.type);
      assert(table_type != -1);
      int table_id = sv.mem_var.get_mem_table_id(table_type, reg_state.map_id);
      assert(table_id != -1);
      z3::expr off_expr = to_expr(reg_state.off);
      sv.mem_var.add_ptr(reg_expr, table_id, off_expr, pc);
      z3::expr ptr_val_expr = get_ptr_value_expr(reg_state.type, sv, reg_state.map_id) + off_expr;
      f = f && z3::implies(pc, smt_input::reg_expr(reg) == ptr_val_expr);
      // cout << reg_expr << " " << table_id << " " << off_expr << endl;
    }
  }

  smt_pgm_set_pre_stack_state_table(sv, input);
  f = f && input.input_constraint();
  f = f && sv.mem_layout_constrain();
  return f;
}

// add the constraints on the input equivalence setting
// the same content in the same memory address in mem_p1 URT == mem_p2 URT
// todo: need to add the legal offset range check?
z3::expr smt_array_mem_set_same_input(smt_var& sv1, smt_var& sv2, int mem_sz, int mem_table_type, int map_id) {
  if (mem_sz == 0) return Z3_true;
  int id1 = sv1.mem_var.get_mem_table_id(mem_table_type, map_id);
  int id2 = sv2.mem_var.get_mem_table_id(mem_table_type, map_id);
  assert(id1 != -1);
  assert(id2 != -1);
  z3::expr f = Z3_true;
  smt_wt& mem1_urt = sv1.mem_var._mem_tables[id1]._urt;
  smt_wt& mem2_urt = sv2.mem_var._mem_tables[id2]._urt;

  z3::expr mem_s = sv1.get_mem_start(mem_table_type, map_id);
  z3::expr mem_e = sv1.get_mem_end(mem_table_type, map_id);

  bool cond = (mem1_urt.size() > 0) && (mem2_urt.size() > 0);
  if (!cond) return f;

  for (int i = 0; i < mem1_urt.size(); i++) {
    z3::expr iv1 = mem1_urt.is_valid[i];
    z3::expr a1 = mem1_urt.addr[i];
    z3::expr v1 = mem1_urt.val[i];
    z3::expr a1_in_mem = Z3_true;
    if (! smt_var::enable_multi_mem_tables) {
      a1_in_mem = addr_in_mem_range(a1, mem_s, mem_e);
    }

    for (int j = 0; j < mem2_urt.size(); j++) {
      z3::expr iv2 = mem2_urt.is_valid[j];
      z3::expr a2 = mem2_urt.addr[j];
      z3::expr v2 = mem2_urt.val[j];

      f = f && z3::implies(a1_in_mem && iv1 && iv2 && (a1 == a2), v1 == v2);
    }
  }
  return f;
}

z3::expr smt_pkt_set_same_input(smt_var& sv1, smt_var& sv2) {
  return smt_array_mem_set_same_input(sv1, sv2, mem_t::_layout._pkt_sz, MEM_TABLE_pkt);
}

z3::expr smt_pgm_set_same_input(smt_var& sv1, smt_var& sv2, bool is_win) {
  z3::expr f = smt_map_set_same_input(sv1, sv2);
  f = f && smt_pkt_set_same_input(sv1, sv2);  // pkt
  f = f && smt_array_mem_set_same_input(sv1, sv2, mem_t::_layout._skb_max_sz, MEM_TABLE_skb); // skb
  int pgm_input_type = mem_t::get_pgm_input_type();
  if (pgm_input_type == PGM_INPUT_pkt_ptrs) {
    f = f && smt_array_mem_set_same_input(sv1, sv2, 8, MEM_TABLE_pkt_ptrs);
  }
  if (is_win) {
    f = f && smt_array_mem_set_same_input(sv1, sv2, STACK_SIZE, MEM_TABLE_stack);
  }
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
                                     smt_var& sv, unsigned int block, bool enable_addr_off, bool is_win) {
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
    z3::expr f = predicate_ld_n_bytes(k_sz / NUM_BYTE_BITS, addr_k, sv, k, block,
                                      map_id_path_conds[i], enable_addr_off, is_win);

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
    z3::expr addr_k, z3::expr addr_v, z3::expr out, smt_var& sv, unsigned int block, bool enable_addr_off, bool is_win) {
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
               predicate_ld_n_bytes(k_sz / NUM_BYTE_BITS, addr_k, sv, k, block, map_id_path_cond, enable_addr_off, is_win) &&
               predicate_ld_n_bytes(v_sz / NUM_BYTE_BITS, addr_v, sv, v, block, map_id_path_cond, enable_addr_off, is_win);
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
  f_ret = f_ret && predicate_st_n_bytes(v_sz / NUM_BYTE_BITS, v, addr_map_v, sv, block,
                                        map_id_path_cond, enable_addr_off);

  return f_ret;
}

// "out" is the return value
z3::expr predicate_map_update_helper(z3::expr addr_map, z3::expr addr_k, z3::expr addr_v,
                                     z3::expr out, smt_var& sv, unsigned int block,
                                     bool enable_addr_off, bool is_win) {
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
            addr_k, addr_v, out, sv, block, enable_addr_off, is_win);
  }
  return f_ret;
}

// "out" is the return value
// if key not in the map, out = 0xfffffffe, else out = 0
z3::expr predicate_map_delete_helper_for_one_map(int map_id, z3::expr map_id_path_cond,
    z3::expr addr_k, z3::expr out, smt_var& sv, unsigned int block, bool enable_addr_off, bool is_win) {
  z3::expr f_ret = Z3_true;
  smt_mem& mem = sv.mem_var;
  int k_sz = mem_t::map_key_sz(map_id);
  z3::expr k = sv.update_key(k_sz);
  /* add the constrains on "k" */
  z3::expr f = predicate_ld_n_bytes(k_sz / NUM_BYTE_BITS, addr_k, sv, k, block, map_id_path_cond, enable_addr_off, is_win);

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
                                     smt_var& sv, unsigned int block, bool enable_addr_off, bool is_win) {
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
            addr_k, out, sv, block, enable_addr_off, is_win);
  }
  return f_ret;
}

z3::expr predicate_helper_function(int func_id, z3::expr r1, z3::expr r2, z3::expr r3,
                                   z3::expr r4, z3::expr r5, z3::expr r0,
                                   smt_var& sv, unsigned int block, bool enable_addr_off, bool is_win) {
  if (func_id == BPF_FUNC_map_lookup_elem) {
    return predicate_map_lookup_helper(r1, r2, r0, sv, block, enable_addr_off, is_win);
  } else if (func_id == BPF_FUNC_map_update_elem) {
    return predicate_map_update_helper(r1, r2, r3, r0, sv, block, enable_addr_off, is_win);
  } else if (func_id == BPF_FUNC_map_delete_elem) {
    return predicate_map_delete_helper(r1, r2, r0, sv, block, enable_addr_off, is_win);
  } else if (func_id == BPF_FUNC_get_prandom_u32) {
    return predicate_get_prandom_u32_helper(r0, sv);
  } else {
    cout << "Error: unknown function id " << func_id << endl; return string_to_expr("true");
  }
}

// out is the return value (random_u32)
z3::expr predicate_get_prandom_u32_helper(z3::expr out, smt_var& sv) {
  // higher 32-bit of out is 0, lower 32-bit of out is a symbolic value
  z3::expr f = (out == z3::concat(to_expr((int32_t)0, 32), sv.get_next_random_u32()));
  return f;
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
uint64_t get_uint64_from_bv64(z3::expr & z3_val, bool assert) {
  z3::expr x = z3_val.simplify();
  bool is_num = x.is_numeral();
  if (is_num) return x.get_numeral_uint64();
  if (assert) {
    assert(false);
  } else {
    return 0;
  }
}

// get an addr-val list for the given memory table,
// if the null_addr_chk is true, the NULL value address in the memory table entry will be ignored
void get_mem_from_mdl(vector<pair<uint64_t, uint8_t>>& mem_addr_val,
                      z3::model & mdl, smt_var& sv, int mem_id,
                      bool null_addr_chk = true) {
  smt_wt& mem_urt = sv.mem_var._mem_tables[mem_id]._urt;
  for (int i = 0; i < mem_urt.size(); i++) {
    z3::expr z3_is_valid = mem_urt.is_valid[i];
    int is_valid = mdl.eval(z3_is_valid).bool_value();
    if (is_valid != 1) continue; // -1 means z3 false, 1 means z3 true, 0: z3 const (not know it is true or false)

    z3::expr z3_addr = mem_urt.addr[i];
    z3::expr z3_addr_eval = mdl.eval(z3_addr);
    if (!z3_addr_eval.is_numeral()) continue;
    uint64_t addr = z3_addr_eval.get_numeral_uint64();
    // addr whose value is NULL means the entry is invalid
    if ((addr == NULL_ADDR) && null_addr_chk) continue;

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
      v[i] = unidist_codegen(gen_codegen) * (double)0xff; // uint8_t: 0 - 0xff
    }
  }
}

void counterex_urt_2_input_map(inout_t& input, z3::model & mdl, smt_var& sv, int mem_id, int map_id) {
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

void counterex_urt_2_input_map_mem_win(inout_t& input, z3::model & mdl, smt_var& sv, int mem_id, int map_id) {
  vector<pair< uint64_t, uint8_t>> mem_addr_val;
  bool stack_null_off_chk = false; // map memory for win prog eq check is offset-record in the table
  get_mem_from_mdl(mem_addr_val, mdl, sv, mem_id, stack_null_off_chk);
  assert(map_id < input.maps_mem.size());
  int map_sz = input.maps_mem[map_id].size();

  for (int i = 0; i < mem_addr_val.size(); i++) {
    uint64_t off = mem_addr_val[i].first;
    uint8_t val = mem_addr_val[i].second;
    // todo: add the map value off check in the safety check
    if (off >= map_sz) {
      string err_msg = "off >= map_sz";
      throw err_msg;
    }
    // assert(off < map_sz);
    input.maps_mem[map_id][off] = val;
  }
}

// set input memory, for now, set pkt, stack(for window program eq check)
// 1. get mem_addr_val list according to the pkt mem urt;
// 2. traverse mem_addr_val list, if the addr is in input memory address range, update "input"
void counterex_urt_2_input_mem(inout_t& input, z3::model & mdl, smt_var& sv, smt_input& sin, uint64_t pkt_s) {
  int pkt_sz = mem_t::_layout._pkt_sz;
  if (pkt_sz > 0) {
    int pkt_mem_id = sv.mem_var.get_mem_table_id(MEM_TABLE_pkt);
    assert(pkt_mem_id != -1);
    vector<pair< uint64_t, uint8_t>> mem_addr_val;
    if (smt_var::enable_addr_off) {
      bool pkt_null_off_chk = false; // since pkt is offset-record in the table
      get_mem_from_mdl(mem_addr_val, mdl, sv, pkt_mem_id, pkt_null_off_chk);
    } else {
      bool pkt_null_off_chk = true;
      get_mem_from_mdl(mem_addr_val, mdl, sv, pkt_mem_id, pkt_null_off_chk);
    }

    for (int i = 0; i < mem_addr_val.size(); i++) {
      uint64_t off;
      if (smt_var::enable_addr_off) off = mem_addr_val[i].first;
      else off = mem_addr_val[i].first - pkt_s;

      uint8_t val = mem_addr_val[i].second;
      assert(off < pkt_sz);
      input.pkt[off] = val;
    }
  }
  if (smt_var::is_win) { // update stack value
    auto it = sin.prog_read.mem.find(PTR_TO_STACK);
    if (it != sin.prog_read.mem.end()) {
      for (auto off : it->second) {
        input.stack_readble[off] = true;
        input.stack[off] = unidist_codegen(gen_codegen) * 0x100; // [0 - 0xff]
      }
    }

    int mem_id = sv.mem_var.get_mem_table_id(MEM_TABLE_stack);
    assert(mem_id != -1);
    vector<pair< uint64_t, uint8_t>> mem_addr_val;
    bool stack_null_off_chk = false; // since stack is offset-record in the table
    get_mem_from_mdl(mem_addr_val, mdl, sv, mem_id, stack_null_off_chk);

    for (int i = 0; i < mem_addr_val.size(); i++) {
      uint64_t off = mem_addr_val[i].first;
      uint8_t val = mem_addr_val[i].second;
      assert(off < STACK_SIZE);
      input.stack[off] = val;
    }

  }
}

void counterex_2_input_simu_pkt_s(inout_t& input, z3::model& mdl, smt_var& sv) {
  z3::expr z3_pkt_s = mdl.eval(sv.get_pkt_start_addr());
  input.input_simu_pkt_s = get_uint64_from_bv64(z3_pkt_s, false); // r10: stack bottom
  if (input.input_simu_pkt_s == 0) {// means z3 does not care about r10, assign a random value
    // 0x10000000 is to make sure r10 >> stack_s
    input.input_simu_pkt_s = 0x10000000 + unidist_codegen(gen_codegen) * (double)0xffff;
  }
}

void counterex_2_input_simu_r10(inout_t& input, z3::model & mdl, smt_var& sv) {
  z3::expr z3_stack_bottom = mdl.eval(sv.get_stack_bottom_addr());
  input.input_simu_r10 = get_uint64_from_bv64(z3_stack_bottom, false); // r10: stack bottom
  if (input.input_simu_r10 == 0) {// means z3 does not care about r10, assign a random value
    // 0x10000 is to make sure r10 > 512
    input.input_simu_r10 = 0x10000 + unidist_codegen(gen_codegen) * (double)0xffff;
  }
}

void counterex_2_input_simu_pkt_ptrs(inout_t& input, z3::model & mdl, smt_var& sv) {
  if (mem_t::get_pgm_input_type() == PGM_INPUT_pkt_ptrs) {
    z3::expr z3_pkt_ptrs_s = mdl.eval(sv.get_pkt_start_ptr_addr());
    input.input_simu_pkt_ptrs_s = get_uint64_from_bv64(z3_pkt_ptrs_s, true);
    z3::expr z3_pkt_start = mdl.eval(sv.get_pkt_start_addr());
    input.input_simu_pkt_ptrs[0] = (uint32_t)get_uint64_from_bv64(z3_pkt_start, true);
    z3::expr z3_pkt_end = mdl.eval(sv.get_pkt_end_addr());
    input.input_simu_pkt_ptrs[1] = (uint32_t)get_uint64_from_bv64(z3_pkt_end, true);
  }
}

void counterex_2_input_randoms_u32(inout_t& input, z3::model & mdl, smt_var& sv) {
  input.set_randoms_u32();
  for (int i = 0; i < smt_var::randoms_u32.size(); i++) {
    z3::expr z3_rand_u32 = mdl.eval(smt_var::randoms_u32[i]);
    if (! z3_rand_u32.is_numeral()) continue;
    uint32_t rand_u32 = z3_rand_u32.get_numeral_uint64();
    input.randoms_u32[i] = rand_u32;
  }
}

void counterex_urt_2_input_mem_for_one_sv(inout_t& input, z3::model & mdl, smt_var& sv, smt_input& sin) {
  counterex_2_input_simu_pkt_s(input, mdl, sv);
  counterex_2_input_simu_r10(input, mdl, sv);
  counterex_2_input_simu_pkt_ptrs(input, mdl, sv);
  counterex_2_input_randoms_u32(input, mdl, sv);
  counterex_urt_2_input_mem(input, mdl, sv, sin, input.input_simu_pkt_s);
  for (int i = 0; i < mem_t::maps_number(); i++) {
    int mem_id = sv.mem_var.get_mem_table_id(MEM_TABLE_map, i);
    assert(mem_id != -1);
    if (! smt_var::is_win) {
      counterex_urt_2_input_map(input, mdl, sv, mem_id, i);
    } else {
      counterex_urt_2_input_map_mem_win(input, mdl, sv, mem_id, i);
    }
  }
}

void counterex_2_input_regs(inout_t& input, z3::model& mdl, smt_var& sv, smt_input& sin) {
  if (! smt_var::is_win) return;
  vector<vector<register_state>>& pre_regs = smt_input::reg_state;
  for (auto reg : sin.prog_read.regs) {
    // update register readable state
    input.reg_readable[reg] = true;
    // update register value
    z3::expr r_expr = smt_input::reg_expr(reg);
    z3::expr r_val = mdl.eval(r_expr);
    if (r_val.is_numeral()) { // if not is_numeral,  mean Z3 does not care about this value
      input.regs[reg] = (int64_t)r_val.get_numeral_uint64();
    }
    // update register type
    bool pc_in_mdl = false;
    for (int i = 0; i < pre_regs[reg].size(); i++) {
      z3::expr pc_expr = smt_input::reg_path_cond(reg, i);
      z3::expr pc_val = mdl.eval(pc_expr);
      int pc = mdl.eval(pc_val).bool_value();
      if (pc != 1) continue; // -1 means z3 false, 1 means z3 true, 0: z3 const (not know it is true or false)
      input.reg_type[reg] = pre_regs[reg][i].type;
      pc_in_mdl = true;
      break;
    }
    if (! pc_in_mdl) {
      assert(pre_regs[reg].size() > 0);
      input.reg_type[reg] = pre_regs[reg][0].type;
    }
  }
}

// make sure sv1 is for the original program
void counterex_2_input_mem(inout_t& input, z3::model & mdl,
                           smt_var& sv1, smt_var& sv2,
                           smt_input& sin1, smt_input& sin2) {
  input.clear();
  input.init();
  input.set_pkt_random_val();
  // update input memory for executing path condition later
  counterex_urt_2_input_mem_for_one_sv(input, mdl, sv2, sin2);
  // update sv1[sv1_id] finally, the same update before will be overwritten
  counterex_urt_2_input_mem_for_one_sv(input, mdl, sv1, sin1);
  if (smt_var::is_win) {
    counterex_2_input_regs(input, mdl, sv2, sin2);
    counterex_2_input_regs(input, mdl, sv1, sin1);
  }
}

// make sure sv1 is for the original program
void counterex_2_input_mem(inout_t& input, z3::model & mdl, smt_var& sv, smt_input& sin) {
  input.clear();
  input.init();
  input.set_pkt_random_val();
  counterex_urt_2_input_mem_for_one_sv(input, mdl, sv, sin);
  counterex_2_input_regs(input, mdl, sv, sin);
}


// safety check related functions
// size > 0
z3::expr stack_safety_chk(z3::expr addr_off, int size, smt_var& sv) {
  // check if addr is a stack pointer, if so, addr_off should be aligned
  // srem: signed remainder operator for bitvectors
  z3::expr f = (z3::srem(addr_off, size) == 0);
  // [addr_off, addr_off + size - 1] should within [0, 512)
  // sle(a, b): a s<= b; slt(a, b): a s< b
  z3::expr addr_off_max = addr_off + size - 1;
  f = f && sle(0, addr_off) && slt(addr_off, STACK_SIZE);
  f = f && sle(0, addr_off_max) && slt(addr_off_max, STACK_SIZE);
  return f;
}

z3::expr safety_chk_ldx(z3::expr addr, z3::expr off, int size, smt_var& sv, bool enable_addr_off) {
  z3::expr f = Z3_true;
  vector<int> ids;
  vector<vector<mem_ptr_info>> info_list;
  sv.mem_var.get_mem_ptr_info(ids, info_list, addr);
  if (ids.size() == 0) {string s = "error!!!"; throw (s); return Z3_true;} // todo: addr is not a pointer
  for (int i = 0; i < ids.size(); i++) {
    if (ids[i] == sv.mem_var.get_mem_table_id(MEM_TABLE_stack)) {
      for (int j = 0; j < info_list[i].size(); j++) {
        z3::expr addr_off = off + info_list[i][j].off;
        if (! enable_addr_off) {
          addr_off = addr + off - sv.get_stack_bottom_addr();
        }
        z3::expr stack_chk = stack_safety_chk(addr_off, size, sv);
        f = f && z3::implies(info_list[i][j].path_cond, stack_chk);
      }
    }
  }
  return f;
}

z3::expr safety_chk_stx(z3::expr addr, z3::expr off, int size, smt_var& sv, bool enable_addr_off) {
  z3::expr f = Z3_true;
  vector<int> ids;
  vector<vector<mem_ptr_info>> info_list;
  sv.mem_var.get_mem_ptr_info(ids, info_list, addr);
  if (ids.size() == 0) {string s = "error!!!"; throw (s); return Z3_true;} // todo: addr is not a pointer
  for (int i = 0; i < ids.size(); i++) {
    if (ids[i] == sv.mem_var.get_mem_table_id(MEM_TABLE_stack)) {
      for (int j = 0; j < info_list[i].size(); j++) {
        z3::expr addr_off = off + info_list[i][j].off;
        if (! enable_addr_off) {
          addr_off = addr + off - sv.get_stack_bottom_addr();
        }
        z3::expr stack_chk = stack_safety_chk(addr_off, size, sv);
        f = f && z3::implies(info_list[i][j].path_cond, stack_chk);
      }
    }
  }
  return f;
}

z3::expr safety_chk_st(z3::expr addr, z3::expr off, int size, smt_var& sv, bool enable_addr_off) {
  z3::expr f = Z3_true;
  vector<int> ids;
  vector<vector<mem_ptr_info>> info_list;
  sv.mem_var.get_mem_ptr_info(ids, info_list, addr);
  if (ids.size() == 0) {string s = "error!!!"; throw (s); return Z3_true;} // todo: addr is not a pointer
  for (int i = 0; i < ids.size(); i++) {
    if (ids[i] == sv.mem_var.get_mem_table_id(MEM_TABLE_stack)) {
      for (int j = 0; j < info_list[i].size(); j++) {
        z3::expr addr_off = off + info_list[i][j].off;
        if (! enable_addr_off) {
          addr_off = addr + off - sv.get_stack_bottom_addr();
        }      z3::expr stack_chk = stack_safety_chk(addr_off, size, sv);
        f = f && z3::implies(info_list[i][j].path_cond, stack_chk);
      }
    }
  }
  return f;
}
