#include <random>
#include "inst_var.h"

using namespace std;

mem_layout mem_t::_layout;

default_random_engine gen_ebpf_inst_var;
uniform_real_distribution<double> unidist_ebpf_inst_var(0.0, 1.0);

int get_mem_size_by_layout() {
  int mem_size;
  int n_maps = mem_t::_layout._maps_attr.size();
  if (n_maps == 0) {
    mem_size = STACK_SIZE;
  } else {
    map_attr& m_attr = mem_t::_layout._maps_attr[n_maps - 1];
    mem_size = mem_t::_layout._maps_start[n_maps - 1] +
               (m_attr.val_sz / NUM_BYTE_BITS) * m_attr.max_entries;
  }
  return mem_size;
}

ostream& operator<<(ostream& out, const map_attr& m_attr) {
  out << m_attr.key_sz << " " << m_attr.val_sz << " " << m_attr.max_entries;
  return out;
}

ostream& operator<<(ostream& out, const mem_layout& layout) {
  out << "stack<start_off>: " << layout._stack_start << endl
      << "map<start_off, key_sz, val_sz, max_entries>:" << endl;
  for (int i = 0; i < layout._maps_attr.size(); i++) {
    out << layout._maps_start[i] << " " << layout._maps_attr[i] << endl;
  }
  out << "packet<len>: " << layout._pkt_sz << endl;
  return out;
}

ostream& operator<<(ostream& out, const map_t& mp) {
  out << "cur_max_entries: " << mp._cur_max_entries << ", "
      << "max_entries: " << mp._max_entries << ", "
      << "available_idx_q size: " << mp._available_idx_q.size() << endl;
  out << "k2idx:" << endl << "key\tlocal index in map: " << endl;
  for (auto it = mp._k2idx.begin(); it != mp._k2idx.end(); it++) {
    out << "0x" << it->first << "\t" << it->second << endl;
  }
  return out;
}

ostream& operator<<(ostream& out, const mem_t& mem) {
#define MEM_PRINT_GAP 32
  if (mem._mem == nullptr) return out;

  out << "1. stack related memory: ";
  for (int i = 0; i < STACK_SIZE; i++) {
    if ((i % MEM_PRINT_GAP) == 0) out << endl;
    out << hex << setfill('0') << setw(2) << static_cast<int>(mem._mem[i]) << " " << dec;
  }
  out << endl;
  if (mem._maps.size() == 0) return out;

  out << "2. map" << endl;
  for (int i = 0; i < mem._maps.size(); i++) {
    out << "map" << i << endl
        << mem._maps[i]
        << "memory: ";
    int val_sz_byte = mem_t::_layout._maps_attr[i].val_sz / NUM_BYTE_BITS;
    // print value one by one, regardless of value size
    for (int j = 0; j < mem_t::_layout._maps_attr[i].max_entries; j++) {
      if (((j * val_sz_byte) % MEM_PRINT_GAP) == 0) out << endl;
      unsigned int off = mem.get_mem_off_by_idx_in_map(i, j);
      for (int k = 0; k < val_sz_byte; k++) {
        out << hex << setfill('0') << setw(2) << static_cast<int>(mem._mem[off + k]) << dec;
      }
      out << " ";
    }
    out << endl;
  }

  out << "3. packet related memory:" << endl;
  for (int i = 0; i < mem_t::_layout._pkt_sz; i++) {
    if ((i % MEM_PRINT_GAP) == 0) out << endl;
    out << hex << setfill('0') << setw(2) << static_cast<int>(mem._pkt[i]) << dec;
  }
  out << endl;
  return out;
#undef MEM_PRINT_GAP
}

unsigned int map_t::get_and_update_next_idx() {
  unsigned int next_idx = 0;
  // If queue(free_list) is not empty, get the next idx from queue,
  // else get a random idx from the unused indexes (i.e., idx has not been used before)
  if (!_available_idx_q.empty()) {
    next_idx = _available_idx_q.front();
    _available_idx_q.pop();
  } else {
    if (_cur_max_entries >= _max_entries) {
      cout << "Error: the number of entries is the maximum, "\
           "cannnot insert more entries" << endl;
      return 0;
    }
    // get a random idx from the available idx range [0, # available indexes]
    int x = (_max_entries - 1 - _cur_max_entries) * unidist_ebpf_inst_var(gen_ebpf_inst_var);
    int c = 0;
    int target_i = -1;
    for (int i = 0; i < _idx_used.size(); i++) {
      if (_idx_used[i]) continue;
      if (c == x) {
        target_i = i;
        break;
      } else {
        c++;
      }
    }
    assert(target_i != -1);
    _idx_used[target_i] = true;
    next_idx = target_i;
    _cur_max_entries++;
  }
  return next_idx;
}

void map_t::add_available_idx(unsigned int idx) {
  _available_idx_q.push(idx);
}

void map_t::clear() {
  for (int i = 0; i < _idx_used.size(); i++) _idx_used[i] = false;
  _k2idx.clear();
  queue<unsigned int> empty;
  _available_idx_q.swap(empty);
  _cur_max_entries = 0;
}

