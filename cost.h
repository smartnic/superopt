#pragma once

#include <vector>
#include "inst.h"
#include "inout.h"
#include "validator.h"
#include "prog.h"

using namespace std;

#define ERROR_COST_STRATEGY_ABS 0
#define ERROR_COST_STRATEGY_POP 1
#define ERROR_COST_STRATEGY_EQ1 0
#define ERROR_COST_STRATEGY_EQ2 1
#define ERROR_COST_STRATEGY_NAVG 0
#define ERROR_COST_STRATEGY_AVG 1

unsigned int pop_count_asm(unsigned int);

class cost {
 private:
  int _num_real_orig;
  int get_ex_error_cost(int output1, int output2);
  int get_avg_value(int ex_set_size);
  double get_final_error_cost(int exs_cost, int is_equal,
                              int ex_set_size, int num_successful_ex,
                              int avg_value);
 public:
  validator _vld;
  examples _examples;
  double _w_e = 0.5;
  double _w_p = 0.5;
  int _strategy_ex = 0;
  int _strategy_eq = 0;
  int _strategy_avg = 0;
  cost();
  ~cost();
  void init(prog* orig, int len, const vector<int> &input,
            double w_e = 0.5, double w_p = 0.5,
            int strategy_ex = 0, int strategy_eq = 0, int strategy_avg = 0);
  void set_orig(prog* orig, int len);
  double error_cost(prog* synth, int len);
  double perf_cost(prog* synth, int len);
  double total_prog_cost(prog* synth, int len);
};
