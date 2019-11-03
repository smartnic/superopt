#pragma once

#include <vector>
#include "inst.h"
#include "inout.h"
#include "validator.h"
#include "ex.h"

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
  void set_orig(inst* orig, int len);
  int error_cost(inst* synth, int len);
  int perf_cost(inst* synth, int len);
  double total_prog_cost(inst* synth, int len);
};