bool map_t::operator==(const map_t &rhs) {
  bool is_equal = (_cur_max_entries == rhs._cur_max_entries) &&
                  (_max_entries == rhs._max_entries) &&
                  (_k2idx.size() == rhs._k2idx.size()) &&
                  (_available_idx_q.size() == rhs._available_idx_q.size());
  if (!is_equal) return false;
  // check _k2idx, if the sizes of k2idx are equal, just need to check
  // whether each element in _k2idx is the same as that in rhs._k2idx
  for (auto it = _k2idx.begin(); it != _k2idx.end(); it++) {
    auto rhs_it = rhs._k2idx.find(it->first);
    if (rhs_it == rhs._k2idx.end()) {
      return false;
    }
    if (it->second != rhs_it->second) return false;
  }
  // check _available_idx_q
  queue<unsigned int> q1 = _available_idx_q;
  queue<unsigned int> q2 = rhs._available_idx_q;
  for (int i = 0; i < q1.size(); i++) {
    if (q1.front() != q2.front()) return false;
    q1.pop();
    q2.pop();
  }
  return true;
}

mem_t::mem_t() {
}

mem_t::~mem_t() {
  if (_mem != nullptr) {
    delete []_mem;
    _mem = nullptr;
  }

  if (_pkt != nullptr) {
    delete []_pkt;
    _pkt = nullptr;
  }
}

void mem_t::add_map(map_attr m_attr) {
  unsigned int start_mem_off = 0;
  int n_maps = _layout._maps_attr.size();
  if (n_maps == 0) {
    start_mem_off = STACK_SIZE;
  } else {
    map_attr& m_attr = _layout._maps_attr[n_maps - 1];
    start_mem_off = _layout._maps_start[n_maps - 1] +
                    (m_attr.val_sz / NUM_BYTE_BITS) * m_attr.max_entries;
  }
  _layout._maps_attr.push_back(m_attr);
  _layout._maps_start.push_back(start_mem_off);
}

unsigned int mem_t::map_key_sz(int map_id) {
  assert(map_id < maps_number());
  return _layout._maps_attr[map_id].key_sz;
}

unsigned int mem_t::map_val_sz(int map_id) {
  assert(map_id < maps_number());
  return _layout._maps_attr[map_id].val_sz;
}

unsigned int mem_t::map_max_entries(int map_id) {
  assert(map_id < maps_number());
  return _layout._maps_attr[map_id].max_entries;
}

void mem_t::init_by_layout() {
  if (_mem != nullptr) {
    delete []_mem;
    _mem = nullptr;
  }

  if (_pkt != nullptr) {
    delete []_pkt;
    _pkt = nullptr;
  }
  _maps.clear();
  int n_maps = maps_number();
  _mem_size = get_mem_size_by_layout();
  _mem = new uint8_t[_mem_size];
  memset(_mem, 0, sizeof(uint8_t)*_mem_size);
  for (int i = 0; i < n_maps; i++) {
    _maps.push_back(map_t{_layout._maps_attr[i].max_entries});
  }

  _pkt = new uint8_t[_layout._pkt_sz];
  memset(_pkt, 0, sizeof(uint8_t)*_layout._pkt_sz);
}

void mem_t::set_map_attr(int map_id, map_attr m_attr) {
  _layout._maps_attr[map_id] = m_attr;
}

unsigned int mem_t::get_mem_off_by_idx_in_map(int map_id, unsigned int idx_in_map) {
  return (_layout._maps_start[map_id] +
          idx_in_map * (_layout._maps_attr[map_id].val_sz / NUM_BYTE_BITS));
}

void mem_t::update_kv_in_map(int map_id, string k, const uint8_t* addr_v) {
  map_t& mp = _maps[map_id];
  auto it = mp._k2idx.find(k);
  unsigned int idx = 0;
  if (it == mp._k2idx.end()) {
    idx = mp.get_and_update_next_idx();
    mp._k2idx[k] = idx;
  } else {
    idx = it->second;
  }
  unsigned int v_sz = _layout._maps_attr[map_id].val_sz / NUM_BYTE_BITS;
  unsigned int off = get_mem_off_by_idx_in_map(map_id, idx);
  uint8_t* addr_v_dst = &_mem[off];
  memcpy(addr_v_dst, addr_v, sizeof(uint8_t)*v_sz);
}

void mem_t::update_kv_in_map(int map_id, string k, const vector<uint8_t>& v) {
  map_t& mp = _maps[map_id];
  auto it = mp._k2idx.find(k);
  unsigned int idx = 0;
  if (it == mp._k2idx.end()) {
    idx = mp.get_and_update_next_idx();
    mp._k2idx[k] = idx;
  } else {
    idx = it->second;
  }
  unsigned int v_sz = _layout._maps_attr[map_id].val_sz / NUM_BYTE_BITS;
  assert(v_sz == v.size());
  unsigned int off = get_mem_off_by_idx_in_map(map_id, idx);
  uint8_t* addr_v_dst = &_mem[off];
  for (int i = 0; i < v_sz; i++) {
    addr_v_dst[i] = v[i];
  }
}

uint8_t* mem_t::get_stack_start_addr() const {
  return &_mem[_layout._stack_start];
}

uint8_t* mem_t::get_stack_bottom_addr() const {
  return &_mem[STACK_SIZE - 1] + 1;
}

uint8_t* mem_t::get_mem_start_addr() const {
  return &_mem[0];
}

uint8_t* mem_t::get_mem_end_addr() const {
  return &_mem[_mem_size - 1];
}

uint8_t* mem_t::get_pkt_start_addr() const {
  if (mem_t::_layout._pkt_sz == 0) return nullptr;
  return &_pkt[0];
}

