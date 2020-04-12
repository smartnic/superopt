#pragma once

#include <vector>
#include <unordered_map>
#include <queue>
#include <string.h>
#include "../../../src/utils.h"
#include "../../../src/verify/smt_var.h"

using namespace std;

#define STACK_SIZE 512 // 512 bytes

class mem_layout {
 public:
  unsigned int _stack_start = 0;
  vector<map_attr> _maps_attr;
  vector<unsigned int> _maps_start;
  friend ostream& operator<<(ostream& out, const mem_layout& layout);
};

class map_t {
 public:
  // map: key to array index i in map range, i starts from 0, the max of i is (_max_entries - 1)
  unordered_map<string, unsigned int> _k2idx;
  // next available idx for each map, starts from empty queue,
  // map delete may push an item in the queue
  queue<unsigned int> _next_idx_q;
  // maximum number of entries in map from the beginning to now
  unsigned int _cur_max_entries = 0;
  unsigned int _max_entries;
  map_t(unsigned int max_entries) {_max_entries = max_entries;}
  unsigned int get_next_idx();
  void add_next_idx(unsigned int idx);
  void clear();
};

class mem_t {
 public:
  int _mem_size; // size unit: byte
  // should ensure memory is contiguous, because of the assumption in memory_access_check
  uint8_t *_mem;
  // stack address is the bottom of the stack
  uint64_t _stack_addr; // = (uint64_t)&_mem[STACK_SIZE - 1] + 1;
  vector<map_t> _maps;
  static mem_layout _layout;
  mem_t();
  ~mem_t();
  static void add_map(map_attr m_attr);
  static void set_map_attr(int map_id, map_attr m_attr);
  unsigned int get_mem_off_by_idx_in_map(int map_id, unsigned int idx_in_map);
  uint8_t* get_stack_start_s();
  void clear();
};
