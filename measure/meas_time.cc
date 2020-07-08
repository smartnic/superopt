#include <iostream>
#include <unordered_map>
#include <unordered_set>
#include <chrono>
#include "../src/utils.h"
#include "../src/inout.h"
#include "../src/isa/toy-isa/inst.h"
#include "../src/isa/prog.h"
#include "../src/verify/smt_prog.h"
#include "../src/verify/validator.h"
#include "../src/search/mh_prog.h"
#include "benchmark_toy_isa.h"
#include "z3++.h"

using namespace std;

#define measure_print(print, loop_times, t1, t2) \
cout << print << DUR(t1, t2) / loop_times << " us" << endl;

#define time_measure(func_called, times, print) \
int loop_times = times;                         \
auto start = NOW;                               \
for (int i = 0; i < loop_times; i++) {          \
  func_called;                                  \
}                                               \
auto end = NOW;                                 \
measure_print(print, times, start, end);

void time_smt_prog() {
  smt_prog ps;
  time_measure(ps.gen_smt(i, bm0, inst::max_prog_len), 1000,
               "smt prog::gen_smt: ");
}

void time_validator_set_orig() {
  validator vld;
  time_measure(vld.set_orig(bm0, inst::max_prog_len), 1000,
               "validator::set_orig: ");
}

void time_validator_is_equal_to() {
  validator vld;
  vld.set_orig(bm0, inst::max_prog_len);
  time_measure(vld.is_equal_to(bm0, inst::max_prog_len, bm0, inst::max_prog_len), 100,
               "validator::is_equal_to: ");
}

void time_validator_is_smt_valid() {
  validator vld;
  vld.is_equal_to(bm0, inst::max_prog_len, bm0, inst::max_prog_len);
  z3::expr smt = vld._store_f;
  z3::model mdl(smt_c);
  time_measure(vld.is_smt_valid(smt, mdl), 100,
               "validator::is_smt_valid: ");
}

void time_validator_get_orig_output() {
  validator vld;
  vld.set_orig(bm0, inst::max_prog_len);
  time_measure(vld.get_orig_output(i, NUM_REGS, bm0->get_input_reg()), 100,
               "validator::get_orig_output: ");
}

void time_interpret() {
  prog_state ps;
  ps.init();
  inout_t in, out;
  in.init();
  out.init();
  prog p(bm0);
  time_measure(p.interpret(out, ps, in), 10000,
               "interpret: ");
}

void time_cost_init() {
  double w_e = 1.0;
  double w_p = 0.0;
  vector<reg_t> input_regs = {10, 16, 11, 48, 1};
  vector<inout_t> inputs(5);
  for (int i = 0; i < inputs.size(); i++) {
    inputs[i].init();
    inputs[i].reg = input_regs[i];
  }
  cost c;
  prog orig(bm0);
  time_measure(c.init(&orig, N, inputs, w_e, w_p), 100,
               "cost::init: ");
}

void time_cost_error_cost() {
  double w_e = 1.0;
  double w_p = 0.0;
  vector<reg_t> input_regs = {10, 16, 11, 48, 1};
  vector<inout_t> inputs(5);
  for (int i = 0; i < inputs.size(); i++) {
    inputs[i].init();
    inputs[i].reg = input_regs[i];
  }
  cost c;
  prog orig(bm0);
  c.init(&orig, N, inputs, w_e, w_p);
  time_measure(c.error_cost(&orig, inst::max_prog_len, &orig, inst::max_prog_len);
               orig._error_cost = -1;
               orig._perf_cost = -1,
               200,
               "cost::error_cost: "
              );
}

void time_cost_perf_cost() {
  double w_e = 1.0;
  double w_p = 0.0;
  vector<reg_t> input_regs = {10, 16, 11, 48, 1};
  vector<inout_t> inputs(5);
  for (int i = 0; i < inputs.size(); i++) {
    inputs[i].init();
    inputs[i].reg = input_regs[i];
  }
  cost c;
  prog orig(bm0);
  c.init(&orig, N, inputs, w_e, w_p);
  time_measure(c.perf_cost(&orig, inst::max_prog_len), 1000,
               "cost::perf_cost: ");
}

void time_mh_sampler() {
  int loop_times = 50;
  auto start = NOW;
  for (int i = 0; i < loop_times; i++) {
    int nrolls = 1000;
    double w_e = 0.45;
    double w_p = 1.55;
    vector<inout_t> inputs(30);
    for (int i = 0; i < inputs.size(); i++) {
      inputs[i].init();
    }
    gen_random_input(inputs, 0, 50);
    mh_sampler mh;
    unordered_map<int, vector<prog*> > prog_freq;
    prog orig(bm0);
    mh._cost.init(&orig, N, inputs, w_e, w_p);
    mh.mcmc_iter(nrolls, orig, prog_freq);
  }
  auto end = NOW;
  measure_print("mh_sampler: ", loop_times, start, end);
}

int main() {
  time_smt_prog();
  time_validator_set_orig();
  time_validator_is_equal_to();
  time_validator_is_smt_valid();
  time_validator_get_orig_output();
  time_interpret();
  time_cost_init();
  time_cost_error_cost();
  time_cost_perf_cost();
  time_mh_sampler();
  return 0;
}
