#pragma once

#include <iomanip>
#include <iostream>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <queue>
#include <string.h>
#include "../../../src/utils.h"
#include "../../../src/isa/inst_var.h"

using namespace std;

#define STACK_SIZE 512 // 512 bytes
#define NULL_ADDR 0
#define NULL_ADDR_EXPR to_expr(NULL_ADDR)
#define ZERO_ADDR_OFF_EXPR to_expr((int64_t)0)
static constexpr int NUM_REGS = 11;

struct map_attr { // map attribute
  unsigned int key_sz;
  unsigned int val_sz;
  unsigned int max_entries;
  map_attr(unsigned int k_sz, unsigned int v_sz, unsigned int n_entries = 0) {
    key_sz = k_sz; val_sz = v_sz; max_entries = n_entries;
  }
};
ostream& operator<<(ostream& out, const map_attr& m_attr);

enum pgm_input_type {
  PGM_INPUT_constant,
  PGM_INPUT_pkt, // input(r1) -> pkt_start_addr
  PGM_INPUT_pkt_ptrs, // input(r1) -> {pkt_start_addr(32-bit), pkt_end_addr(32-bit)}
  PGM_INPUT_skb,
};

class mem_layout {
 public:
  int _pgm_input_type = PGM_INPUT_constant;
  unsigned int _stack_start = 0;
  vector<map_attr> _maps_attr;
  vector<unsigned int> _maps_start;
  unsigned int _pkt_sz = 0; // means no pkt, unit: byte
  void clear() {_maps_attr.clear(); _maps_start.clear(); _pkt_sz = 0;}
  friend ostream& operator<<(ostream& out, const mem_layout& layout);
};

class map_t {
 private:
  vector<bool> _idx_used; // idxes have been used before
 public:
  // map: key to array index i in map range, i starts from 0, the max of i is (_max_entries - 1)
  unordered_map<string, unsigned int> _k2idx;
  // next available idx for each map, starts from empty queue,
  // map delete may push an item in the queue
  queue<unsigned int> _available_idx_q;
  // maximum number of entries in map from the beginning to now
  unsigned int _cur_max_entries = 0;
  unsigned int _max_entries;
  map_t(unsigned int max_entries) {
    _max_entries = max_entries;
    _idx_used.resize(_max_entries, false);
  }
  unsigned int get_and_update_next_idx();
  void add_available_idx(unsigned int idx);
  void clear();
  bool operator==(const map_t &rhs);
  friend ostream& operator<<(ostream& out, const map_t& mp);
};

class mem_t {
 public:
  int _mem_size; // size unit: byte
  // should ensure memory is contiguous, because of the assumption in memory_access_check
  uint8_t *_mem = nullptr;
  uint8_t *_pkt = nullptr;
  vector<map_t> _maps;
  static mem_layout _layout;
  uint64_t _simu_mem_s = 0;
  uint64_t _simu_pkt_s = 0; // used when pgm input type is PGM_INPUT_pkt
  uint64_t _simu_pkt_ptrs_s = 0; // used when pgm input type is PGM_INPUT_pkt_ptrs
  uint32_t _pkt_ptrs[2] = {0, 0}; // 0: pkt_start_ptr, 1: pkt_end_ptr; these are simulated pkt addresses (32-bit)
  mem_t();
  ~mem_t();
  static void add_map(map_attr m_attr);
  static void set_pkt_sz(unsigned int sz) {_layout._pkt_sz = sz;}
  static void set_pgm_input_type(int type) {_layout._pgm_input_type = type;}
  static int get_pgm_input_type() {return _layout._pgm_input_type;}
  static unsigned int maps_number() {return _layout._maps_attr.size();}
  static unsigned int map_key_sz(int map_id);
  static unsigned int map_val_sz(int map_id);
  static unsigned int map_max_entries(int map_id);
  // 1. compute "_mem_size" and according to "_layout";
  // 2. allocate memory for "_mem", "_pkt"
  // 3. init _maps
  // 4. init _pkt_ptrs
  void init_by_layout();
  static void set_map_attr(int map_id, map_attr m_attr);
  static unsigned int get_mem_off_by_idx_in_map(int map_id, unsigned int idx_in_map);
  void update_kv_in_map(int map_id, string k, const uint8_t* addr_v); // get v_sz from layout
  void update_kv_in_map(int map_id, string k, const vector<uint8_t>& v);
  uint8_t* get_stack_start_addr() const;
  // designed for r10
  uint8_t* get_stack_bottom_addr() const;
  uint8_t* get_mem_start_addr() const;
  uint8_t* get_mem_end_addr() const;
  uint8_t* get_pkt_start_addr() const;
  uint8_t* get_pkt_end_addr() const;
  uint8_t* get_pkt_addr_by_offset(int off) const;
  uint32_t* get_pkt_ptrs_start_addr();
  uint32_t* get_pkt_ptrs_end_addr();
  uint64_t get_simu_mem_start_addr();
  uint64_t get_simu_mem_end_addr();
  uint64_t get_simu_pkt_start_addr();
  uint64_t get_simu_pkt_end_addr();
  uint64_t get_simu_pkt_ptrs_start_addr();
  uint64_t get_simu_pkt_ptrs_end_addr();
  mem_t& operator=(const mem_t &rhs);
  bool operator==(const mem_t &rhs);
  void clear();
  friend ostream& operator<<(ostream& out, const mem_t& mem);
};