uint8_t* mem_t::get_pkt_end_addr() const {
  if (mem_t::_layout._pkt_sz == 0) return nullptr;
  return &_pkt[mem_t::_layout._pkt_sz - 1];
}

mem_t& mem_t::operator=(const mem_t &rhs) {
  memcpy(_mem, rhs._mem, sizeof(uint8_t)*_mem_size);
  for (int i = 0; i < rhs._maps.size(); i++) _maps[i] = rhs._maps[i];
  memcpy(_pkt, rhs._pkt, sizeof(uint8_t)*_layout._pkt_sz);
  return *this;
}

bool mem_t::operator==(const mem_t &rhs) {
  bool is_equal = (_mem_size == rhs._mem_size) &&
                  (_maps.size() == rhs._maps.size());
  if (!is_equal) return false;
  for (int i = 0; i < _mem_size; i++) {
    if (_mem[i] != rhs._mem[i]) return false;
  }
  for (int i = 0; i < _maps.size(); i++) {
    if (! (_maps[i] == rhs._maps[i])) return false;
  }
  for (int i = 0; i < _layout._pkt_sz; i++) {
    if (_pkt[i] != rhs._pkt[i]) return false;
  }
  return true;
}

// need to make sure memory size is the same and has allocated memory.
// Function: copy input memory only: map related memory, maps
void mem_t::cp_input_mem(const mem_t &rhs) {
  // copy map related from "rhs" to "this"
  int n_maps = mem_t::_layout._maps_attr.size();
  if (n_maps > 0) {
    map_attr& last_m_attr = _layout._maps_attr[n_maps - 1];
    unsigned int map_start_mem_off = get_mem_off_by_idx_in_map(0, 0);
    unsigned int map_end_mem_off = get_mem_off_by_idx_in_map(n_maps - 1,
                                   last_m_attr.max_entries - 1);
    int map_mem_size = map_end_mem_off - map_start_mem_off + 1;
    uint8_t* map_start_addr = &_mem[map_start_mem_off];
    uint8_t* rhs_map_start_addr = &rhs._mem[map_start_mem_off];
    memcpy(map_start_addr, rhs_map_start_addr, sizeof(uint8_t) * map_mem_size);
    for (int i = 0; i < rhs._maps.size(); i++) _maps[i] = rhs._maps[i];
  }
}

// safe address: [get_mem_start_addr(), get_mem_end_addr()]
void mem_t::memory_access_check(uint64_t addr, uint64_t num_bytes) {
  // to avoid overflow
  uint64_t start = (uint64_t)get_mem_start_addr();
  uint64_t end = (uint64_t)get_mem_end_addr();
  uint64_t max_uint64 = 0xffffffffffffffff;
  bool legal = (addr >= start) && (addr + num_bytes - 1 <= end) &&
               (addr <= (max_uint64 - num_bytes + 1));
  if (legal) return;

  if (mem_t::_layout._pkt_sz == 0) {
    legal = false;
  } else {
    start = (uint64_t)get_pkt_start_addr();
    end = (uint64_t)get_pkt_end_addr();
    legal = (addr >= start) && (addr + num_bytes - 1 <= end) &&
            (addr <= (max_uint64 - num_bytes + 1));
  }
  if (!legal) {
    string err_msg = "unsafe memory access";
    throw (err_msg);
  }
}

void mem_t::clear() {
  memset(_mem, 0, sizeof(uint8_t)*_mem_size);
  for (int i = 0; i < _maps.size(); i++) _maps[i].clear();
  memset(_pkt, 0, sizeof(uint8_t)*_layout._pkt_sz);
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
    add(rhs.block[i], rhs.addr[i], rhs.val[i]);
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
    out << i << ": " << s.is_valid[i] << " "
        << s.key[i].simplify() << " " << s.addr_v[i].simplify() << endl;
  }
  return out;
}

/* class smt_map_wt start */
ostream& operator<<(ostream& out, const mem_table& m) {
  out << "pointers: " << endl;
  for (auto it = m._ptrs.begin(); it != m._ptrs.end(); it++) {
    out << "pointer id: " << it->first << ", pc: " << it->second[0] << endl;
  }
  out << "type: " << m._type << ", map: " << m._map_id << endl
      << "memory WT:" << endl << m._wt
      << "memory URT:" << endl << m._urt
      << endl;
  return out;
}

/* class smt_wt end */
void smt_mem::init_by_layout(unsigned int n_blocks) {
  _path_cond_list.resize(n_blocks, Z3_false);
  // the first block is the root block, without this default value, codegen_test will fail
  if (n_blocks > 0) _path_cond_list[0] = Z3_true;
  int n_maps = mem_t::maps_number();
  for (int i = 0; i < n_maps; i++) {
    unsigned int start_mem_off = mem_t::get_mem_off_by_idx_in_map(i, 0);
    z3::expr start = (_stack_start + to_expr((uint64_t)start_mem_off)).simplify();
    _addrs_map_v_next.push_back(start);
  }
  int n_mem_tables = 1 + n_maps; // stack + maps
  if (mem_t::mem_t::_layout._pkt_sz > 0) n_mem_tables++;
  _mem_tables.resize(n_mem_tables);
  int i = 0;
  _mem_tables[i]._type = MEM_TABLE_stack;
  i++;
  if (mem_t::mem_t::_layout._pkt_sz > 0) {
    _mem_tables[i]._type = MEM_TABLE_pkt;
    i++;
  }
  for (int map_id = 0; i < _mem_tables.size(); i++, map_id++) {
    _mem_tables[i]._type = MEM_TABLE_map;
    _mem_tables[i]._map_id = map_id;
  }
  _map_tables.resize(n_maps);
}

