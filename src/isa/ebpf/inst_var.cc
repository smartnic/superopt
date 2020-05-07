#include "inst_var.h"

using namespace std;

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
  return out;
#undef MEM_PRINT_GAP
}

unsigned int map_t::get_and_update_next_idx() {
  unsigned int next_idx = 0;
  if (!_available_idx_q.empty()) {
    next_idx = _available_idx_q.front();
    _available_idx_q.pop();
  } else {
    if (_cur_max_entries >= _max_entries) {
      cout << "Error: the number of entries is the maximum, "\
           "cannnot insert more entries" << endl;
      return 0;
    }
    next_idx = _cur_max_entries;
    _cur_max_entries++;
  }
  return next_idx;
}

void map_t::add_available_idx(unsigned int off) {
  _available_idx_q.push(off);
}

void map_t::clear() {
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
  if (map_id >= maps_number()) {
    string err_msg = "map_id > #maps";
    throw (err_msg);
  }
  return _layout._maps_attr[map_id].key_sz;
}

unsigned int mem_t::map_val_sz(int map_id) {
  if (map_id >= maps_number()) {
    string err_msg = "map_id > #maps";
    throw (err_msg);
  }
  return _layout._maps_attr[map_id].val_sz;
}

unsigned int mem_t::map_max_entries(int map_id) {
  if (map_id >= maps_number()) {
    string err_msg = "map_id > #maps";
    throw (err_msg);
  }
  return _layout._maps_attr[map_id].max_entries;
}

void mem_t::init_mem_by_layout() {
  int n_maps = _layout._maps_attr.size();
  if (n_maps == 0) {
    _mem_size = STACK_SIZE;
  } else {
    map_attr& m_attr = _layout._maps_attr[n_maps - 1];
    _mem_size = _layout._maps_start[n_maps - 1] +
                (m_attr.val_sz / NUM_BYTE_BITS) * m_attr.max_entries;
  }
  _mem = new uint8_t[_mem_size];
  memset(_mem, 0, sizeof(uint8_t)*_mem_size);
  for (int i = 0; i < n_maps; i++) {
    _maps.push_back(map_t{_layout._maps_attr[i].max_entries});
  }
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

mem_t& mem_t::operator=(const mem_t &rhs) {
  memcpy(_mem, rhs._mem, sizeof(uint8_t)*_mem_size);
  for (int i = 0; i < rhs._maps.size(); i++) _maps[i] = rhs._maps[i];
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
  if (!legal) {
    string err_msg = "unsafe memory access";
    throw (err_msg);
  }
}

void mem_t::clear() {
  memset(_mem, 0, sizeof(uint8_t)*_mem_size);
  for (int i = 0; i < _maps.size(); i++) _maps[i].clear();
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
void smt_mem::init_addrs_map_v_next_by_layout() {
  for (int i = 0; i < mem_t::maps_number(); i++) {
    unsigned int start_mem_off = mem_t::get_mem_off_by_idx_in_map(i, 0);
    z3::expr start = (_stack_start + to_expr((uint64_t)start_mem_off)).simplify();
    _addrs_map_v_next.push_back(start);
  }
}

z3::expr smt_mem::get_and_update_addr_v_next(int map_id) {
  z3::expr res = _addrs_map_v_next[map_id];
  unsigned int v_sz = mem_t::map_val_sz(map_id) / NUM_BYTE_BITS;
  _addrs_map_v_next[map_id] = _addrs_map_v_next[map_id] + to_expr((uint64_t)v_sz);
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
smt_var::smt_var(unsigned int prog_id, unsigned int node_id, unsigned int num_regs)
  : smt_var_base(prog_id, node_id, num_regs) {
  mem_addr_id = 0;
  is_vaild_id = 0;
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

// constrain: 1. stack_start != NULL
// 2. stack_start + mem_size - 1 <= max_uint64
z3::expr smt_var::stack_start_constrain() const {
  int mem_size = get_mem_size_by_layout();
  z3::expr offset = to_expr((uint64_t)(mem_size - 1));
  z3::expr max_uint64 = to_expr((uint64_t)0xffffffffffffffff);
  z3::expr f = (mem_var._stack_start != NULL_ADDR_EXPR) &&
               uge(max_uint64 - offset, mem_var._stack_start);
  return f;
}

void smt_var::get_from_previous_block(smt_var& sv) {
  mem_var = sv.mem_var;
}

void smt_var::clear() {
  smt_var_base::clear();
  for (size_t i = 0; i < reg_var.size(); i++) {
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

void prog_state::init() {
  _mem.init_mem_by_layout();
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

void inout_t::clear() {
  reg = 0;
  for (int i = 0; i < maps.size(); i++) maps[i].clear();
}

void inout_t::init() {
  reg = 0;
  int n_maps = mem_t::maps_number();
  maps.resize(n_maps);
}

bool inout_t::operator==(const inout_t &rhs) const {
  // cout << "==" << endl;
  // cout << hex << reg << " " << rhs.reg << " " << maps.size() << " " << rhs.maps.size() << dec << endl;
  bool res = (reg == rhs.reg) && (maps.size() == rhs.maps.size());
  // cout << res << endl;
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
  out << x.input_simu_r10 << " ";
  out << x.reg;
  // out << "(hexadecimal)" << endl;
  // out << "register: " << hex << x.reg << dec << endl;
  for (int i = 0; i < x.maps.size(); i++) {
    out << "map " << i << ": ";
    for (auto it = x.maps[i].begin(); it != x.maps[i].end(); it++) {
      out << it->first << "," << uint8_t_vec_2_hex_str(it->second) << " ";
    }
    // out << endl;
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
}

// parameter r10 is the simulated r10, stack_bottom is the real r10
uint64_t get_simu_addr_by_real(uint64_t real_addr, uint64_t simu_r10, uint64_t real_r10) {
  return (real_addr + simu_r10 - real_r10);
}

uint64_t get_real_addr_by_simu(uint64_t simu_addr, uint64_t simu_r10, uint64_t real_r10) {
  return (simu_addr + real_r10 - simu_r10);
}