extern mem_layout _layout;

class smt_wt {
 private:
  bool is_equal(z3::expr e1, z3::expr e2);
 public:
  vector<unsigned int> block;
  vector<z3::expr> is_valid; // flag, indicate whether this entry is valid or not
  vector<z3::expr> addr; // 64-bit bitvector, addr now is an offset
  vector<z3::expr> val;  // 8-bit bitvector
  void add(unsigned int b, z3::expr iv, z3::expr a, z3::expr v) {
    block.push_back(b); is_valid.push_back(iv); addr.push_back(a.simplify()); val.push_back(v);
    // cout << "mem add entry: " << b << " " << iv << " " << a.simplify() << " " << v.simplify() << endl;
  }
  void clear() {block.clear(); is_valid.clear(); addr.clear(); val.clear();}
  unsigned int size() {return addr.size();}
  smt_wt& operator=(const smt_wt &rhs);
  bool operator==(const smt_wt &rhs);
  friend ostream& operator<<(ostream& out, const smt_wt& s);
};

class smt_map_wt {
 public:
  vector<unsigned int> block;
  vector<z3::expr> is_valid; // flag, indicate whether this entry is valid or not
  vector<z3::expr> key;
  vector<z3::expr> addr_v;
  void add(unsigned int b, z3::expr iv, z3::expr k, z3::expr v) {
    block.push_back(b);
    is_valid.push_back(iv);
    key.push_back(k);
    addr_v.push_back(v);
    // cout << "map add entry: " << b << " " << iv << " " << k << " " << v.simplify() << endl;
  }
  void clear() {block.clear(); is_valid.clear(); key.clear(); addr_v.clear();}
  unsigned int size() {return key.size();}
  friend ostream& operator<<(ostream& out, const smt_map_wt& s);
};

class map_wt {
 public:
  smt_map_wt _wt;
  smt_map_wt _urt;
  void clear() {_wt.clear(); _urt.clear();}
};

enum mem_table_type {
  MEM_TABLE_stack = 0,
  MEM_TABLE_pkt_ptrs,
  MEM_TABLE_pkt,
  MEM_TABLE_map,
};

class mem_ptr_info {
 public:
  z3::expr path_cond = Z3_false;
  z3::expr off = ZERO_ADDR_OFF_EXPR; // the default off is 0
  mem_ptr_info(z3::expr pc = Z3_false, z3::expr o = ZERO_ADDR_OFF_EXPR) {
    path_cond = pc.simplify(); off = o.simplify();
  }
};

class mem_table {
 public:
  int _type = -1;
  int _map_id = -1; // valid when _type == MEM_TABLE_map
  // ptr and its path condition;
  // only use vector<z3::expr>[0]; z3::expr does not have a constructor without input value
  // that is unordered_map<unsigned int, z3::expr> is not allowed
  unordered_map<unsigned int, mem_ptr_info> _ptrs;
  smt_wt _wt;
  smt_wt _urt;
  void clear() {_ptrs.clear(); _wt.clear(); _urt.clear();}
  void add_ptr(z3::expr ptr_expr, z3::expr path_cond, z3::expr off) {
    // cout << "add_ptr: ptr:" << ptr_expr << ", off:" << off.simplify() << ", \npc:" << path_cond.simplify() << endl;
    auto found = _ptrs.find(ptr_expr.id());
    if (found == _ptrs.end()) _ptrs.insert({ptr_expr.id(), mem_ptr_info(path_cond, off)});
    else {
      found->second.path_cond = path_cond;
      found->second.off = off;
    }
  }
  bool is_ptr_in_ptrs(z3::expr ptr_expr) {
    auto found = _ptrs.find(ptr_expr.id());
    if (found == _ptrs.end()) return false;
    else return true;
  }
  mem_ptr_info get_ptr_info(z3::expr ptr_expr) {
    auto found = _ptrs.find(ptr_expr.id());
    if (found == _ptrs.end()) return mem_ptr_info(Z3_false, ZERO_ADDR_OFF_EXPR);
    else return found->second;
  }
  friend ostream& operator<<(ostream& out, const mem_table& mt);
};

