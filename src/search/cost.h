#pragma once

#include <vector>
#include "../../src/utils.h"
#include "../../src/inout.h"
#include "../../src/isa/inst_header.h"
#include "../../src/isa/prog.h"
#include "../../src/verify/validator.h"

using namespace std;
extern int dur_sum;
extern int dur_sum_long;
extern int n_sum_long;

#define ERROR_COST_STRATEGY_ABS 0
#define ERROR_COST_STRATEGY_POP 1
#define ERROR_COST_STRATEGY_EQ1 0
#define ERROR_COST_STRATEGY_EQ2 1
#define ERROR_COST_STRATEGY_NAVG 0
#define ERROR_COST_STRATEGY_AVG 1

#define PERF_COST_STRATEGY_LEN 0  // length of programs
#define PERF_COST_STRATEGY_RUNTIME 1 // runtime of programs

class cost {
 private:
  int _num_real_orig;
  double get_ex_error_cost(inout_t& output1, inout_t& output2);
  int get_avg_value(int ex_set_size);
  double get_final_error_cost(double exs_cost, int is_equal,
                              int ex_set_size, int num_successful_ex,
                              int avg_value);
  double get_ex_error_cost_from_val_lists_abs(vector<reg_t>& val_list1, vector<reg_t>& val_list2);
  double get_ex_error_cost_from_val_lists_pop(vector<reg_t>& val_list1, vector<reg_t>& val_list2);
 public:
  validator _vld;
  examples _examples;
  bool _meas_new_counterex_gened;
  double _w_e = 0.5;
  double _w_p = 0.5;
  int _strategy_ex = 0;
  int _strategy_eq = 0;
  int _strategy_avg = 0;
  int _strategy_perf = 0;
  cost();
  ~cost();
  void init(prog* orig, int len, const vector<inout_t> &input,
            double w_e = 0.5, double w_p = 0.5,
            int strategy_ex = 0, int strategy_eq = 0,
            int strategy_avg = 0, int strategy_perf = 0,
            bool enable_prog_eq_cache = true,
            bool enable_prog_uneq_cache = false);
  void set_orig(prog* orig, int len);
  double error_cost(prog* orig, int len1, prog* synth, int len2);
  double perf_cost(prog* synth, int len);
  double total_prog_cost(prog* orig, int len1, prog* synth, int len2);
};

unsigned int pop_count_outputs(int64_t output1, int64_t output2);
