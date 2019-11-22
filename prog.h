#pragma once

#include <iostream>
#include <bitset>
#include <vector>
#include "inst.h"

using namespace std;

typedef bitset<MAX_PROG_LEN> bv_prog;
typedef bitset<MAX_PROG_LEN * INST_ABS_BIT_LEN> abs_bv_prog;

class prog {
 public:
  inst inst_list[MAX_PROG_LEN];
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
  bv_prog prog_rel_bit_vec(const prog &p);
  bv_prog prog_rel_bit_vec(const vector<prog> &ps);
  abs_bv_prog prog_abs_bit_vec() const;
};

struct progHash {
  size_t operator()(const prog &x) const;
};
