#pragma once

#include <iostream>
#include <vector>
#include <unordered_map>
#include "../../src/utils.h"
#include "../../src/isa/inst.h"
#if ISA_TOY_ISA
#include "../../src/isa/toy-isa/inst.h"
#elif ISA_EBPF
#include "../../src/isa/ebpf/inst.h"
#endif

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
  static prog* make_prog(const prog &other);
  static void clear_prog(prog* p);
  void reset_vals();
  void set_error_cost(double cost);
  void set_perf_cost(double cost);
  int to_rel_bv(const prog &p) const;
  int to_rel_bv(const vector<prog> &ps) const;
  void to_abs_bv(vector<int>& bv) const;
  bool if_ret_exists(int start, int end) const;
  void update_map_if_implicit_ret_r0_needed(unordered_map<int, int> &map_before_after) const;
  void canonicalize();
  int num_real_instructions() const;
  int get_max_prog_len() const {return inst_list->get_max_prog_len();}
  int get_max_op_len() const {return inst_list->get_max_op_len();}
  int get_num_instr() const {return inst_list->get_num_instr();}
  reg_t interpret(prog_state &ps, reg_t input) const;
};

struct progHash {
  size_t operator()(const prog &x) const;
};
