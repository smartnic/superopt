#pragma once

#include <vector>
#include "inst.h"
#include "inout.h"
#include "validator.h"
#include "prog.h"

using namespace std;

#define ERROR_COST_MAX 100000
#define ERROR_COST_NORMAL 1
#define PERF_COST_NORMAL 1

unsigned int pop_count_asm(unsigned int);

class cost {
 private:
  int _num_real_orig;
  int num_real_instructions(inst* program, int len);
 public:
  validator _vld;
  examples _examples;
  double _w_e = 0.5;
  double _w_p = 0.5;
  cost();
  ~cost();
  void init(prog* orig, int len, const vector<int> &input,
            double w_e = 0.5, double w_p = 0.5);
  void set_orig(prog* orig, int len);
  double error_cost(prog* synth, int len);
  double perf_cost(prog* synth, int len);
  double total_prog_cost(prog* synth, int len);
};