z3::expr smt_mem::get_and_update_addr_v_next(int map_id) {
  z3::expr res = _addrs_map_v_next[map_id];
  unsigned int v_sz = mem_t::map_val_sz(map_id) / NUM_BYTE_BITS;
  _addrs_map_v_next[map_id] = _addrs_map_v_next[map_id] + to_expr((uint64_t)v_sz);
  return res;
}

void smt_mem::get_mem_table_id(vector<int>& table_ids, vector<z3::expr>& path_conds, z3::expr ptr_expr) {
  table_ids.clear();
  path_conds.clear();
  for (int i = 0; i < _mem_tables.size(); i++) {
    if (! _mem_tables[i].is_ptr_in_ptrs(ptr_expr)) continue;
    z3::expr pc = _mem_tables[i].get_ptr_path_cond(ptr_expr);
    table_ids.push_back(i);
    path_conds.push_back(pc);
  }
}

int smt_mem::get_mem_table_id(int type, int map_id) {
  int not_found_flag = -1;
  for (int i = 0; i < _mem_tables.size(); i++) {
    // check type
    if (_mem_tables[i]._type == type) {
      if (type != MEM_TABLE_map) return i;
      // check map_id
      if (_mem_tables[i]._map_id == map_id) return i;
    }
  }
  return not_found_flag;
}

int smt_mem::get_type(int mem_table_id) {
  assert(mem_table_id >= 0);
  assert(mem_table_id < _mem_tables.size());
  return _mem_tables[mem_table_id]._type;
}

void smt_mem::add_in_mem_table_wt(int mem_table_id, unsigned int block, z3::expr addr, z3::expr val) {
  assert(mem_table_id >= 0);
  assert(mem_table_id < _mem_tables.size());
  _mem_tables[mem_table_id]._wt.add(block, addr, val);
}

void smt_mem::add_in_mem_table_urt(int mem_table_id, unsigned int block, z3::expr addr, z3::expr val) {
  assert(mem_table_id >= 0);
  assert(mem_table_id < _mem_tables.size());
  _mem_tables[mem_table_id]._urt.add(block, addr, val);
}

void smt_mem::add_ptr(z3::expr ptr_expr, int table_id, z3::expr path_cond) {
  assert(table_id >= 0);
  assert(table_id < _mem_tables.size());
  _mem_tables[table_id].add_ptr(ptr_expr, path_cond);
}

void smt_mem::add_ptr(z3::expr ptr_expr, z3::expr ptr_from_expr, z3::expr path_cond) {
  unsigned int ptr_from_id = ptr_from_expr.id();
  for (int i = 0; i < _mem_tables.size(); i++) {
    auto found = _mem_tables[i]._ptrs.find(ptr_from_id);
    if (found != _mem_tables[i]._ptrs.end()) {
      _mem_tables[i].add_ptr(ptr_expr, path_cond);
    }
  }
}

void smt_mem::add_ptr_by_map_id(z3::expr ptr_expr, int map_id, z3::expr path_cond) {
  int table_id = get_mem_table_id(MEM_TABLE_map, map_id);
  if (table_id != -1) {
    _mem_tables[table_id].add_ptr(ptr_expr, path_cond);
  }
}

z3::expr smt_mem::get_block_path_cond(unsigned int block_id) {
  assert(block_id < _path_cond_list.size());
  return _path_cond_list[block_id];
}

void smt_mem::add_path_cond(z3::expr pc, unsigned int block_id) {
  assert(block_id < _path_cond_list.size());
  _path_cond_list[block_id] = pc;
}

ostream& operator<<(ostream& out, const smt_mem& s) {
  for (int i = 0; i < s._mem_tables.size(); i++) {
    out << "memory table " << i << endl
        << s._mem_tables[i];
  }
  for (int i = 0; i < s._map_tables.size(); i++) {
    out << "map table " << i << endl
        << "map WT:" << endl << s._map_tables[i]._wt
        << "map URT:" << endl << s._map_tables[i]._urt
        << endl;
  }

  return out;
}
/* class smt_wt end */

/* class smt_var start */
smt_var::smt_var()
  : smt_var_base() {
  mem_addr_id = 0;
  is_vaild_id = 0;
  path_cond_id = 0;
  key_cur_id = 0;
  val_cur_id = 0;
  addr_v_cur_id = 0;
  map_helper_func_ret_cur_id = 0;
}

smt_var::smt_var(unsigned int prog_id, unsigned int node_id, unsigned int num_regs)
  : smt_var_base(prog_id, node_id, num_regs) {
  mem_addr_id = 0;
  is_vaild_id = 0;
  path_cond_id = 0;
  key_cur_id = 0;
  val_cur_id = 0;
  addr_v_cur_id = 0;
  map_helper_func_ret_cur_id = 0;
}

