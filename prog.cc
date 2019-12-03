#include <unordered_map>
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
  unordered_map<int, int> map_before_after; // key: reg_id before, val: reg_id after
  vector<int> reg_list(2);
  // traverse all instructions and once there is a new reg_id(before), assign it a reg_id(after)
  // store reg_id(before) and reg_id(after) into map
  for (int i = 0; i < MAX_PROG_LEN; i++) {
    reg_list = inst_list[i].get_reg_list();
    for (size_t j = 0; j < reg_list.size(); j++) {
      if (map_before_after.find(reg_list[j]) == map_before_after.end()) {
        map_before_after[reg_list[j]] = (int)map_before_after.size();
      }
    }
  }
  if (map_before_after.size() == 0) return;
  // replace reg_ids(before) with reg_ids(after) for all instructions
  for (int i = 0; i < MAX_PROG_LEN; i++) {
    for (int j = 0; j < inst_list[i].get_num_reg(); j++) {
      int reg_id_after = map_before_after[inst_list[i]._args[j]];
      if (inst_list[i]._args[j] != reg_id_after)
        inst_list[i]._args[j] = reg_id_after;
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
