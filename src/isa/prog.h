#pragma once

#include <iostream>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include "../../src/utils.h"
#include "../../src/isa/inst_header.h"

using namespace std;

class prog {
 public:
  inst* inst_list;
  int freq_count;
  double  _error_cost;
  double  _perf_cost;
  double _safety_cost;
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
  void set_safety_cost(double cost);
  int to_rel_bv(const prog &p) const;
  int to_rel_bv(const vector<prog> &ps) const;
  void to_abs_bv(vector<op_t>& bv) const;
  bool if_ret_exists(int start, int end) const;
  void update_map_if_implicit_ret_r0_needed(unordered_map<int, int> &map_before_after) const;
  void canonicalize();
  int num_real_instructions() const;
  double instructions_runtime() const;
  double instructions_runtime(int insn_s, int insn_e) const;
  void interpret(inout_t& output, prog_state &ps, const inout_t& input) const;
};

struct progHash {
  size_t operator()(const prog &x) const;
};

// top_k_progs: performance cost top k different programs with zero error cost
// make sure k >= 1
// assume k is a small number
class top_k_progs {
 private:
  double max_cost;
  int max_cost_id;
  unsigned int k;
  bool can_find(prog* p);
  void insert_without_check(prog* p);
 public:
  // `greater` makes progs in descending order of keys
  // key: perf cost, value.first: prog hash value, value.second: prog pointer
  vector<prog*> progs;
  top_k_progs(unsigned int k_val);
  ~top_k_progs();
  void insert(prog* p); // insert p if p is one of top k
  void clear();
  void sort();
};
