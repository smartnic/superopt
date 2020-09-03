#pragma once

#include <iostream>
#include <vector>
#include <unordered_map>
#include "../../src/utils.h"
#include "../../src/isa/inst_header.h"

using namespace std;

class prog {
 public:
  inst* inst_list;
  int freq_count;
  double  _error_cost;
  double  _perf_cost;
  prog(const prog& other);
  prog(inst* instructions);
  prog();
  void print() const;
  ~prog();
  bool operator==(const prog &x) const;
  void reset_vals();
  void set_vals(const prog &x);
  void set_error_cost(double cost);
  void set_perf_cost(double cost);
  int to_rel_bv(const prog &p) const;
  int to_rel_bv(const vector<prog> &ps) const;
  void to_abs_bv(vector<op_t>& bv) const;
  bool if_ret_exists(int start, int end) const;
  void update_map_if_implicit_ret_r0_needed(unordered_map<int, int> &map_before_after) const;
  void canonicalize();
  int num_real_instructions() const;
  double instructions_runtime() const;
  void interpret(inout_t& output, prog_state &ps, const inout_t& input) const;
};

struct progHash {
  size_t operator()(const prog &x) const;
};
