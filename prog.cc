#include <map>
#include "prog.h"

using namespace std;

// TODO: find canonical way to invoke one constructor from another
prog::prog(const prog& other) {
  freq_count = other.freq_count;
  _error_cost = other._error_cost;
  _perf_cost = other._perf_cost;
  for (int i = 0; i < MAX_PROG_LEN; i++) {
    inst_list[i] = other.inst_list[i];
  }
}

prog::prog(inst* instructions) {
  freq_count = 0;
  _error_cost = -1;
  _perf_cost = -1;
  for (int i = 0; i < MAX_PROG_LEN; i++) {
    inst_list[i] = instructions[i];
  }
}

// TODO: find canonical way to invoke such a constructor + destructor
prog* prog::make_prog(const prog &other) {
  prog* new_prog = (prog*)malloc(sizeof(prog));
  new_prog->freq_count = 0;
  new_prog->_error_cost = -1;
  new_prog->_perf_cost = -1;
  for (int i = 0; i < MAX_PROG_LEN; i++) {
    new_prog->inst_list[i] = other.inst_list[i];
  }
  return new_prog;
}

void prog::clear_prog(prog* p) {
  free(p);
}

prog::prog() {
  freq_count = 0;
}

prog::~prog() {
}

void prog::print() {
  print_program(inst_list, MAX_PROG_LEN);
}

void prog::print(const prog &p) {
  print_program(p.inst_list, MAX_PROG_LEN);
}

bool prog::operator==(const prog &x) const {
  for (int i = 0; i < MAX_PROG_LEN; i++) {
    if (! (inst_list[i] == x.inst_list[i])) return false;
  }
  return true;
}

void prog::set_error_cost(double cost) {
  _error_cost = cost;
}

void prog::set_perf_cost(double cost) {
  _perf_cost = cost;
}

rel_bv_prog prog::prog_rel_bit_vec(const prog &p) {
  rel_bv_prog bv;
  bv.reset(0);
  for (int i = 0; i < MAX_PROG_LEN; i++) {
    if (inst_list[i] == p.inst_list[i]) {
      bv.set(MAX_PROG_LEN - 1 - i);
    }
  }
  return bv;
}

rel_bv_prog prog::prog_rel_bit_vec(const vector<prog> &ps) {
  rel_bv_prog best;
  int count = 0;
  for (int i = 0; i < ps.size(); i++) {
    rel_bv_prog bv = prog_rel_bit_vec(ps[i]);
    int bv_count = bv.count();
    if (bv_count > count) {
      count = bv_count;
      best = bv;
    }
  }
  return best;
}

abs_bv_prog prog::prog_abs_bit_vec() const {
  string s = "";
  for (int i = 0; i < MAX_PROG_LEN; i++) {
    s += inst_list[i].inst_to_abs_bv().to_string();
  }
  abs_bv_prog bvp(s);
  return bvp;
}

void prog::canonicalize() {
  map<int, int> map_before_after; // key: reg_id before, val: reg_id after
  vector<int> reg_list(2);
  // store all used reg_ids as key in map_before_after
  for (int i = 0; i < MAX_PROG_LEN; i++) {
    reg_list = inst_list[i].get_reg_list();
    for (size_t j = 0; j < reg_list.size(); j++) {
      map_before_after[reg_list[j]] = -1;
    }
  }
  // if (max_reg_id_before + 1) == map_before_after.size(),
  // do not need to be canonicalized (already canonicalized)
  // reg_id starts from 0, so (max_reg_id_before + 1)
  int max_reg_id_before = -1;
  for (auto it = map_before_after.begin(); it != map_before_after.end(); it++) {
    if (it->first > max_reg_id_before)
      max_reg_id_before = it->first;
  }
  if ((max_reg_id_before + 1) == map_before_after.size()) return;
  // compute all reg_ids(after) and store into map_before_after
  int cur_reg_id = -1;
  for (auto it = map_before_after.begin(); it != map_before_after.end(); it++) {
    cur_reg_id++;
    it->second = cur_reg_id;
  }
  // replace reg_ids(before) with reg_ids(after) for all instructions
  for (int i = 0; i < MAX_PROG_LEN; i++) {
    for (int j = 0; j < inst_list[i].get_num_reg(); j++) {
      inst_list[i]._args[j] = map_before_after[inst_list[i]._args[j]];
    }
  }
}

size_t progHash::operator()(const prog &x) const {
  size_t hval = 0;
  for (int i = 0; i < MAX_PROG_LEN; i++) {
    hval = hval ^ (instHash()(x.inst_list[i]) << (i % 4));
  }
  return hval;
}
