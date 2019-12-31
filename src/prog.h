#pragma once

#include <iostream>
#include <bitset>
#include <vector>
#include <unordered_map>
#include "../src/isa/toy-isa/inst.h"

using namespace std;

typedef bitset<toy_isa::MAX_PROG_LEN> rel_bv_prog;
typedef bitset<toy_isa::MAX_PROG_LEN * INST_ABS_BIT_LEN> abs_bv_prog;

class prog {
 public:
  inst* inst_list;
  int freq_count;
  double  _error_cost;
  double  _perf_cost;
  prog(const prog& other);
  prog(inst* instructions);
  prog();
  void print();
  static void print(const prog &p);
  ~prog();
  bool operator==(const prog &x) const;
  static prog* make_prog(const prog &x);
  static void clear_prog(prog* p);
  void set_error_cost(double cost);
  void set_perf_cost(double cost);
  rel_bv_prog prog_rel_bit_vec(const prog &p);
  rel_bv_prog prog_rel_bit_vec(const vector<prog> &ps);
  abs_bv_prog prog_abs_bit_vec() const;
  bool if_ret_exists(int start, int end) const;
  void update_map_if_implicit_ret_r0_needed(unordered_map<int, int> &map_before_after) const;
  void canonicalize();
  int get_max_prog_len() const {return inst_list[0].get_max_prog_len();}
  int get_max_op_len() const {return inst_list[0].get_max_op_len();}
};

struct progHash {
  size_t operator()(const prog &x) const;
};
