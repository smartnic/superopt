#include "prog.h"

using namespace std;

// TODO: find canonical way to invoke one constructor from another
prog::prog(const prog& other) {
  freq_count = other.freq_count;
  _verfiy_res_flag = other._verfiy_res_flag;
  _error_cost_flag = other._error_cost_flag;
  _perf_cost_flag = other._perf_cost_flag;
  for (int i=0; i < MAX_PROG_LEN; i++) {
    inst_list[i] = other.inst_list[i];
  }
}

prog::prog(inst* instructions) {
  freq_count = 0;
  _verfiy_res_flag = false;
  _error_cost_flag = false;
  _perf_cost_flag = false;
  for (int i=0; i < MAX_PROG_LEN; i++) {
    inst_list[i] = instructions[i];
  }
}

// TODO: find canonical way to invoke such a constructor + destructor
prog* prog::make_prog(const prog &other) {
  prog* new_prog = (prog*)malloc(sizeof(prog));
  new_prog->freq_count = 0;
  new_prog->_verfiy_res_flag = false;
  new_prog->_error_cost_flag = false;
  new_prog->_perf_cost_flag = false;
  for (int i=0; i < MAX_PROG_LEN; i++) {
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
  for (int i=0; i < MAX_PROG_LEN; i++) {
    if (! (inst_list[i] == x.inst_list[i])) return false;
  }
  return true;
}

void prog::set_verify_res(int res) {
  _verfiy_res_flag = true;
  _verfiy_res = res;
}

void prog::set_error_cost(int cost) {
  _error_cost_flag = true;
  _error_cost = cost;
}

void prog::set_perf_cost(int cost) {
  _perf_cost_flag = true;
  _perf_cost = cost;
}

size_t progHash::operator()(const prog &x) const {
  size_t hval = 0;
  for (int i=0; i < MAX_PROG_LEN; i++) {
    hval = hval ^ (instHash()(x.inst_list[i]) << (i % 4));
  }
  return hval;
}