class smt_mem {
 public:
  z3::expr _stack_start = string_to_expr("stack_start");
  z3::expr _pkt_start = string_to_expr("pkt_start");
  z3::expr _pkt_off = string_to_expr("pkt_off"); // pkt_off = pkt_sz - 1
  z3::expr _pkt_start_ptr_addr = string_to_expr("pkt_start_ptr_addr");
  vector<mem_table> _mem_tables; // stack, pkt, map related memory
  vector<map_wt> _map_tables; // vector idx: map id
  vector<z3::expr> _addrs_map_v_next;
  vector<z3::expr> _path_cond_list;

  smt_mem() {}
  smt_mem(uint64_t stack_start) {_stack_start = to_expr(stack_start);}
  void set_stack_start(uint64_t stack_start) {_stack_start = to_expr(stack_start);}
  void init_by_layout(unsigned int n_blocks = 1);
  z3::expr get_and_update_addr_v_next(int map_id);
  void clear() {_mem_tables.clear(); _map_tables.clear(); _addrs_map_v_next.clear(); _path_cond_list.clear();}
  // return <table_id, block id>, table_id == -1 means not found
  void get_mem_ptr_info(vector<int>& table_ids, vector<mem_ptr_info>& ptr_info_list, z3::expr ptr_expr);
  int get_mem_table_id(int type, int map_id = -1);
  int get_type(int mem_table_id);
  void add_in_mem_table_wt(int mem_table_id, unsigned int block, z3::expr is_valid, z3::expr addr, z3::expr val);
  void add_in_mem_table_urt(int mem_table_id, unsigned int block, z3::expr is_valid, z3::expr addr, z3::expr val);
  void add_ptr(z3::expr ptr_expr, int table_id, z3::expr off = ZERO_ADDR_OFF_EXPR, z3::expr path_cond = Z3_true);
  // called by smt_inst()
  void add_ptr(z3::expr ptr_expr, z3::expr ptr_from_expr, z3::expr ptr_minus_ptr_from, z3::expr path_cond = Z3_true);
  void add_ptr_by_map_id(z3::expr ptr_expr, int map_id, z3::expr path_cond = Z3_true);
  z3::expr get_block_path_cond(unsigned int block_id);
  void add_path_cond(z3::expr pc, unsigned int block_id);
  friend ostream& operator<<(ostream& out, const smt_mem& s);
};

struct map_id_pc {
  int map_id;
  z3::expr path_cond = Z3_false;
  map_id_pc(int id, z3::expr pc) {map_id = id; path_cond = pc;}
};

// SMT Variable format
// register: r_[prog_id]_[node_id]_[reg_id]_[version_id]
// map key: k_[prog_id]_[node_id]_[version_id]
// map value: v_[prog_id]_[node_id]_[version_id]
// map address of value: av_[prog_id]_[node_id]_[version_id]
class smt_var: public smt_var_base {
 private:
  unsigned int mem_addr_id, is_vaild_id, key_cur_id,
           val_cur_id, addr_v_cur_id, map_helper_func_ret_cur_id,
           var_cur_id;
  // key: expr id; value: a list of <map_id, path_cond of map_id>
  // program global variable
  unordered_map<unsigned int, vector<map_id_pc>> expr_map_id;
 public:
  smt_mem mem_var;
  smt_var();
  // 1. Convert prog_id and node_id into _name, that is string([prog_id]_[node_id])
  // 2. Initialize reg_val[i] = r_[_name]_0, i = 0, ..., num_regs
  smt_var(unsigned int prog_id, unsigned int node_id, unsigned int num_regs);
  ~smt_var();
  // map related functions
  z3::expr update_mem_addr();
  z3::expr update_is_valid();
  z3::expr update_key(unsigned int k_sz = NUM_BYTE_BITS);
  z3::expr update_val(unsigned int v_sz = NUM_BYTE_BITS);
  z3::expr update_addr_v();
  z3::expr update_map_helper_func_ret();
  z3::expr new_var(unsigned int bit_sz);
  z3::expr get_stack_start_addr() {return mem_var._stack_start;} // return value: z3 bv64
  z3::expr get_stack_end_addr() {return (mem_var._stack_start + STACK_SIZE - 1);} // return value: z3 bv64
  z3::expr get_stack_bottom_addr() {return (mem_var._stack_start + STACK_SIZE);}
  z3::expr get_map_start_addr(int map_id); // return value: z3 bv64
  z3::expr get_map_end_addr(int map_id); // return value: z3 bv64
  z3::expr get_pkt_start_addr() {return mem_var._pkt_start;}
  z3::expr get_pkt_end_addr() {return (mem_var._pkt_start + mem_var._pkt_off);}
  z3::expr get_pkt_start_ptr_addr() const {return mem_var._pkt_start_ptr_addr;}
  z3::expr mem_layout_constrain() const;
  void add_expr_map_id(z3::expr e, z3::expr map_id_expr, z3::expr path_cond = Z3_true);
  void add_expr_map_id(z3::expr e, int map_id, z3::expr path_cond = Z3_true);
  void get_map_id(vector<int>& map_ids, vector<z3::expr>& path_conds, z3::expr e);
  void set_new_node_id(unsigned int node_id, const vector<unsigned int>& nodes_in,
                       const vector<z3::expr>& node_in_pc_list,
                       const vector<vector<z3::expr>>& nodes_in_regs);
  void init(unsigned int n_blocks = 1) {mem_var.init_by_layout(n_blocks);}
  void init(unsigned int prog_id, unsigned int node_id, unsigned int num_regs, unsigned int n_blocks = 1);
  void clear();
};

