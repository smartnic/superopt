#include <random>
#include "inst_var.h"

using namespace std;

mem_layout mem_t::_layout;
vector<vector<register_state>> smt_input::reg_state;
live_variables smt_output::post_prog_r;
vector<z3::expr> smt_var::randoms_u32;
bool smt_var::enable_addr_off = true;
bool smt_var::is_win = false;
int inout_t::start_insn = 0;
int inout_t::end_insn = 0;

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

  if (_skb != nullptr) {
    delete []_pkt;
    _skb = nullptr;
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

unsigned int mem_t::map_type(int map_id) {
  if (map_id >= maps_number()) {
    string err_msg = "map_id > #maps";
    throw (err_msg);
  }
  return _layout._maps_attr[map_id].map_type;
}

unsigned int mem_t::prog_array_map_max_entries() {
  int map_id = -1;
  for (int i = 0; i < maps_number(); i++) {
    if (map_type(i) != MAP_TYPE_prog_array) continue;
    map_id = i;
  }
  if (map_id == -1) return 0; // means no prog_array map
  return map_max_entries(map_id);
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

  if (_skb != nullptr) {
    delete []_skb;
    _skb = nullptr;
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

  _skb = new uint8_t[_layout._skb_max_sz];
  memset(_skb, 0, sizeof(uint8_t)*_layout._skb_max_sz);
}

void mem_t::set_map_attr(int map_id, map_attr m_attr) {
  _layout._maps_attr[map_id] = m_attr;
}

void mem_t::update_stack(int idx, uint8_t v) {
  _mem[idx] = v;
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

uint8_t mem_t::get_stack_val_by_offset(int off) const {
  return _mem[_layout._stack_start + off];
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

uint8_t* mem_t::get_pkt_addr_by_offset(int off) const {
  int pkt_sz = mem_t::_layout._pkt_sz;
  if ((off >= 0) && (off < pkt_sz)) {
    return &_pkt[off];
  }
  string err_msg = "offset " + to_string(off) + " is outside the legal pkt_sz offsets[0, "
                   + to_string(pkt_sz) + ")";
  throw (err_msg);
}

uint8_t* mem_t::get_skb_start_addr() const {
  if (mem_t::_layout._skb_max_sz == 0) return nullptr;
  return &_skb[0];
}

uint8_t* mem_t::get_skb_end_addr() const {
  if (mem_t::_layout._skb_max_sz == 0) return nullptr;
  int skb_sz = get_skb_sz();
  return &_skb[skb_sz - 1];
}

uint8_t* mem_t::get_skb_addr_by_offset(int off) const {
  int skb_sz = get_skb_sz();
  if ((off >= 0) && (off < skb_sz)) {
    return &_skb[off];
  }
  string err_msg = "offset " + to_string(off) + " is outside the legal skb_sz offsets[0, "
                   + to_string(skb_sz) + ")";
  throw (err_msg);
}

uint32_t mem_t::get_skb_sz() const {// real skb sz
  if (mem_t::_layout._skb_max_sz == 0) return 0;
  return (_simu_skb_e - _simu_skb_s + 1);
}

uint32_t* mem_t::get_pkt_ptrs_start_addr() {
  if (mem_t::get_pgm_input_type() != PGM_INPUT_pkt_ptrs) return nullptr;
  return &_pkt_ptrs[0];
}

uint32_t* mem_t::get_pkt_ptrs_end_addr() {
  if (mem_t::get_pgm_input_type() != PGM_INPUT_pkt_ptrs) return nullptr;
  return &_pkt_ptrs[1] + sizeof(uint32_t) - 1;
}

uint64_t mem_t::get_simu_mem_start_addr() {
  return _simu_mem_s;
}

uint64_t mem_t::get_simu_mem_end_addr() {
  return (_simu_mem_s + _mem_size - 1);
}

uint64_t mem_t::get_simu_pkt_start_addr() {
  int pgm_input_type = mem_t::get_pgm_input_type();
  if ((pgm_input_type == PGM_INPUT_pkt) || (pgm_input_type == PGM_INPUT_skb))
    return _simu_pkt_s;
  else if (pgm_input_type == PGM_INPUT_pkt_ptrs) return _pkt_ptrs[0];
  else return NULL_ADDR;
}

uint64_t mem_t::get_simu_pkt_end_addr() {
  int pgm_input_type = mem_t::get_pgm_input_type();
  if ((pgm_input_type == PGM_INPUT_pkt) || (pgm_input_type == PGM_INPUT_skb))
    return (_simu_pkt_s + mem_t::_layout._pkt_sz - 1);
  else if (pgm_input_type == PGM_INPUT_pkt_ptrs) return _pkt_ptrs[1];
  else return NULL_ADDR;
}

uint32_t mem_t::get_simu_skb_start_addr() {
  int pgm_input_type = mem_t::get_pgm_input_type();
  if (pgm_input_type != PGM_INPUT_skb) return NULL_ADDR;
  assert(mem_t::_layout._pkt_sz >= (SKB_data_s_off + sizeof(uint32_t)));
  return *(uint32_t*)&_pkt[SKB_data_s_off];
}

uint32_t mem_t::get_simu_skb_end_addr() {
  int pgm_input_type = mem_t::get_pgm_input_type();
  if (pgm_input_type != PGM_INPUT_skb) return NULL_ADDR;
  assert(mem_t::_layout._pkt_sz >= (SKB_data_e_off + sizeof(uint32_t)));
  return *(uint32_t*)&_pkt[SKB_data_e_off];
}

uint64_t mem_t::get_simu_pkt_ptrs_start_addr() {
  int pgm_input_type = mem_t::get_pgm_input_type();
  if (pgm_input_type == PGM_INPUT_pkt_ptrs) return _simu_pkt_ptrs_s;
  else return NULL_ADDR;
}

uint64_t mem_t::get_simu_pkt_ptrs_end_addr() {
  int pgm_input_type = mem_t::get_pgm_input_type();
  int pkt_ptrs_sz = 8; // 8 bytes, two 32-bit address
  if (pgm_input_type == PGM_INPUT_pkt_ptrs) return (_simu_pkt_ptrs_s + pkt_ptrs_sz - 1);
  else return NULL_ADDR;
}

mem_t& mem_t::operator=(const mem_t &rhs) {
  memcpy(_mem, rhs._mem, sizeof(uint8_t)*_mem_size);
  for (int i = 0; i < rhs._maps.size(); i++) _maps[i] = rhs._maps[i];
  memcpy(_pkt, rhs._pkt, sizeof(uint8_t)*_layout._pkt_sz);
  memcpy(_skb, rhs._skb, sizeof(uint8_t)*_layout._skb_max_sz);
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
  for (int i = 0; i < _layout._skb_max_sz; i++) {
    if (_skb[i] != rhs._skb[i]) return false;
  }
  return true;
}

void mem_t::clear() {
  memset(_mem, 0, sizeof(uint8_t)*_mem_size);
  for (int i = 0; i < _maps.size(); i++) _maps[i].clear();
  memset(_pkt, 0, sizeof(uint8_t)*_layout._pkt_sz);
  memset(_skb, 0, sizeof(uint8_t)*_layout._skb_max_sz);
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
  block.clear();
  is_valid.clear();
  addr.clear();
  val.clear();
  for (int i = 0; i < rhs.addr.size(); i++) {
    add(rhs.block[i], rhs.is_valid[i], rhs.addr[i], rhs.val[i]);
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
    out << i << ": " << s.block[i] << " " << s.is_valid[i] << " "
        << s.addr[i].simplify() << " " << s.val[i].simplify() << endl;
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
  // out << "pointers: " << endl;
  // for (auto it = m._ptrs.begin(); it != m._ptrs.end(); it++) {
  //   out << "pointer id: " << it->first << ", pc: " << it->second[0] << endl;
  // }
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
  int pgm_input_type = mem_t::get_pgm_input_type();
  if (pgm_input_type == PGM_INPUT_pkt_ptrs) n_mem_tables++;
  if (mem_t::_layout._pkt_sz > 0) n_mem_tables++;
  if (mem_t::_layout._skb_max_sz > 0) n_mem_tables++;
  _mem_tables.resize(n_mem_tables);

  int i = 0;
  _mem_tables[i]._type = MEM_TABLE_stack;
  i++;
  if (pgm_input_type == PGM_INPUT_pkt_ptrs) {
    _mem_tables[i]._type = MEM_TABLE_pkt_ptrs;
    i++;
  }
  if (mem_t::_layout._pkt_sz > 0) {
    _mem_tables[i]._type = MEM_TABLE_pkt;
    i++;
  }
  if (mem_t::_layout._skb_max_sz > 0) {
    _mem_tables[i]._type = MEM_TABLE_skb;
    i++;
  }
  for (int map_id = 0; i < _mem_tables.size(); i++, map_id++) {
    _mem_tables[i]._type = MEM_TABLE_map;
    _mem_tables[i]._map_id = map_id;
  }
  _map_tables.resize(n_maps);

  if (pgm_input_type == PGM_INPUT_pkt) {
    _pkt_off = to_expr((uint64_t)(mem_t::_layout._pkt_sz - 1));
  }
}

z3::expr smt_mem::get_and_update_addr_v_next(int map_id) {
  z3::expr res = _addrs_map_v_next[map_id];
  unsigned int v_sz = mem_t::map_val_sz(map_id) / NUM_BYTE_BITS;
  _addrs_map_v_next[map_id] = _addrs_map_v_next[map_id] + to_expr((uint64_t)v_sz);
  return res;
}

void smt_mem::get_mem_ptr_info(vector<int>& table_ids, vector<mem_ptr_info>& ptr_info_list, z3::expr ptr_expr) {
  table_ids.clear();
  ptr_info_list.clear();
  for (int i = 0; i < _mem_tables.size(); i++) {
    if (! _mem_tables[i].is_ptr_in_ptrs(ptr_expr)) continue;
    mem_ptr_info ptr_info = _mem_tables[i].get_ptr_info(ptr_expr);
    table_ids.push_back(i);
    ptr_info_list.push_back(ptr_info);
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

void smt_mem::add_in_mem_table_wt(int mem_table_id, unsigned int block,
                                  z3::expr is_valid, z3::expr addr, z3::expr val) {
  assert(mem_table_id >= 0);
  assert(mem_table_id < _mem_tables.size());
  _mem_tables[mem_table_id]._wt.add(block, is_valid, addr, val);
}

void smt_mem::add_in_mem_table_urt(int mem_table_id, unsigned int block,
                                   z3::expr is_valid, z3::expr addr, z3::expr val) {
  assert(mem_table_id >= 0);
  assert(mem_table_id < _mem_tables.size());
  _mem_tables[mem_table_id]._urt.add(block, is_valid, addr, val);
}

void smt_mem::add_ptr(z3::expr ptr_expr, int table_id, z3::expr off, z3::expr path_cond) {
  assert(table_id >= 0);
  assert(table_id < _mem_tables.size());
  _mem_tables[table_id].add_ptr(ptr_expr, path_cond, off);
}

void smt_mem::add_ptr(z3::expr ptr_expr, z3::expr ptr_from_expr, z3::expr ptr_minus_ptr_from, z3::expr path_cond) {
  unsigned int ptr_from_id = ptr_from_expr.id();
  for (int i = 0; i < _mem_tables.size(); i++) {
    auto found = _mem_tables[i]._ptrs.find(ptr_from_id);
    if (found != _mem_tables[i]._ptrs.end()) {
      z3::expr off = found->second.off + ptr_minus_ptr_from;
      _mem_tables[i].add_ptr(ptr_expr, path_cond, off);
    }
  }
}

void smt_mem::add_ptr_by_map_id(z3::expr ptr_expr, int map_id, z3::expr path_cond) {
  int table_id = get_mem_table_id(MEM_TABLE_map, map_id);
  if (table_id != -1) {
    // for map memory table, addr_off won't be used. so set it as zero
    _mem_tables[table_id].add_ptr(ptr_expr, path_cond, ZERO_ADDR_OFF_EXPR);
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

void live_variables::intersection(live_variables& lv, const live_variables& lv1, const live_variables& lv2) {
  lv.clear();
  // process live registers
  const unordered_set<int>& live_reg1 = lv1.regs;
  const unordered_set<int>& live_reg2 = lv2.regs;
  for (auto reg : lv1.regs) {
    if (lv2.regs.find(reg) != lv1.regs.end()) {
      lv.regs.insert(reg);
    }
  }

  // process live memory
  for (auto it1 = lv1.mem.begin(); it1 != lv1.mem.end(); it1++) {
    int type = it1->first;
    auto it2 = lv2.mem.find(type);
    if (it2 == lv2.mem.end()) continue;
    unordered_set<int> offs;
    const unordered_set<int>& off_set1 = it1->second;
    const unordered_set<int>& off_set2 = it2->second;
    for (auto off : off_set1) {
      if (off_set2.find(off) == off_set2.end()) continue;
      offs.insert(off);
    }
    lv.mem[type] = offs;
  }
}

ostream& operator<<(ostream& out, const live_variables& x) {
  out << "regs: ";
  for (auto reg : x.regs) out << reg << " ";
  out << endl;
  out << "mem: " << endl;;
  for (auto it = x.mem.begin(); it != x.mem.end(); it++) {
    out << it->first << ":";
    for (auto off : it->second) out << off << " ";
    out << endl;
  }
  return out;
}

z3::expr smt_input::input_constraint() {
  z3::expr f = Z3_true;
  for (auto reg : prog_read.regs) {
    if (reg_state[reg].size() == 0) {
      string err_msg = "prog_read r" + to_string(reg) + " is not in input";
      throw err_msg;
    }

    // constraint of register path condition
    vector<z3::expr> path_conds;
    for (int i = 0; i < reg_state[reg].size(); i++) {
      z3::expr reg_pc = reg_path_cond(reg, i);
      path_conds.push_back(reg_pc);
    }
    // 1. at least one path condition is true
    z3::expr f_pc_true = Z3_false;
    for (int i = 0; i < path_conds.size(); i++) {
      f_pc_true = f_pc_true || path_conds[i];
    }
    // 2. at most one path condition is true
    z3::expr f_pc = Z3_true;
    for (int i = 0; i < path_conds.size(); i++) {
      z3::expr f_pc_i = Z3_true;
      for (int j = 0; j < path_conds.size(); j++) {
        if (j == i) continue;
        f_pc_i = f_pc_i && (! path_conds[j]);
      }
      f_pc = f_pc && z3::implies(path_conds[i], f_pc_i);
    }

    f = f && f_pc_true && f_pc;
  }

  // add constraints to input register (constant registers, type: SCALAR_VALUE)
  z3::expr f_const_reg = Z3_true;
  for (auto reg : prog_read.regs) {
    if (reg_state[reg].size() == 0) {
      string err_msg = "r" + to_string(reg) + "in prog_read is not in input";
      throw err_msg;
    }
    for (int i = 0; i < reg_state[reg].size(); i++) {
      int type = reg_state[reg][i].type;
      bool val_flag = reg_state[reg][i].val_flag;
      int64_t val = reg_state[reg][i].val;
      // only deal with constant register
      if ((type != SCALAR_VALUE) || (! val_flag)) continue;
      z3::expr pc = reg_path_cond(reg, i);
      z3::expr f_reg = (reg_expr(reg) == to_expr(val));
      f_const_reg = f_const_reg && z3::implies(pc, f_reg);
    }
  }
  f = f && f_const_reg;
  return f;
}

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
  var_cur_id = 0;
  rand_u32_cur_id = 0;
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
  var_cur_id = 0;
  rand_u32_cur_id = 0;
  smt_out.set_pgm_id(prog_id);
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

z3::expr smt_var::new_var(unsigned int bit_sz) {
  var_cur_id++;
  string name = "var_" + _name + "_" + to_string(var_cur_id);
  return to_expr(name, bit_sz);
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
  if (mem_t::maps_number() == 0) return;
  assert(map_id_expr.is_numeral());
  int map_id = map_id_expr.get_numeral_uint64();
  add_expr_map_id(e, map_id, path_cond);
}

void smt_var::add_expr_map_id(z3::expr e, int map_id, z3::expr path_cond) {
  // cout << "add map_id: reg:" << e << ", map_id:" << map_id << ", pc:" << path_cond << endl;
  if (mem_t::maps_number() == 0) return;
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
// 3. pgm input type == PGM_INPUT_pkt_ptrs: pkt address is 32-bit
// 0 < [pkt start, pkt end] < [memory start, memory end] < {pkt start pointer, pkt end pointer} <= max_uint64
// {pkt start pointer, pkt end pointer} takes 2 bytes
z3::expr smt_var::mem_layout_constrain() const {
  z3::expr mem_start = mem_var._stack_start;
  int mem_sz = get_mem_size_by_layout();
  z3::expr mem_off = to_expr((uint64_t)(mem_sz - 1));
  z3::expr pkt_start = mem_var._pkt_start;
  z3::expr pkt_off = mem_var._pkt_off;
  z3::expr pkt_start_ptr_addr = get_pkt_start_ptr_addr();
  int pkt_ptrs_off = 8 - 1; // two elements: pkt start pointer and pkt end pointer
  z3::expr max_uint64 = to_expr((uint64_t)0xffffffffffffffff);
  z3::expr max_uint32 = to_expr((uint64_t)0xffffffff);

  int pgm_input_type = mem_t::get_pgm_input_type();
  if (pgm_input_type == PGM_INPUT_constant) {
    z3::expr f = (mem_start != NULL_ADDR_EXPR) &&
                 uge(max_uint64 - mem_off, mem_start);
    return f;
  } else if (pgm_input_type == PGM_INPUT_pkt) {
    z3::expr f = (mem_start != NULL_ADDR_EXPR) &&
                 ugt(pkt_start, mem_off) && ugt(pkt_start - mem_off, mem_start) &&
                 uge(max_uint64 - pkt_off, pkt_start);
    return f;
  } else if (pgm_input_type == PGM_INPUT_pkt_ptrs) {
    z3::expr max_pkt_off = to_expr((uint64_t)mem_t::_layout._pkt_sz - 1);
    z3::expr f = (pkt_start.extract(63, 32) == NULL_ADDR_EXPR.extract(63, 32)) && // pkt address is 32-bit
                 (pkt_start.extract(31, 0) != NULL_ADDR_EXPR.extract(31, 0)) &&  // pkt address is not NULL
                 uge(max_pkt_off, pkt_off) && uge(max_uint32 - pkt_off, pkt_start) &&
                 ugt(mem_start, pkt_off) && ugt(mem_start - pkt_off, pkt_start) && // mem_start > pkt_end
                 ugt(pkt_start_ptr_addr, mem_off) && ugt(pkt_start_ptr_addr - mem_off, mem_start) && // pkt_start_ptr_addr > mem_end
                 uge(max_uint64 - pkt_ptrs_off, pkt_start_ptr_addr); // max_uint64 > pkt_end_ptr_addr
    return f;
  } else if (pgm_input_type == PGM_INPUT_skb) {
    z3::expr skb_start = mem_var._skb_start;
    z3::expr skb_end = mem_var._skb_end;
    z3::expr skb_off = mem_var._skb_end - mem_var._skb_start;
    z3::expr skb_max_off = to_expr((uint64_t)mem_t::_layout._skb_max_sz - 1);
    z3::expr f = (skb_start.extract(63, 32) == NULL_ADDR_EXPR.extract(63, 32)) && // skb start address is 32-bit
                 (skb_start.extract(31, 0) != NULL_ADDR_EXPR.extract(31, 0)) &&  // skb start address is not NULL
                 (skb_end.extract(63, 32) == NULL_ADDR_EXPR.extract(63, 32)) &&
                 uge(skb_end, skb_start) && uge(skb_max_off, skb_off) &&
                 ugt(pkt_start, skb_end) &&
                 ugt(mem_start, pkt_off) && ugt(mem_start - pkt_off, pkt_start) &&
                 uge(max_uint64 - mem_off, mem_start);
    return f;
  }
  return Z3_true;
}

void smt_var::set_new_node_id(unsigned int node_id, const vector<unsigned int>& nodes_in,
                              const vector<z3::expr>& node_in_pc_list,
                              const vector<vector<z3::expr>>& nodes_in_regs) {
  // cout << "set_new_node_id block:" << node_id << endl;
  // set the register names first, use names later
  smt_var_base::set_new_node_id(node_id, nodes_in, node_in_pc_list, nodes_in_regs);
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
    // cout << "cur_reg: " << cur_reg << endl;
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
          assert(id < map_id_pcs.size());
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
    vector<mem_ptr_info> infos(mem_var._mem_tables.size());
    for (int j = 0; j < nodes_in.size(); j++) {
      z3::expr reg_ptr = nodes_in_regs[nodes_in[j]][i];
      vector<int> ids;
      vector<mem_ptr_info> info_list;
      mem_var.get_mem_ptr_info(ids, info_list, reg_ptr);
      for (int k = 0; k < ids.size(); k++) {
        int id = ids[k];
        // merge path condition for the same table id
        infos[id].path_cond = infos[id].path_cond || (info_list[k].path_cond && node_in_pc_list[j]);
        infos[id].off = info_list[k].off;
        // cout << "******   " << j << " " << nodes_in[j] << " off: " << infos[id].off << endl;
        if (! table_ids[id]) {
          table_ids[id] = true;
        }
      }
    }
    // update mem_table's pointers
    for (int j = 0; j < table_ids.size(); j++) {
      if (! table_ids[j]) continue;
      mem_var.add_ptr(cur_reg, j, infos[j].off, infos[j].path_cond);
    }
  }
}

void smt_var::init(unsigned int prog_id, unsigned int node_id, unsigned int num_regs, unsigned int n_blocks, bool is_win) {
  smt_var_base::init(prog_id, node_id, num_regs);
  mem_var.init_by_layout(n_blocks);
  int root = 0;
  if (node_id != root) return;
  smt_out.set_pgm_id(prog_id);
  if (is_win) return;
  // todo: move the init of root node into inst::smt_set_pre?
  int stack_mem_table_id = mem_var.get_mem_table_id(MEM_TABLE_stack);
  mem_var.add_ptr(get_init_reg_var(10), stack_mem_table_id, to_expr(STACK_SIZE), Z3_true); // r10 is the stack pointer
  int pgm_input_type = mem_t::get_pgm_input_type();
  if ((pgm_input_type == PGM_INPUT_pkt) || (pgm_input_type == PGM_INPUT_skb)) {
    int pkt_mem_table_id = mem_var.get_mem_table_id(MEM_TABLE_pkt);
    mem_var.add_ptr(get_init_reg_var(1), pkt_mem_table_id, ZERO_ADDR_OFF_EXPR, Z3_true); // r1 is the pkt pointer
  }
  // deal with the case that input is packet pointers
  // init the memory table of packet pointers
  // pkt_start = *(u32 *)(r1 + 0); pkt_end(=pkt_start+pkt_off) = *(u32 *)(r1 + 0);
  if (pgm_input_type == PGM_INPUT_pkt_ptrs) {
    int mem_table_id = mem_var.get_mem_table_id(MEM_TABLE_pkt_ptrs);
    mem_var.add_ptr(get_init_reg_var(1), mem_table_id, ZERO_ADDR_OFF_EXPR, Z3_true);
    z3::expr pkt_start = mem_var._pkt_start;
    z3::expr pkt_end = mem_var._pkt_start + mem_var._pkt_off;
    for (int i = 0; i < 4; i++) { // the first 4 bytes are the address of pkt start
      mem_var.add_in_mem_table_urt(mem_table_id, root, Z3_true, to_expr((int64_t)i),
                                   pkt_start.extract(8 * i + 7, 8 * i));
    }
    for (int i = 0; i < 4; i++) { // the last 4 bytes are the address of pkt end
      mem_var.add_in_mem_table_urt(mem_table_id, root, Z3_true, to_expr((int64_t)(i + 4)),
                                   pkt_end.extract(8 * i + 7, 8 * i));
    }
  }
  if (pgm_input_type == PGM_INPUT_skb) { // add skb data start/end into pkt table
    int mem_table_id = mem_var.get_mem_table_id(MEM_TABLE_pkt);
    z3::expr skb_start = mem_var._skb_start;
    z3::expr skb_end = mem_var._skb_end;
    for (int i = 0; i < 4; i++) {
      mem_var.add_in_mem_table_urt(mem_table_id, root, Z3_true, to_expr(SKB_data_s_off + (int64_t)i),
                                   skb_start.extract(8 * i + 7, 8 * i));
    }
    for (int i = 0; i < 4; i++) {
      mem_var.add_in_mem_table_urt(mem_table_id, root, Z3_true, to_expr(SKB_data_e_off + (int64_t)(i + 4)),
                                   skb_end.extract(8 * i + 7, 8 * i));
    }
  }
}

void smt_var::init_static_variables() {
  randoms_u32.clear();
  for (int i = 0; i < mem_t::_layout._n_randoms_u32; i++) {
    string name = "rand_u32_" + to_string(i);
    randoms_u32.push_back(to_expr(name, 32));
  }
}

z3::expr smt_var::get_next_random_u32() {
  assert(rand_u32_cur_id < randoms_u32.size());
  z3::expr rand_u32 = randoms_u32[rand_u32_cur_id];
  rand_u32_cur_id++;
  return rand_u32;
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
    var_cur_id = 0;
    rand_u32_cur_id = 0;
  }
  mem_var.clear();
  expr_map_id.clear();
}

/* class smt_var end */

smt_var_bl::smt_var_bl() {
  int n_maps =  mem_t::maps_number();
  int n_mem_tables = 1 + n_maps; // stack + maps
  if (mem_t::get_pgm_input_type() == PGM_INPUT_pkt_ptrs) n_mem_tables++;
  if (mem_t::_layout._pkt_sz > 0) n_mem_tables++;
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
      f = f && z3::implies(!pc, sv.mem_var._mem_tables[i]._wt.is_valid[j] == Z3_false);
    }
    for (int j = _mem_urt_sz[i]; j < sv.mem_var._mem_tables[i]._urt.size(); j++) {
      f = f && z3::implies(!pc, sv.mem_var._mem_tables[i]._urt.is_valid[j] == Z3_false);
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

prog_state::prog_state() {
  _regs.resize(NUM_REGS, 0);
  init_safety_chk();
}

void prog_state::init_safety_chk() {
  _reg_readable.resize(NUM_REGS, false);
  _reg_readable[1] = true; // r1 and r10 are in the program input
  _reg_readable[10] = true;
  _stack_readable.resize(STACK_SIZE, false);
  _reg_type.resize(NUM_REGS, SCALAR_VALUE);
  set_reg_type(1, PTR_TO_CTX);
  set_reg_type(10, PTR_TO_STACK);
}

void prog_state::init_safety_chk(const vector<bool>& reg_readable, const vector<bool>& stack_readable, const vector<int>& reg_type) {
  _reg_readable.resize(NUM_REGS);
  for (int i = 0; i < _reg_readable.size(); i++) _reg_readable[i] = reg_readable[i];
  _stack_readable.resize(STACK_SIZE);
  for (int i = 0; i < _stack_readable.size(); i++) _stack_readable[i] = stack_readable[i];
  _reg_type.resize(NUM_REGS);
  for (int i = 0; i < _reg_type.size(); i++) _reg_type[i] = reg_type[i];
}

void prog_state::reg_safety_chk(int reg_write, vector<int> reg_read_list) {
  // check whether reg_read is readable first, then set the reg_write is readable
  for (int i = 0; i < reg_read_list.size(); i++) {
    if (_reg_readable[reg_read_list[i]]) continue;
    string err_msg = "register " + to_string(reg_read_list[i]) + " is not readable.";
    throw (err_msg);
  }
  _reg_readable[reg_write] = true;
}

void prog_state::init() {
  _mem.init_by_layout();
  init_safety_chk();
  _randoms_u32.resize(mem_t::_layout._n_randoms_u32);
}

// memory_access_chk is used to avoid segmentation fault: If memory access not in the legal range, throw error
// safe address: [get_mem_start_addr(), get_mem_end_addr()]
void prog_state::memory_access_chk(uint64_t addr, uint64_t num_bytes) {
  // to avoid overflow
  uint64_t start = (uint64_t)_mem.get_mem_start_addr();
  uint64_t end = (uint64_t)_mem.get_mem_end_addr();
  uint64_t max_uint64 = 0xffffffffffffffff;
  bool legal = (addr >= start) && (addr + num_bytes - 1 <= end) &&
               (addr <= (max_uint64 - num_bytes + 1));
  if (legal) return;

  if (mem_t::_layout._pkt_sz == 0) {
    legal = false;
  } else {
    start = (uint64_t)_mem.get_pkt_start_addr();
    end = (uint64_t)_mem.get_pkt_end_addr();
    legal = (addr >= start) && (addr + num_bytes - 1 <= end) &&
            (addr <= (max_uint64 - num_bytes + 1));
  }
  if (legal) return;

  int pgm_input_type = mem_t::get_pgm_input_type();
  if (pgm_input_type == PGM_INPUT_pkt_ptrs) {
    start = (uint64_t)_mem.get_pkt_ptrs_start_addr();
    end = (uint64_t)_mem.get_pkt_ptrs_end_addr();
    legal = (addr >= start) && (addr + num_bytes - 1 <= end) &&
            (addr <= (max_uint64 - num_bytes + 1));
  }
  if (legal) return;

  if (mem_t::_layout._skb_max_sz == 0) {
    legal = false;
  } else {
    start = (uint64_t)_mem.get_skb_start_addr();
    end = (uint64_t)_mem.get_skb_end_addr();
    legal = (addr >= start) && (addr + num_bytes - 1 <= end) &&
            (addr <= (max_uint64 - num_bytes + 1));
  }

  if (!legal) {
    string err_msg = "unsafe memory access";
    throw (err_msg);
  }
}

// memory_access_and_safety_chk is used to
// 1. avoid segmentation fault: If memory access not in the legal range, throw error
// 2. stack read before write check
void prog_state::memory_access_and_safety_chk(uint64_t addr, uint64_t num_bytes, bool chk_safety, bool is_read, bool stack_aligned_chk) {
  memory_access_chk(addr, num_bytes);
  if (! chk_safety) return;
  // stack read before write check
  uint64_t start = (uint64_t)_mem.get_stack_start_addr();
  uint64_t end = (uint64_t)_mem.get_stack_bottom_addr() - 1;
  uint64_t max_uint64 = 0xffffffffffffffff;
  bool is_stack_addr = (addr >= start) && (addr <= end);
  if (! is_stack_addr) return;
  bool is_legal_range = (addr + num_bytes - 1 <= end) &&
                        (addr <= (max_uint64 - num_bytes + 1));
  if (! is_legal_range) {
    string err_msg = "offset > stack range";
    throw (err_msg);
  }

  int idx_s = addr - start;
  if (is_read) {
    for (int i = 0; i < num_bytes; i++) {
      if (_stack_readable[idx_s + i]) continue;
      string err_msg = "stack[" + to_string(idx_s + i) + "] is not readable.";
      throw (err_msg);
    }
  } else {
    for (int i = 0; i < num_bytes; i++) _stack_readable[idx_s + i] = true;
  }
  if (stack_aligned_chk) {
    // stack address should be aligned
    uint64_t stack_bottom = (uint64_t)_mem.get_stack_bottom_addr();
    uint64_t remainder = (stack_bottom - addr) % num_bytes;
    if (remainder != 0) {
      string err_msg = "stack access is not aligned";
      throw (err_msg);
    }
  }
}

int prog_state::get_reg_type(int reg) const {
  bool legal = (reg >= 0) && (reg < NUM_REGS);
  assert(legal);
  return _reg_type[reg];
}

void prog_state::set_reg_type(int reg, int type) {
  bool legal = (reg >= 0) && (reg < NUM_REGS) &&
               (type >= SCALAR_VALUE) && (type < MAX_REG_TYPE);
  assert(legal);
  _reg_type[reg] = type;
}

void prog_state::print() const {
  prog_state_base::print();
  cout << "Memory:" << endl;
  cout << _mem << endl;
}

void prog_state::clear() {
  prog_state_base::clear();
  _mem.clear();
  _reg_readable.clear();
  _stack_readable.clear();
  _reg_type.clear();
  for (int i = 0; i < _randoms_u32.size(); i++) _randoms_u32[i] = 0;
  _cur_randoms_u32_idx = 0;
}

uint32_t prog_state::get_next_random_u32() {
  assert(_cur_randoms_u32_idx < _randoms_u32.size());
  uint32_t rand = _randoms_u32[_cur_randoms_u32_idx];
  _cur_randoms_u32_idx++;
  return rand;
}

inout_t::inout_t() {
  uint64_t r10_min = STACK_SIZE;
  uint64_t r10_max = 0xffffffffffffffff - get_mem_size_by_layout() + 1 - STACK_SIZE;
  input_simu_r10 = r10_min + (r10_max - r10_min) * unidist_ebpf_inst_var(gen_ebpf_inst_var);
  pkt = new uint8_t[mem_t::_layout._pkt_sz];
  memset(pkt, 0, sizeof(uint8_t)*mem_t::_layout._pkt_sz);
  skb = new uint8_t[mem_t::_layout._skb_max_sz];
  memset(skb, 0, sizeof(uint8_t)*mem_t::_layout._skb_max_sz);
  randoms_u32.resize(mem_t::_layout._n_randoms_u32);
  reg_readable.resize(NUM_REGS);
  stack_readble.resize(STACK_SIZE);
  reg_type.resize(NUM_REGS);
  maps_mem.resize(mem_t::maps_number());
  for (int i = 0; i < maps_mem.size(); i++) {
    int max_num = mem_t::map_max_entries(i) * mem_t::map_val_sz(i) / NUM_BYTE_BITS;
    maps_mem[i].resize(max_num);
  }
}

// deep copy for vector push back
inout_t::inout_t(const inout_t& rhs) {
  pkt = new uint8_t[mem_t::_layout._pkt_sz];
  input_simu_pkt_s = rhs.input_simu_pkt_s;
  input_simu_r10 = rhs.input_simu_r10;
  reg = rhs.reg;
  maps = rhs.maps;
  memcpy(pkt, rhs.pkt, sizeof(uint8_t)*mem_t::_layout._pkt_sz);
  skb = new uint8_t[mem_t::_layout._skb_max_sz];
  memcpy(skb, rhs.skb, sizeof(uint8_t)*mem_t::_layout._skb_max_sz);
  tail_call_para = rhs.tail_call_para;
  pgm_exit_type = rhs.pgm_exit_type;
  randoms_u32.resize(mem_t::_layout._n_randoms_u32);
  for (int i = 0; i < randoms_u32.size(); i++) {
    randoms_u32[i] = rhs.randoms_u32[i];
  }
  is_win = rhs.is_win;
  reg_readable.resize(NUM_REGS);
  stack_readble.resize(STACK_SIZE);
  reg_type.resize(NUM_REGS);
  for (int i = 0; i < reg_readable.size(); i++) reg_readable[i] = rhs.reg_readable[i];
  for (int i = 0; i < stack_readble.size(); i++) stack_readble[i] = rhs.stack_readble[i];
  for (int i = 0; i < reg_type.size(); i++) reg_type[i] = rhs.reg_type[i];
  regs = rhs.regs;
  stack = rhs.stack;
  maps_mem.resize(rhs.maps_mem.size());
  for (int i = 0; i < maps_mem.size(); i++) {
    maps_mem[i].resize(rhs.maps_mem[i].size());
    for (int j = 0; j < maps_mem[i].size(); j++) {
      maps_mem[i][j] = rhs.maps_mem[i][j];
    }
  }
}

inout_t::~inout_t() {
  if (pkt != nullptr) {
    delete []pkt;
    pkt = nullptr;
  }
  if (skb != nullptr) {
    delete []skb;
    skb = nullptr;
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
  if (! mem_t::_layout._enable_pkt_random_val) return;
  for (int i = 0; i < mem_t::_layout._pkt_sz; i++) {
    uint8_t val = (0xff + 1) * unidist_ebpf_inst_var(gen_ebpf_inst_var);
    pkt[i] = val;
  }
  if (mem_t::get_pgm_input_type() == PGM_INPUT_skb) {
    uint64_t max_uint32 = 0xffffffff;
    uint32_t skb_min_start = 1;
    uint32_t skb_max_start = max_uint32 - mem_t::_layout._skb_max_sz;
    uint32_t skb_rand_start = skb_min_start +
                              (skb_max_start - skb_min_start) * unidist_ebpf_inst_var(gen_ebpf_inst_var);

    uint32_t skb_max_end = skb_rand_start + mem_t::_layout._skb_max_sz;
    uint32_t skb_rand_end = skb_max_end * unidist_ebpf_inst_var(gen_ebpf_inst_var);

    *(uint32_t*)&pkt[SKB_data_s_off] = skb_rand_start;
    *(uint32_t*)&pkt[SKB_data_e_off] = skb_rand_end;
  }
}

void inout_t::set_skb_random_val() {
  for (int i = 0; i < mem_t::_layout._skb_max_sz; i++) {
    uint8_t val = (0xff + 1) * unidist_ebpf_inst_var(gen_ebpf_inst_var);
    skb[i] = val;
  }
}

void inout_t::set_randoms_u32() {
// generate random values
  uint64_t max_uint32 = 0xffffffff;
  for (int i = 0; i < randoms_u32.size(); i++) {
    randoms_u32[i] = (uint64_t)(max_uint32 + 1) * unidist_ebpf_inst_var(gen_ebpf_inst_var);
  }
}

void inout_t::clear() {
  reg = 0;
  for (int i = 0; i < maps.size(); i++) maps[i].clear();
  reg_readable.clear();
  stack_readble.clear();
  reg_type.clear();
  regs.clear();
  stack.clear();
}

void inout_t::init() {
  reg = 0;
  int n_maps = mem_t::maps_number();
  maps.resize(n_maps);
  tail_call_para = -1;
  pgm_exit_type = PGM_EXIT_TYPE_default;
  int n_randoms = mem_t::_layout._n_randoms_u32;
  randoms_u32.resize(mem_t::_layout._n_randoms_u32);
  maps_mem.resize(mem_t::maps_number());
  for (int i = 0; i < maps_mem.size(); i++) {
    int max_num = mem_t::map_max_entries(i) * mem_t::map_val_sz(i) / NUM_BYTE_BITS;
    maps_mem[i].resize(max_num);
  }
}

void inout_t::operator=(const inout_t &rhs) {
  input_simu_pkt_s = rhs.input_simu_pkt_s;
  input_simu_r10 = rhs.input_simu_r10;
  reg = rhs.reg;
  maps = rhs.maps;
  memcpy(pkt, rhs.pkt, sizeof(uint8_t)*mem_t::_layout._pkt_sz);
  memcpy(skb, rhs.skb, sizeof(uint8_t)*mem_t::_layout._skb_max_sz);
  pgm_exit_type = rhs.pgm_exit_type;
  tail_call_para = rhs.tail_call_para;
  for (int i = 0; i < randoms_u32.size(); i++) {
    randoms_u32[i] = rhs.randoms_u32[i];
  }
  is_win = rhs.is_win;
  for (int i = 0; i < reg_readable.size(); i++) reg_readable[i] = rhs.reg_readable[i];
  for (int i = 0; i < stack_readble.size(); i++) stack_readble[i] = rhs.stack_readble[i];
  for (int i = 0; i < reg_type.size(); i++) reg_type[i] = rhs.reg_type[i];
  regs = rhs.regs;
  stack = rhs.stack;
  input_simu_pkt_ptrs[0] = rhs.input_simu_pkt_ptrs[0];
  input_simu_pkt_ptrs[1] = rhs.input_simu_pkt_ptrs[1];
  if (smt_var::is_win) {
    for (int i = 0; i < maps_mem.size(); i++) {
      for (int j = 0; j < maps_mem[i].size(); j++) {
        maps_mem[i][j] = rhs.maps_mem[i][j];
      }
    }
  }
}

bool inout_t::operator==(const inout_t &rhs) const {
  bool res = (maps.size() == rhs.maps.size());
  if (! res) return false;
  for (int i = 0; i < maps.size(); i++) {
    if (maps[i].size() != rhs.maps[i].size()) return false;
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

  for (int i = 0; i < mem_t::_layout._pkt_sz; i++) {
    if (pkt[i] != rhs.pkt[i]) return false;
  }

  if (! smt_var::is_win) {
    if (pgm_exit_type != rhs.pgm_exit_type) return false;
    // now two outputs have the same program exit type
    int output_type = pgm_exit_type;
    if (output_type == PGM_EXIT_TYPE_default) {
      if (reg != rhs.reg) return false;
    } else if (output_type == MAP_TYPE_prog_array) {
      if (tail_call_para != rhs.tail_call_para) return false;
    } else {
      assert(false); // program has bug
    }
  }

  if (smt_var::is_win) {
    // compare registers
    for (auto it1 = regs.begin(); it1 != regs.end(); it1++) {
      int reg = it1->first;
      auto it2 = rhs.regs.find(reg);
      assert(it2 != rhs.regs.end()); // all register in V_post_r are in output
      if (it1->second != it2->second) return false;
    }
    // update stack
    for (auto it1 = stack.begin(); it1 != stack.end(); it1++) {
      int off = it1->first;
      auto it2 = rhs.stack.find(off);
      assert(it2 != rhs.stack.end()); // all stack offs in V_post_r are in output
      if (it1->second != it2->second) return false;
    }
    // compare ptr
    int pgm_input_type = mem_t::get_pgm_input_type();
    if (pgm_input_type == PGM_INPUT_pkt_ptrs) {
      if (input_simu_pkt_ptrs[0] != rhs.input_simu_pkt_ptrs[0]) return false;
      if (input_simu_pkt_ptrs[1] != rhs.input_simu_pkt_ptrs[1]) return false;
    }
    if (smt_var::is_win) {
      for (int i = 0; i < maps_mem.size(); i++) {
        for (int j = 0; j < maps_mem[i].size(); j++) {
          if (maps_mem[i][j] != rhs.maps_mem[i][j]) return false;
        }
      }
    }
  }
  return true;
}

ostream& operator<<(ostream& out, const inout_t& x) {
  out << "is_win:" << x.is_win << " ";
  out << hex << "simu_r10:" << x.input_simu_r10 << dec << " ";
  out << hex << "reg:" << x.reg << " " << dec;
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
  out << " " << "pkt_ptrs:"
      << hex << x.input_simu_pkt_ptrs[0] << "," << x.input_simu_pkt_ptrs[1] << dec;
  out << " " << "pgm_exit_type: " << x.pgm_exit_type;
  out << " " << "tail_call_para: " << x.tail_call_para;
  out << " " << "randoms_u32: ";
  for (int i = 0; i < x.randoms_u32.size(); i++) {
    out << hex << x.randoms_u32[i] << dec << " ";
  }
  for (auto it : x.regs) {
    out << "r" << it.first << ":" << hex << it.second << dec
        << ",type:" << x.reg_type[it.first] << "," << x.reg_readable[it.first] << " ";
  }
  out << "  stack: ";
  for (auto it : x.stack) {
    out << it.first << ":" << hex << (int)it.second << dec
        << "," << x.stack_readble[it.first] << " ";
  }
  if (smt_var::is_win) {
    out << " maps memory: ";
    for (int i = 0; i < x.maps_mem.size(); i++) {
      out << i << ":";
      for (int j = 0; j < x.maps_mem[i].size(); j++) {
        out << hex << setfill('0') << setw(2) << static_cast<int>(x.maps_mem[i][j]) << dec;
      }
      out << " ";
    }
  }
  return out;
}


void update_ps_by_input(prog_state& ps, const inout_t& input) {
  // cout << "input" << endl;
  // cout << input << endl;
  if (! input.is_win) ps.init_safety_chk();
  else ps.init_safety_chk(input.reg_readable, input.stack_readble, input.reg_type);
  ps._regs[10] = input.input_simu_r10;
  if (! input.is_win) {
    // cp input register
    ps._regs[1] = input.reg;
  }
  ps._input_reg_val = input.reg;
  // cp input map
  ps._mem.clear();
  if (input.is_win) {
    for (int i = 0; i < input.maps_mem.size(); i++) {
      int mem_off_s = mem_t::get_mem_off_by_idx_in_map(i, 0);
      assert(ps._mem._mem_size >= (mem_off_s + input.maps_mem[i].size()));
      for (int j = 0; j < input.maps_mem[i].size(); j++) {
        ps._mem._mem[mem_off_s + j] = input.maps_mem[i][j];
      }
    }
  }
  for (int i = 0; i < input.maps.size(); i++) {
    for (auto it = input.maps[i].begin(); it != input.maps[i].end(); it++) {
      ps._mem.update_kv_in_map(i, it->first, it->second);
    }
  }
  unsigned int pkt_sz = mem_t::_layout._pkt_sz;
  memcpy(ps._mem._pkt, input.pkt, sizeof(uint8_t)*pkt_sz);
  unsigned int skb_sz = mem_t::_layout._skb_max_sz;
  memcpy(ps._mem._skb, input.skb, sizeof(uint8_t)*skb_sz);

  ps._mem._simu_mem_s = input.input_simu_r10 - STACK_SIZE;
  int pgm_input_type = mem_t::get_pgm_input_type();
  if ((pgm_input_type == PGM_INPUT_pkt) || (pgm_input_type == PGM_INPUT_skb)) {
    ps._mem._simu_pkt_s = (uint64_t)input.reg;
    if (input.is_win) {
      ps._mem._simu_pkt_s = input.input_simu_pkt_s;
    }
  } else if (pgm_input_type == PGM_INPUT_pkt_ptrs) {
    ps._mem._simu_pkt_ptrs_s = (uint64_t)input.reg;
    if (input.is_win) {
      ps._mem._simu_pkt_ptrs_s = input.input_simu_pkt_ptrs_s;
    }
    ps._mem._pkt_ptrs[0] = input.input_simu_pkt_ptrs[0];
    ps._mem._pkt_ptrs[1] = input.input_simu_pkt_ptrs[1];
    ps._mem._simu_pkt_s = ps._mem._pkt_ptrs[0];
  }
  // set the exit type as the default type
  ps._pgm_exit_type = PGM_EXIT_TYPE_default;
  ps._tail_call_para = -1;
  // update u32 randoms values for BPF_FUNC_get_prandom_u32()
  for (int i = 0; i < ps._randoms_u32.size(); i++) {
    ps._randoms_u32[i] = input.randoms_u32[i];
  }
  ps._cur_randoms_u32_idx = 0;

  if (input.is_win) { // it is a window program, get stack and registers from input
    // update registers
    for (auto it : input.regs) {
      ps._regs[it.first] = it.second;
    }
    // update stack
    for (auto it : input.stack) {
      ps._mem.update_stack(it.first, it.second);
    }
  }
}

// update output according to program state for window_base program equivalence check method
void update_output_by_ps_win(inout_t& output, const prog_state& ps) {
  // update V_post_r in output
  const live_variables& post_r = smt_output::post_prog_r;
  // 1. update registers
  for (auto reg : post_r.regs) {
    output.regs[reg] = ps._regs[reg];
  }
  // 2. update stack
  auto it = post_r.mem.find(PTR_TO_STACK);
  if (it != post_r.mem.end()) {
    const unordered_set<int>& offs = it->second;
    for (auto off : offs) {
      output.stack[off] = ps._mem.get_stack_val_by_offset(off);
    }
  }
  // 3. update pkt
  int pgm_input_type = mem_t::get_pgm_input_type();
  int pkt_mem_type = PTR_TO_CTX;
  if (pgm_input_type == PGM_INPUT_pkt_ptrs) {
    pkt_mem_type = PTR_TO_PKT;
  }
  it = post_r.mem.find(pkt_mem_type);
  if (it != post_r.mem.end()) {
    const unordered_set<int>& offs = it->second;
    for (auto off : offs) {
      output.pkt[off] = ps._mem._pkt[off];
    }
  }

  // 4. update pkt_ptrs if input type is PGM_INPUT_pkt_ptrs
  if (pgm_input_type == PGM_INPUT_pkt_ptrs) {
    it = post_r.mem.find(PTR_TO_CTX);
    if (it != post_r.mem.end()) {
      const unordered_set<int>& offs = it->second;
      uint64_t addr1 = (uint64_t)&output.input_simu_pkt_ptrs;
      uint64_t addr2 = (uint64_t)&ps._mem._pkt_ptrs;
      for (auto off : offs) {
        uint64_t offset = off;
        *(uint8_t*)(addr1 + offset) = *(uint8_t*)(addr2 + offset);
      }
    }
  }
  // update map memory
  for (int i = 0; i < output.maps_mem.size(); i++) {
    int mem_off_s = mem_t::get_mem_off_by_idx_in_map(i, 0);
    assert(ps._mem._mem_size >= (mem_off_s + output.maps_mem[i].size()));
    for (int j = 0; j < output.maps_mem[i].size(); j++) {
      output.maps_mem[i][j] = ps._mem._mem[mem_off_s + j];
    }
  }
}

void update_output_by_ps(inout_t& output, const prog_state& ps) {
  output.clear();
  output.init();
  if (smt_var::is_win) {
    update_output_by_ps_win(output, ps);
    return;
  }
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
  memcpy(output.skb, ps._mem._skb, sizeof(uint8_t)*mem_t::_layout._skb_max_sz);

  output.pgm_exit_type = ps._pgm_exit_type;
  output.tail_call_para = ps._tail_call_para;
}

// parameter r10 is the simulated r10, stack_bottom is the real r10
uint64_t get_simu_addr_by_real(uint64_t real_addr, mem_t& mem, simu_real sr) {
  // real_addr may in mem or pkt
  // assume real_addr in mem first
  if ((real_addr >= (uint64_t)mem.get_mem_start_addr()) &&
      (real_addr <= (uint64_t)mem.get_mem_end_addr())) {
    return (real_addr + sr.simu_r10 - sr.real_r10);
  }
  int pgm_input_type = mem_t::get_pgm_input_type();
  if ((pgm_input_type == PGM_INPUT_pkt) || (pgm_input_type == PGM_INPUT_skb)) {
    // if real_addr not in mem, assume it is in pkt
    if ((real_addr >= (uint64_t)mem.get_pkt_start_addr()) &&
        (real_addr <= (uint64_t)mem.get_pkt_end_addr())) {
      return (real_addr + sr.simu_r1 - sr.real_r1);
    }
  } else if (pgm_input_type == PGM_INPUT_pkt_ptrs) {
    if ((real_addr >= (uint64_t)mem.get_pkt_ptrs_start_addr()) &&
        (real_addr <= (uint64_t)mem.get_pkt_ptrs_end_addr())) {
      return (real_addr + sr.simu_r1 - sr.real_r1);
    } else if ((real_addr >= (uint64_t)mem.get_pkt_start_addr()) &&
               (real_addr <= (uint64_t)mem.get_pkt_end_addr())) {
      return (real_addr + sr.simu_pkt - sr.real_pkt);
    }
  }

  if (pgm_input_type == PGM_INPUT_skb) { // check whether it is skb data
    if ((real_addr >= (uint64_t)mem.get_skb_start_addr()) &&
        (real_addr <= (uint64_t)mem.get_skb_end_addr())) {
      uint64_t simu_skb = (uint64_t)mem.get_simu_skb_start_addr();
      uint64_t real_skb = (uint64_t)mem.get_skb_start_addr();
      return (real_addr + simu_skb - real_skb);
    }
  }

  // else cannot convert real_addr to simu_addr, raise error
  string err_msg = "convert real_addr to simu_addr fail";
  throw (err_msg);
}

uint64_t get_real_addr_by_simu(uint64_t simu_addr, mem_t& mem, simu_real sr, int reg_type) {
  if (reg_type == PTR_TO_STACK) {
    uint64_t stack_start = mem.get_simu_mem_start_addr();
    if ((simu_addr >= stack_start) && (simu_addr < stack_start + STACK_SIZE)) {
      return (simu_addr + sr.real_r10 - sr.simu_r10);
    } else {
      string err_msg = "convert simu_addr to real_addr fail";
      throw (err_msg);
    }
  }
  if (smt_var::is_win) {
    if (reg_type == PTR_TO_MAP_VALUE) {
      bool flag = true;
      string err_msg = "";
      int n_maps = mem_t::maps_number();
      if (n_maps > 0) {
        uint64_t maps_s = mem.get_simu_mem_start_addr() + mem_t::get_mem_off_by_idx_in_map(0, 0);
        uint64_t last_map_s = mem.get_simu_mem_start_addr() + mem_t::get_mem_off_by_idx_in_map(n_maps - 1, 0);
        int last_map_sz = mem_t::map_max_entries(n_maps - 1) * mem_t::map_val_sz(n_maps - 1) / NUM_BYTE_BITS;
        uint64_t maps_e = last_map_s + last_map_sz - 1;
        if ((simu_addr >= maps_s) && (simu_addr <= maps_e)) {
          return (simu_addr + sr.real_r10 - sr.simu_r10);
        } else {
          flag = false;
          err_msg = "addr not in map region, convert simu_addr to real_addr fail";
        }
      } else {
        flag = false;
        err_msg = "there is no maps, convert simu_addr to real_addr fail";
      }

      if (! flag) {
        throw (err_msg);
      }
    }
  }
  if ((simu_addr >= mem.get_simu_mem_start_addr()) &&
      (simu_addr <= mem.get_simu_mem_end_addr())) {
    return (simu_addr + sr.real_r10 - sr.simu_r10);
  }
  int pgm_input_type = mem_t::get_pgm_input_type();
  if ((pgm_input_type == PGM_INPUT_pkt) || (pgm_input_type == PGM_INPUT_skb)) {
    if ((simu_addr >= mem.get_simu_pkt_start_addr()) &&
        (simu_addr <= mem.get_simu_pkt_end_addr())) {
      return (simu_addr + sr.real_r1 - sr.simu_r1);
    }
  } else if (pgm_input_type == PGM_INPUT_pkt_ptrs) {
    // assume addr is pkt_ptrs address
    if ((simu_addr >= mem.get_simu_pkt_ptrs_start_addr()) &&
        (simu_addr <= mem.get_simu_pkt_ptrs_end_addr())) {
      return (simu_addr + sr.real_r1 - sr.simu_r1);
    }
    if ((simu_addr >= mem.get_simu_pkt_start_addr()) &&
        (simu_addr <= mem.get_simu_pkt_end_addr())) {
      return (simu_addr + sr.real_pkt - sr.simu_pkt);
    }
  }

  if (pgm_input_type == PGM_INPUT_skb) {
    uint64_t simu_skb_s = (uint64_t)mem.get_simu_skb_start_addr();
    uint64_t simu_skb_e = (uint64_t)mem.get_simu_skb_end_addr();
    uint64_t real_skb = (uint64_t)mem.get_skb_start_addr();
    return (simu_addr + real_skb - simu_skb_s);
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

void get_cmp_lists_win(vector<int64_t>& val_list1, vector<int64_t>& val_list2,
                       inout_t& output1, inout_t& output2) {
  val_list1.clear();
  val_list2.clear();
  // update registers
  for (auto it1 = output1.regs.begin(); it1 != output1.regs.end(); it1++) {
    int reg = it1->first;
    auto it2 = output2.regs.find(reg);
    assert(it2 != output2.regs.end()); // all register in V_post_r are in output
    val_list1.push_back(it1->second);
    val_list2.push_back(it2->second);
  }
  // update stack
  for (auto it1 = output1.stack.begin(); it1 != output1.stack.end(); it1++) {
    int off = it1->first;
    auto it2 = output2.stack.find(off);
    assert(it2 != output2.stack.end()); // all stack offs in V_post_r are in output
    val_list1.push_back(it1->second);
    val_list2.push_back(it2->second);
  }
  // update pkt
  for (int i = 0; i < mem_t::_layout._pkt_sz; i++) val_list1.push_back(output1.pkt[i]);
  for (int i = 0; i < mem_t::_layout._pkt_sz; i++) val_list2.push_back(output2.pkt[i]);
  // update pkt_ptrs if PGM_INPUT_pkt_ptrs
  int pgm_input_type = mem_t::get_pgm_input_type();
  if (pgm_input_type == PGM_INPUT_pkt_ptrs) {
    for (int i = 0; i < 2; i++) { // 2 pointers
      val_list1.push_back(output1.input_simu_pkt_ptrs[i]);
      val_list2.push_back(output2.input_simu_pkt_ptrs[i]);
    }
  }
  // update map memory
  assert(output1.maps_mem.size() == output2.maps_mem.size());
  for (int i = 0; i < output1.maps_mem.size(); i++) {
    assert(output1.maps_mem[i].size() == output2.maps_mem[i].size());
    for (int j = 0; j < output1.maps_mem[i].size(); j++) {
      uint8_t v1 = output1.maps_mem[i][j];
      uint8_t v2 = output2.maps_mem[i][j];
      if (v1 != v2) {
        val_list1.push_back(v1);
        val_list2.push_back(v2);
      }
    }
  }
}

// val_list: [reg, #different keys, map0[k0], map0[k1], ...., map1[k0'], ...., pkt]
void get_cmp_lists(vector<int64_t>& val_list1, vector<int64_t>& val_list2,
                   inout_t& output1, inout_t& output2) {
  if (smt_var::is_win) {
    get_cmp_lists_win(val_list1, val_list2, output1, output2);
    return;
  }
  int diff_count1 = 0;
  int diff_count2 = 0;
  for (int i = 0; i < mem_t::maps_number(); i++) {
    unsigned int val_sz = mem_t::map_val_sz(i) / NUM_BYTE_BITS;
    diff_count1 += get_cmp_lists_one_map(val_list1, val_list2, val_sz,
                                         output1.maps[i], output2.maps[i], true);
    diff_count2 += get_cmp_lists_one_map(val_list2, val_list1, val_sz,
                                         output2.maps[i], output1.maps[i], false);
  }
  val_list1.push_back(diff_count1 + diff_count2);
  val_list2.push_back(0);
  // add pkt into lists
  for (int i = 0; i < mem_t::_layout._pkt_sz; i++) val_list1.push_back(output1.pkt[i]);
  for (int i = 0; i < mem_t::_layout._pkt_sz; i++) val_list2.push_back(output2.pkt[i]);

  int prog_exit_type_max_diff = mem_t::prog_array_map_max_entries();
  if (output1.pgm_exit_type != output2.pgm_exit_type) {
    val_list1.push_back(prog_exit_type_max_diff);
    val_list2.push_back(0);
    return;
  }
  // now two outputs have the same program exit type
  int output_type = output1.pgm_exit_type;
  if (output_type == PGM_EXIT_TYPE_default) {
    val_list1.push_back(output1.reg);
    val_list2.push_back(output2.reg);
  } else if (output_type == PGM_EXIT_TYPE_tail_call) {
    // MAP_TYPE_prog_array does not need to check registers
    // reference: github.com/torvalds/linux/blob/cb95712138ec5e480db5160b41172bbc6f6494cc/include/uapi/linux/bpf.h#L874
    // The same stack frame is used (but values on stack and in registers for the
    // caller are not accessible to the callee).
    val_list1.push_back(output1.tail_call_para);
    val_list2.push_back(output2.tail_call_para);
  } else {
    assert(false); // program has bug
  }
}
