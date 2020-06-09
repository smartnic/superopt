#pragma once

#include <iomanip>
#include <iostream>
#include <vector>
#include <unordered_map>
#include <queue>
#include <string.h>
#include "../../../src/utils.h"
#include "../../../src/isa/inst_var.h"

using namespace std;

#define STACK_SIZE 512 // 512 bytes
#define NULL_ADDR 0
#define NULL_ADDR_EXPR to_expr(NULL_ADDR)
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

class mem_layout {
 public:
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
  mem_t();
  ~mem_t();
  static void add_map(map_attr m_attr);
  static void set_pkt_sz(unsigned int sz) {_layout._pkt_sz = sz;}
  static unsigned int maps_number() {return _layout._maps_attr.size();}
  static unsigned int map_key_sz(int map_id);
  static unsigned int map_val_sz(int map_id);
  static unsigned int map_max_entries(int map_id);
  // 1. compute "_mem_size" and according to "_layout";
  // 2. allocate memory for "_mem", "_pkt"
  // 3. init _maps
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
  mem_t& operator=(const mem_t &rhs);
  bool operator==(const mem_t &rhs);
  void cp_input_mem(const mem_t &rhs);
  // memory_access_check is used to avoid segmentation fault
  // If memory access not in the legal range, throw error
  void memory_access_check(uint64_t addr, uint64_t num_bytes);
  void clear();
  friend ostream& operator<<(ostream& out, const mem_t& mem);
};

extern mem_layout _layout;

class smt_wt {
 private:
  bool is_equal(z3::expr e1, z3::expr e2);
 public:
  vector<z3::expr> addr; // 64-bit bitvector
  vector<z3::expr> val;  // 8-bit bitvector
  void add(z3::expr a, z3::expr v) {addr.push_back(a); val.push_back(v);}
  void clear() {addr.clear(); val.clear();}
  unsigned int size() {return addr.size();}
  smt_wt& operator=(const smt_wt &rhs);
  bool operator==(const smt_wt &rhs);
  friend ostream& operator<<(ostream& out, const smt_wt& s);
};

class mem_wt {
 public:
  smt_wt _wt; // write table, each element is for write instructions
  smt_wt _urt; // uninitalized read table, each element is for read before write instructions
  void clear() {_wt.clear(); _urt.clear();}
};

class smt_map_wt {
 public:
  vector<z3::expr> is_valid; // flag, indicate whether this entry is valid or not
  vector<z3::expr> addr_map;
  vector<z3::expr> key;
  vector<z3::expr> addr_v;
  void add(z3::expr iv, z3::expr a, z3::expr k, z3::expr v) {
    is_valid.push_back(iv);
    addr_map.push_back(a);
    key.push_back(k);
    addr_v.push_back(v);
  }
  void clear() {is_valid.clear(); addr_map.clear(); key.clear(); addr_v.clear();}
  unsigned int size() {return addr_map.size();}
  friend ostream& operator<<(ostream& out, const smt_map_wt& s);
};

class map_wt {
 public:
  smt_map_wt _wt;
  smt_map_wt _urt;
  void clear() {_wt.clear(); _urt.clear();}
};

class smt_mem {
 public:
  z3::expr _stack_start = string_to_expr("stack_start");
  z3::expr _pkt_start = string_to_expr("pkt_start");
  mem_wt _mem_table; // stack, pkt, map related memory
  map_wt _map_table;
  vector<z3::expr> _addrs_map_v_next;

  smt_mem() {}
  smt_mem(uint64_t stack_start) {_stack_start = to_expr(stack_start);}
  void set_stack_start(uint64_t stack_start) {_stack_start = to_expr(stack_start);}
  void init_addrs_map_v_next_by_layout();
  z3::expr get_and_update_addr_v_next(int map_id);
  void clear() {_mem_table.clear(); _map_table.clear(); _addrs_map_v_next.clear();}
  friend ostream& operator<<(ostream& out, const smt_mem& s);
};

// SMT Variable format
// register: r_[prog_id]_[node_id]_[reg_id]_[version_id]
// map key: k_[prog_id]_[node_id]_[version_id]
// map value: v_[prog_id]_[node_id]_[version_id]
// map address of value: av_[prog_id]_[node_id]_[version_id]
class smt_var: public smt_var_base {
 private:
  unsigned int mem_addr_id, is_vaild_id, key_cur_id,
           val_cur_id, addr_v_cur_id, map_helper_func_ret_cur_id;
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
  z3::expr get_stack_start_addr() {return mem_var._stack_start;} // return value: z3 bv64
  z3::expr get_stack_end_addr() {return (mem_var._stack_start + STACK_SIZE - 1);} // return value: z3 bv64
  z3::expr get_stack_bottom_addr() {return (mem_var._stack_start + STACK_SIZE);}
  z3::expr get_map_start_addr(int map_id); // return value: z3 bv64
  z3::expr get_map_end_addr(int map_id); // return value: z3 bv64
  z3::expr get_pkt_start_addr() {return mem_var._pkt_start;}
  z3::expr get_pkt_end_addr() {return (mem_var._pkt_start + to_expr((uint64_t)mem_t::_layout._pkt_sz - 1));}
  z3::expr mem_layout_constrain() const;
  void init() {mem_var.init_addrs_map_v_next_by_layout();}
  void init(unsigned int prog_id, unsigned int node_id, unsigned int num_regs);
  void clear();
};

class prog_state: public prog_state_base {
 public:
  mem_t _mem;
  prog_state() {_regs.resize(NUM_REGS, 0);}
  void init();
  void print() const;
  void clear();
};

// A class representing input/output type for input-output examples. now there is maps for
// BPF maps, register for input/output register
class inout_t: public inout_t_base {
 public:
  uint64_t input_simu_r10;
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
  void clear();
  void init();
  void operator=(const inout_t &rhs);
  // not update input_simu_r10 which is only used for input
  bool operator==(const inout_t &rhs) const;
  friend ostream& operator<<(ostream& out, const inout_t& x);
};

struct simu_real {
  uint64_t simu_r10, real_r10;
  uint64_t simu_r1, real_r1; // if pkt_len != 0, input reg r1 contains the pkt start address
  simu_real(uint64_t si_r10 = 0, uint64_t re_r10 = 0, uint64_t si_r1 = 0, uint64_t re_r1 = 0) {
    simu_r10 = si_r10;
    real_r10 = re_r10;
    simu_r1 = si_r1;
    real_r1 = re_r1;
  }
};

void update_ps_by_input(prog_state& ps, const inout_t& input);
// not update input_simu_r10 which is only used for input
void update_output_by_ps(inout_t& output, const prog_state& ps);
uint64_t get_simu_addr_by_real(uint64_t real_addr, mem_t& mem, simu_real sr);
uint64_t get_real_addr_by_simu(uint64_t simu_addr, mem_t& mem, simu_real sr);
void get_cmp_lists(vector<int64_t>& val_list1, vector<int64_t>& val_list2,
                   inout_t& output1, inout_t& output2);