class smt_var_bl {
 private:
  vector<int> _mem_wt_sz, _mem_urt_sz;
  vector<int> _map_wt_sz, _map_urt_sz;
 public:
  smt_var_bl();
  void store_state_before_smt_block(smt_var& sv);
  z3::expr gen_smt_after_smt_block(smt_var& sv, z3::expr& pc);
};

class prog_state: public prog_state_base {
 public:
  mem_t _mem;
  // _reg_readable/_stack_readable is used to indicate whether this register/stack byte
  // is readable or not.
  vector<bool> _reg_readable;
  vector<bool> _stack_readable;
  prog_state();
  void init_safety_chk();
  void reg_safety_chk(int reg_write, vector<int> reg_read_list = {});
  void memory_access_and_safety_chk(uint64_t addr, uint64_t num_bytes, bool chk_safety, bool is_read);
  void memory_access_chk(uint64_t addr, uint64_t num_bytes);
  void init();
  void print() const;
  void clear();
};

// A class representing input/output type for input-output examples. now there is maps for
// BPF maps, register for input/output register
class inout_t: public inout_t_base {
 public:
  // uint64_t input_simu_r10, input_simu_pkt_ptrs[2];
  uint64_t input_simu_r10;
  uint32_t input_simu_pkt_ptrs[2];
  int64_t reg;
  // kv map: k hex_string, v: vector<uint8_t>
  vector<unordered_map<string, vector<uint8_t>>> maps;
  uint8_t* pkt = nullptr;
  inout_t();
  inout_t(const inout_t& rhs); // deep copy for vector push back
  ~inout_t();
  // insert/update kv in map
  void update_kv(int map_id, string k, vector<uint8_t> v);
  // return whether k is in the map
  bool k_in_map(int map_id, string k);
  // set pkt with random values
  void set_pkt_random_val();
  void clear();
  void init();
  void operator=(const inout_t &rhs);
  // not update input_simu_r10 which is only used for input
  bool operator==(const inout_t &rhs) const;
  friend ostream& operator<<(ostream& out, const inout_t& x);
};

struct simu_real {
  uint64_t simu_r10, real_r10;
  uint64_t simu_r1, real_r1;
  uint64_t simu_pkt, real_pkt; // used when pgm input type is PGM_INPUT_pkt_ptrs
  simu_real(uint64_t si_r10 = 0, uint64_t re_r10 = 0, uint64_t si_r1 = 0, uint64_t re_r1 = 0,
            uint64_t si_pkt = 0, uint64_t re_pkt = 0) {
    set_vals(si_r10, re_r10, si_r1, re_r1, si_pkt, re_pkt);
  }
  void set_vals(uint64_t si_r10 = 0, uint64_t re_r10 = 0, uint64_t si_r1 = 0, uint64_t re_r1 = 0,
                uint64_t si_pkt = 0, uint64_t re_pkt = 0) {
    simu_r10 = si_r10;
    real_r10 = re_r10;
    simu_r1 = si_r1;
    real_r1 = re_r1;
    simu_pkt = si_pkt;
    real_pkt = re_pkt;
  }
};

void update_ps_by_input(prog_state& ps, const inout_t& input);
// not update input_simu_r10 which is only used for input
void update_output_by_ps(inout_t& output, const prog_state& ps);
uint64_t get_simu_addr_by_real(uint64_t real_addr, mem_t& mem, simu_real sr);
uint64_t get_real_addr_by_simu(uint64_t simu_addr, mem_t& mem, simu_real sr);
void get_cmp_lists(vector<int64_t>& val_list1, vector<int64_t>& val_list2,
                   inout_t& output1, inout_t& output2);
void gen_random_input(vector<inout_t>& inputs, int64_t reg_min, int64_t reg_max);
