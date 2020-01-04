#pragma once

#include <iostream>
#include <bitset>
#include <vector>
#include <unordered_map>
#include "../../../src/isa/inst.h"
#include "../../../src/isa/toy-isa/inst.h"

using namespace std;

typedef bitset<toy_isa::MAX_PROG_LEN> rel_bv_prog;
typedef bitset<toy_isa::MAX_PROG_LEN * INST_ABS_BIT_LEN> abs_bv_prog;

class prog {
 public:
  toy_isa_inst* inst_list = nullptr;
  int freq_count;
  double  _error_cost;
  double  _perf_cost;
  prog(const prog& other);
  prog(inst* instructions);
  prog();
  void print() const;
  ~prog();
  bool operator==(const prog &x) const;
  void init_vals();
  void set_error_cost(double cost);
  void set_perf_cost(double cost);
  rel_bv_prog prog_rel_bit_vec(const prog &p);
  rel_bv_prog prog_rel_bit_vec(const vector<prog> &ps);
  abs_bv_prog prog_abs_bit_vec() const;
  bool if_ret_exists(int start, int end) const;
  void update_map_if_implicit_ret_r0_needed(unordered_map<int, int> &map_before_after) const;
  void canonicalize();
  int num_real_instructions() const;
  int get_max_prog_len() const {return inst_list->get_max_prog_len();}
  int get_max_op_len() const {return inst_list->get_max_op_len();}
  int get_num_instr() const {return inst_list->get_num_instr();}
};

struct progHash {
  size_t operator()(const prog &x) const;
};
