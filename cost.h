#pragma once

#include <vector>
#include "inst.h"
#include "inout.h"
#include "validator.h"
#include "prog.h"

using namespace std;

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
  void init(prog* orig, int len, const vector<inout> &ex_set,
            double w_e = 0.5, double w_p = 0.5);
  void set_orig(prog* orig, int len);
  int error_cost(prog* synth, int len);
  int perf_cost(prog* synth, int len);
  double total_prog_cost(prog* synth, int len);
};
