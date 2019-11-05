#pragma once

#include <iostream>
#include "inst.h"

using namespace std;

class prog {
 public:
  inst inst_list[MAX_PROG_LEN];
  int freq_count;
  bool _verfiy_res_flag;
  int  _verfiy_res;
  bool _error_cost_flag;
  int  _error_cost;
  bool _perf_cost_flag;
  int  _perf_cost;
  prog(const prog& other);
  prog(inst* instructions);
  prog();
  void print();
  static void print(const prog &p);
  ~prog();
  bool operator==(const prog &x) const;
  static prog* make_prog(const prog &x);
  static void clear_prog(prog* p);
  void set_verify_res(int res);
  void set_error_cost(int cost);
  void set_perf_cost(int cost);
};

struct progHash {
  size_t operator()(const prog &x) const;
};