smt_var::~smt_var() {
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

z3::expr smt_var::get_map_start_addr(int map_id) { // return value: z3 bv64
  unsigned int map_start_off = mem_t::get_mem_off_by_idx_in_map(map_id, 0);
  z3::expr map_start_addr = (mem_var._stack_start + to_expr((uint64_t)map_start_off)).simplify();
  return map_start_addr;
}

z3::expr smt_var::get_map_end_addr(int map_id) { // return value: z3 bv64
  unsigned int number_entries = mem_t::map_max_entries(map_id);
  unsigned int map_end_off = mem_t::get_mem_off_by_idx_in_map(map_id, number_entries) - 1;
  z3::expr map_end_addr = (mem_var._stack_start + to_expr((uint64_t)map_end_off)).simplify();
  return map_end_addr;
}

void smt_var::add_expr_map_id(z3::expr e, z3::expr map_id_expr, z3::expr path_cond) {
  assert(map_id_expr.is_numeral());
  int map_id = map_id_expr.get_numeral_uint64();
  add_expr_map_id(e, map_id, path_cond);
}

void smt_var::add_expr_map_id(z3::expr e, int map_id, z3::expr path_cond) {
  cout << "add map_id: reg:" << e << ", map_id:" << map_id << ", pc:" << path_cond << endl;
  unsigned int e_id = e.id();
  auto found = expr_map_id.find(e_id);
  if (found == expr_map_id.end()) {
    expr_map_id.insert({e_id, {map_id_pc(map_id, path_cond)}});
    return;
  }
  found->second.push_back({map_id_pc(map_id, path_cond)});
}

void smt_var::get_map_id(vector<int>& map_ids, vector<z3::expr>& path_conds, z3::expr e) {
  map_ids.clear();
  path_conds.clear();
  auto found = expr_map_id.find(e.id());
  if (found == expr_map_id.end()) return;
  for (int i = 0; i < found->second.size(); i++) {
    map_ids.push_back(found->second[i].map_id);
    path_conds.push_back(found->second[i].path_cond);
  }
}

// constrain:
// 1. pkt_sz = 0: 0 < [memory start, memory end] <= max_uint64
// 2. pkt_sz > 0: 0 < [memory start, memory end] < [pkt start, pkt end] <= max_uint64
z3::expr smt_var::mem_layout_constrain() const {
  z3::expr mem_start = mem_var._stack_start;
  int mem_sz = get_mem_size_by_layout();
  z3::expr mem_off = to_expr((uint64_t)(mem_sz - 1));
  z3::expr pkt_start = mem_var._pkt_start;
  int pkt_sz = mem_t::_layout._pkt_sz;
  z3::expr pkt_off = to_expr((uint64_t)(pkt_sz - 1));
  z3::expr max_uint64 = to_expr((uint64_t)0xffffffffffffffff);

  if (pkt_sz == 0) {
    z3::expr f = (mem_start != NULL_ADDR_EXPR) &&
                 uge(max_uint64 - mem_off, mem_start);
    return f;
  }

  z3::expr f = (mem_start != NULL_ADDR_EXPR) &&
               ugt(pkt_start, mem_off) && ugt(pkt_start - mem_off, mem_start) &&
               uge(max_uint64 - pkt_off, pkt_start);
  return f;
}

void smt_var::set_new_node_id(unsigned int node_id, vector<unsigned int>& nodes_in,
                              vector<z3::expr>& node_in_pc_list,
                              vector<vector<z3::expr>>& nodes_in_regs) {
  // set the register names first, use names later
  smt_var_base::set_new_node_id(node_id);
  // update path condition of this block
  z3::expr path_cond = Z3_false;
  for (int i = 0; i < node_in_pc_list.size(); i++) {
    path_cond = path_cond || node_in_pc_list[i];
  }
  mem_var.add_path_cond(path_cond, node_id);
  if (node_id == 0) return;
  int num_regs = 0;
  if (nodes_in.size() > 0) num_regs = nodes_in_regs[0].size();
  for (int i = 0; i < num_regs; i++) {
    z3::expr cur_reg = get_init_reg_var(i);
    // 1. update map ids
    if (expr_map_id.size() > 0) {
      vector<bool> map_ids(mem_t::maps_number(), false);
      vector<z3::expr> map_id_pcs(mem_t::maps_number(), Z3_false);
      for (int j = 0; j < nodes_in.size(); j++) {
        z3::expr reg_expr = nodes_in_regs[nodes_in[j]][i];
        vector<int> ids;
        vector<z3::expr> pcs;
        get_map_id(ids, pcs, reg_expr);
        for (int k = 0; k < ids.size(); k++) {
          int id = ids[k];
          // merge path condition for the same map id
          map_id_pcs[id] = map_id_pcs[id] || (pcs[k] && node_in_pc_list[j]);
          if (! map_ids[id]) map_ids[id] = true;
        }
      }
      for (int j = 0; j < map_ids.size(); j++) {
        if (! map_ids[j]) continue;
        add_expr_map_id(cur_reg, j, map_id_pcs[j].simplify());
      }
    }
    // 2. update pointers
    // for each register, get the table_id list and the corresponding path condition list
    vector<bool> table_ids(mem_var._mem_tables.size(), false);
    vector<z3::expr> path_conds(mem_var._mem_tables.size(), Z3_false);
    for (int j = 0; j < nodes_in.size(); j++) {
      z3::expr reg_ptr = nodes_in_regs[nodes_in[j]][i];
      vector<int> ids;
      vector<z3::expr> pcs;
      mem_var.get_mem_table_id(ids, pcs, reg_ptr);
      for (int k = 0; k < ids.size(); k++) {
        int id = ids[k];
        // merge path condition for the same table id
        path_conds[id] = path_conds[id] || (pcs[k] && node_in_pc_list[j]);
        if (! table_ids[id]) {
          table_ids[id] = true;
        }
      }
    }
    // update mem_table's pointers
    for (int j = 0; j < table_ids.size(); j++) {
      if (! table_ids[j]) continue;
      mem_var.add_ptr(cur_reg, j, path_conds[j].simplify());
    }
  }
}

void smt_var::init(unsigned int prog_id, unsigned int node_id, unsigned int num_regs, unsigned int n_blocks) {
  smt_var_base::init(prog_id, node_id, num_regs);
  mem_var.init_by_layout(n_blocks);
  if (node_id == 0) {
    int stack_mem_table_id = mem_var.get_mem_table_id(MEM_TABLE_stack);
    mem_var.add_ptr(get_init_reg_var(10), stack_mem_table_id, Z3_true); // r10 is the stack pointer
    if (mem_t::_layout._pkt_sz > 0) {
      int pkt_mem_table_id = mem_var.get_mem_table_id(MEM_TABLE_pkt);
      mem_var.add_ptr(get_init_reg_var(1), pkt_mem_table_id, Z3_true); // r1 is the pkt pointer
    }
  }
}

void smt_var::clear() {
  smt_var_base::clear();
  for (size_t i = 0; i < reg_var.size(); i++) {
    mem_addr_id = 0;
    is_vaild_id = 0;
    path_cond_id = 0;
    key_cur_id = 0;
    val_cur_id = 0;
    addr_v_cur_id = 0;
    map_helper_func_ret_cur_id = 0;
  }
  mem_var.clear();
  expr_map_id.clear();
}
/* class smt_var end */

smt_var_bl::smt_var_bl() {
  int n_maps =  mem_t::maps_number();
  int n_mem_tables = 1 + n_maps; // stack + maps
  if (mem_t::mem_t::_layout._pkt_sz > 0) n_mem_tables++;
  _mem_wt_sz.resize(n_mem_tables, 0);
  _mem_urt_sz.resize(n_mem_tables, 0);
  _map_wt_sz.resize(n_maps, 0);
  _map_urt_sz.resize(n_maps, 0);
}

void smt_var_bl::store_state_before_smt_block(smt_var& sv) {
  for (int i = 0; i < sv.mem_var._mem_tables.size(); i++) {
    _mem_wt_sz[i] = sv.mem_var._mem_tables[i]._wt.size();
    _mem_urt_sz[i] = sv.mem_var._mem_tables[i]._urt.size();
  }
  for (int i = 0; i < sv.mem_var._map_tables.size(); i++) {
    _map_wt_sz[i] = sv.mem_var._map_tables[i]._wt.size();
    _map_urt_sz[i] = sv.mem_var._map_tables[i]._urt.size();
  }
}

z3::expr smt_var_bl::gen_smt_after_smt_block(smt_var& sv, z3::expr& pc) {
  z3::expr f = Z3_true;
  for (int i = 0; i < sv.mem_var._mem_tables.size(); i++) {
    for (int j = _mem_wt_sz[i]; j < sv.mem_var._mem_tables[i]._wt.size(); j++) {
      f = f && z3::implies(!pc, sv.mem_var._mem_tables[i]._wt.addr[j] == NULL_ADDR_EXPR);
    }
    for (int j = _mem_urt_sz[i]; j < sv.mem_var._mem_tables[i]._urt.size(); j++) {
      f = f && z3::implies(!pc, sv.mem_var._mem_tables[i]._urt.addr[j] == NULL_ADDR_EXPR);
    }
  }
  for (int i = 0; i < sv.mem_var._map_tables.size(); i++) {
    for (int j = _map_wt_sz[i]; j < sv.mem_var._map_tables[i]._wt.size(); j++) {
      f = f && z3::implies(!pc, sv.mem_var._map_tables[i]._wt.is_valid[j] == Z3_false);
    }
    for (int j = _map_urt_sz[i]; j < sv.mem_var._map_tables[i]._urt.size(); j++) {
      f = f && z3::implies(!pc, sv.mem_var._map_tables[i]._urt.is_valid[j] == Z3_false);
    }
  }
  return f;
}

void prog_state::init() {
  _mem.init_by_layout();
}

void prog_state::print() const {
  prog_state_base::print();
  cout << "Memory:" << endl;
  cout << _mem << endl;
}

void prog_state::clear() {
  prog_state_base::clear();
  _mem.clear();
}

inout_t::inout_t() {
  uint64_t r10_min = STACK_SIZE;
  uint64_t r10_max = 0xffffffffffffffff - get_mem_size_by_layout() + 1 - STACK_SIZE;
  input_simu_r10 = r10_min + (r10_max - r10_min) * unidist_ebpf_inst_var(gen_ebpf_inst_var);
  pkt = new uint8_t[mem_t::_layout._pkt_sz];
  memset(pkt, 0, sizeof(uint8_t)*mem_t::_layout._pkt_sz);
}

// deep copy for vector push back
inout_t::inout_t(const inout_t& rhs) {
  pkt = new uint8_t[mem_t::_layout._pkt_sz];
  input_simu_r10 = rhs.input_simu_r10;
  reg = rhs.reg;
  maps = rhs.maps;
  memcpy(pkt, rhs.pkt, sizeof(uint8_t)*mem_t::_layout._pkt_sz);
}

inout_t::~inout_t() {
  if (pkt != nullptr) {
    delete []pkt;
    pkt = nullptr;
  }
}

// insert/update kv in map
void inout_t::update_kv(int map_id, string k, vector<uint8_t> v) {
  assert(map_id < maps.size());
  auto it = maps[map_id].find(k);
  maps[map_id][k] = v;
}

bool inout_t::k_in_map(int map_id, string k) {
  assert(map_id < maps.size());
  auto it = maps[map_id].find(k);
  if (it == maps[map_id].end()) return false;
  return true;
}

void inout_t::set_pkt_random_val() {
  for (int i = 0; i < mem_t::_layout._pkt_sz; i++) {
    uint8_t val = 0xff * unidist_ebpf_inst_var(gen_ebpf_inst_var);
    pkt[i] = val;
  }
}

void inout_t::clear() {
  reg = 0;
  for (int i = 0; i < maps.size(); i++) maps[i].clear();
}

void inout_t::init() {
  reg = 0;
  int n_maps = mem_t::maps_number();
  maps.resize(n_maps);
}

void inout_t::operator=(const inout_t &rhs) {
  input_simu_r10 = rhs.input_simu_r10;
  reg = rhs.reg;
  maps = rhs.maps;
  memcpy(pkt, rhs.pkt, sizeof(uint8_t)*mem_t::_layout._pkt_sz);
}

bool inout_t::operator==(const inout_t &rhs) const {
  bool res = (reg == rhs.reg) && (maps.size() == rhs.maps.size());
  if (! res) return false;
  for (int i = 0; i < maps.size(); i++) {
    // each map should be the same
    for (auto it = maps[i].begin(); it != maps[i].end(); it++) {
      // for each key in map, key should be found in rhs.map and
      // the corresponding values should be equal
      string k = it->first;
      auto it_rhs = rhs.maps[i].find(k);
      if (it_rhs == rhs.maps[i].end()) return false;

      const vector<uint8_t>& v = it->second;
      const vector<uint8_t>& v_rhs = it_rhs->second;
      if (v.size() != v_rhs.size()) return false;
      for (int j = 0; j < v.size(); j++) {
        if (v[j] != v_rhs[j]) return false;
      }
    }
  }
  return true;
}

ostream& operator<<(ostream& out, const inout_t& x) {
  out << hex << "simu_r10:" << x.input_simu_r10 << dec << " ";
  out << "reg:" << x.reg << " ";
  for (int i = 0; i < x.maps.size(); i++) {
    out << "map " << i << ": ";
    for (auto it = x.maps[i].begin(); it != x.maps[i].end(); it++) {
      out << it->first << "," << uint8_t_vec_2_hex_str(it->second) << " ";
    }
  }
  out << " " << "pkt: ";
  for (int i = 0; i < mem_t::_layout._pkt_sz; i++) {
    out << hex << setfill('0') << setw(2) << static_cast<int>(x.pkt[i]) << dec;
  }
  return out;
}


void update_ps_by_input(prog_state& ps, const inout_t& input) {
  ps._regs[10] = input.input_simu_r10;
  // cp input register
  ps._regs[1] = input.reg;
  // cp input map
  ps._mem.clear();
  for (int i = 0; i < input.maps.size(); i++) {
    for (auto it = input.maps[i].begin(); it != input.maps[i].end(); it++) {
      ps._mem.update_kv_in_map(i, it->first, it->second);
    }
  }
  unsigned int pkt_sz = mem_t::_layout._pkt_sz;
  memcpy(ps._mem._pkt, input.pkt, sizeof(uint8_t)*pkt_sz);
}

void update_output_by_ps(inout_t& output, const prog_state& ps) {
  output.clear();
  // cp register
  output.reg = ps._regs[0];
  // cp map
  int n_maps = ps._mem.maps_number();
  for (int i = 0; i < n_maps; i++) {
    const map_t& mp = ps._mem._maps[i];
    for (auto it = mp._k2idx.begin(); it != mp._k2idx.end(); it++) {
      string key = it->first;
      unsigned int idx = it->second;
      unsigned int off = ps._mem.get_mem_off_by_idx_in_map(i, idx);
      unsigned int v_sz = mem_t::map_val_sz(i) / NUM_BYTE_BITS;
      vector<uint8_t> value(v_sz);
      for (int j = 0; j < v_sz; j++) {
        value[j] = ps._mem._mem[j + off];
      }
      output.update_kv(i, key, value);
    }
  }
  // cp pkt
  unsigned int pkt_sz = mem_t::_layout._pkt_sz;
  memcpy(output.pkt, ps._mem._pkt, sizeof(uint8_t)*pkt_sz);
}

// parameter r10 is the simulated r10, stack_bottom is the real r10
uint64_t get_simu_addr_by_real(uint64_t real_addr, mem_t& mem, simu_real sr) {
  // real_addr may in mem or pkt
  // assume real_addr in mem first
  if ((real_addr >= (uint64_t)mem.get_mem_start_addr()) &&
      (real_addr <= (uint64_t)mem.get_mem_end_addr())) {
    return (real_addr + sr.simu_r10 - sr.real_r10);
  }
  // if real_addr not in mem, assume it is in pkt
  if ((real_addr >= (uint64_t)mem.get_pkt_start_addr()) &&
      (real_addr <= (uint64_t)mem.get_pkt_end_addr())) {
    return (real_addr + sr.simu_r1 - sr.real_r1);
  }

  // else cannot convert real_addr to simu_addr, raise error
  string err_msg = "convert real_addr to simu_addr fail";
  throw (err_msg);
}

uint64_t get_real_addr_by_simu(uint64_t simu_addr, mem_t& mem, simu_real sr) {
  // real_addr may in mem or pkt
  // assume real_addr in mem first
  uint64_t real_addr = simu_addr + sr.real_r10 - sr.simu_r10;
  if ((real_addr >= (uint64_t)mem.get_mem_start_addr()) &&
      (real_addr <= (uint64_t)mem.get_mem_end_addr())) {
    return real_addr;
  }
  // if real_addr not in mem, assume it is in pkt
  real_addr = simu_addr + sr.real_r1 - sr.simu_r1;
  if ((real_addr >= (uint64_t)mem.get_pkt_start_addr()) &&
      (real_addr <= (uint64_t)mem.get_pkt_end_addr())) {
    return real_addr;
  }

  // else cannot convert simu_addr to real_addr, raise error
  string err_msg = "convert simu_addr to real_addr fail";
  throw (err_msg);
}

int get_cmp_lists_one_map(vector<int64_t>& val_list1, vector<int64_t>& val_list2,
                          unsigned int val_sz,
                          const unordered_map<string, vector<uint8_t>>& map1,
                          const unordered_map<string, vector<uint8_t>>& map2,
                          bool record_shared_keys) {
  int diff_count = 0;
  for (auto it = map1.begin(); it != map1.end(); it++) {
    string k = it->first;
    auto it_map2 = map2.find(k);
    if (it_map2 == map2.end()) {
      diff_count++;
    } else {
      if (! record_shared_keys) continue;
      for (int j = 0; j < val_sz; j++) {
        val_list1.push_back((uint64_t)it->second[j]);
        val_list2.push_back((uint64_t)it_map2->second[j]);
      }
    }
  }
  return diff_count;
}

// val_list: [reg, #different keys, map0[k0], map0[k1], ...., map1[k0'], ...., pkt]
void get_cmp_lists(vector<int64_t>& val_list1, vector<int64_t>& val_list2,
                   inout_t& output1, inout_t& output2) {
  val_list1.resize(2);
  val_list2.resize(2);
  val_list1[0] = output1.reg;
  val_list2[0] = output2.reg;
  int diff_count1 = 0;
  int diff_count2 = 0;
  for (int i = 0; i < mem_t::maps_number(); i++) {
    unsigned int val_sz = mem_t::map_val_sz(i) / NUM_BYTE_BITS;
    diff_count1 += get_cmp_lists_one_map(val_list1, val_list2, val_sz,
                                         output1.maps[i], output2.maps[i], true);
    diff_count2 += get_cmp_lists_one_map(val_list2, val_list1, val_sz,
                                         output2.maps[i], output1.maps[i], false);
  }
  val_list1[1] = diff_count1 + diff_count2;
  val_list2[1] = 0;
  // add pkt into lists
  for (int i = 0; i < mem_t::_layout._pkt_sz; i++) val_list1.push_back(output1.pkt[i]);
  for (int i = 0; i < mem_t::_layout._pkt_sz; i++) val_list2.push_back(output2.pkt[i]);
}

void gen_random_input(vector<inout_t>& inputs, int64_t reg_min, int64_t reg_max) {
  uint64_t max_uint64 = 0xffffffffffffffff;
  // 1. Generate stack bottom address r10
  uint64_t r10_min = 1; // address cannot be 0
  uint64_t mem_size_without_stack = get_mem_size_by_layout() - STACK_SIZE;
  uint64_t r10_max = max_uint64 - mem_size_without_stack - mem_t::_layout._pkt_sz;
  for (int i = 0; i < inputs.size(); i++) {
    inputs[i].input_simu_r10 = r10_min + (uint64_t)((r10_max - r10_min) * unidist_ebpf_inst_var(gen_ebpf_inst_var));
  }
  // 2. Generate pkt, set pkt with random values
  for (int i = 0; i < inputs.size(); i++) {
    inputs[i].set_pkt_random_val();
  }
  // 3. Generate input register r1
  if (mem_t::_layout._pkt_sz == 0) { // case 1: r1 is not used as pkt address
    unordered_set<int64_t> reg_set; // use set to avoid that registers have the same values
    for (int i = 0; i < inputs.size();) {
      int64_t reg = reg_min + (reg_max - reg_min) * unidist_ebpf_inst_var(gen_ebpf_inst_var);
      if (reg_set.find(reg) == reg_set.end()) {
        reg_set.insert(reg);
        inputs[i].reg = reg;
        i++;
      }
    }
  } else { // case 2: r1 is used as pkt address
    for (int i = 0; i < inputs.size(); i++) {
      uint64_t pkt_min = inputs[i].input_simu_r10 + mem_size_without_stack;
      uint64_t pkt_max = max_uint64;
      inputs[i].reg = pkt_min + (uint64_t)((pkt_max - pkt_min) * unidist_ebpf_inst_var(gen_ebpf_inst_var));
    }
  }
}
