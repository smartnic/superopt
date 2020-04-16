#include "inst_var.h"

using namespace std;

ostream& operator<<(ostream& out, const mem_layout& layout) {
  out << "stack<start_off>: " << layout._stack_start << endl
      << "map<start_off, key_sz, val_sz, max_entries>:" << endl;
  for (int i = 0; i < layout._maps_attr.size(); i++) {
    out << layout._maps_start[i] << " " << layout._maps_attr[i] << endl;
  }
  return out;
}

unsigned int map_t::get_next_idx() {
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

mem_t::mem_t() {
}

mem_t::~mem_t() {
  delete []_mem;
  _mem = NULL;
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

uint8_t* mem_t::get_stack_start_addr() {
  return &_mem[_layout._stack_start];
}

uint8_t* mem_t::get_stack_bottom_addr() {
  return &_mem[STACK_SIZE - 1] + 1;
}

uint8_t* mem_t::get_mem_start_addr() {
  return &_mem[0];
}

uint8_t* mem_t::get_mem_end_addr() {
  return &_mem[_mem_size - 1];
}

mem_t& mem_t::operator=(const mem_t &rhs) {
  memcpy(_mem, rhs._mem, sizeof(uint8_t)*_mem_size);
  for (int i = 0; i < rhs._maps.size(); i++) _maps[i] = rhs._maps[i];
  return *this;
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

void mem_t::clear() {
  memset(_mem, 0, sizeof(uint8_t)*_mem_size);
  for (int i = 0; i < _maps.size(); i++) _maps[i].clear();
}
