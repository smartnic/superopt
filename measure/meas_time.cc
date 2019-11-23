#include <iostream>
#include <unordered_map>
#include <unordered_set>
#include <random>
#include <algorithm>
#include <chrono>
#include "../prog.h"
#include "../inout.h"
#include "../mh_prog.h"
#include "../utils.h"
#include "common.h"
#include "z3++.h"

using namespace std;

#define NOW chrono::steady_clock::now()
#define DUR chrono::duration <double, micro> (end - start).count()

default_random_engine time_msr_gen;
uniform_real_distribution<double> time_msr_unidist(0.0, 1.0);

void time_smt_prog() {
  int loop_times = 1000;
  smt_prog ps;
  auto start = NOW;
  for (int i = 0; i < loop_times; i++) {
    ps.gen_smt(i, bm0, N);
  }
  auto end = NOW;
  cout << "smt prog::gen_smt: " << DUR / loop_times << " us" << endl;
}

void time_validator_set_orig() {
  int loop_times = 1000;
  validator vld;
  auto start = NOW;
  for (int i = 0; i < loop_times; i++) {
    vld.set_orig(bm0, N);
  }
  auto end = NOW;
  cout << "validator::set_orig: " << DUR / loop_times << " us" << endl;
}

void time_validator_is_equal_to() {
  int loop_times = 100;
  validator vld;
  vld.set_orig(bm0, N);
  auto start = NOW;
  for (int i = 0; i < loop_times; i++) {
    vld.is_equal_to(bm0, N);
  }
  auto end = NOW;
  cout << "validator::is_equal_to: " << DUR / loop_times << " us" << endl;
}

void time_validator_is_smt_valid() {
  int loop_times = 100;
  validator vld;
  vld.is_equal_to(bm0, N);
  z3::expr smt = vld._store_f;
  auto start = NOW;
  for (int i = 0; i < loop_times; i++) {
    vld.is_smt_valid(smt);
  }
  auto end = NOW;
  cout << "validator::is_smt_valid: " << DUR / loop_times << " us" << endl;
}

void time_validator_get_orig_output() {
  int loop_times = 100;
  auto start = NOW;
  validator vld;
  vld.set_orig(bm0, N);
  for (int i = 0; i < loop_times; i++) {
    vld.get_orig_output(i);
  }
  auto end = NOW;
  cout << "validator::get_orig_output: " << DUR / loop_times << " us" << endl;
}

void time_interpret() {
  int loop_times = 10000;
  prog_state ps;
  auto start = NOW;
  for (int i = 0; i < loop_times; i++) {
    interpret(bm0, N, ps, i);
  }
  auto end = NOW;
  cout << "interpret: " << DUR / loop_times << " us" << endl;
}

void time_examples_insert() {
  int loop_times = 10000;
  int num_ex = 1000;
  int max_input = 1000;
  vector<int> inputs(num_ex);
  vector<inout> inouts(num_ex);
  for (int i = 0; i < num_ex; i++) {
    inputs[i] = time_msr_unidist(time_msr_gen) * max_input;
  }
  for (int i = 0; i < num_ex; i++) {
    prog_state ps;
    int output = interpret(bm0, N, ps, inputs[i]);
    inouts[i].set_in_out(inputs[i], output);
  }
  examples ex_set;
  auto start = NOW;
  for (int i = 0; i < loop_times; i++) {
    ex_set.insert(inouts[i]);
  }
  auto end = NOW;
  cout << "examples::insert: " << DUR / loop_times << " us" << endl;
}

void time_cost_init() {
  int loop_times = 30;
  double w_e = 1.0;
  double w_p = 0.0;
  vector<int> input = {10, 16, 11, 48, 1};
  cost c;
  prog orig(bm0);
  auto start = NOW;
  for (int i = 0; i < loop_times; i++) {
    c.init(&orig, N, input, w_e, w_p);
  }
  auto end = NOW;
  cout << "cost::init: " << DUR / loop_times << " us" << endl;
}

void time_cost_error_cost() {
  int loop_times = 200;
  double w_e = 1.0;
  double w_p = 0.0;
  vector<int> input = {10, 16, 11, 48, 1};
  cost c;
  prog orig(bm0);
  c.init(&orig, N, input, w_e, w_p);
  auto start = NOW;
  for (int i = 0; i < loop_times; i++) {
    c.error_cost(&orig, N);
    orig._error_cost = -1;
    orig._perf_cost = -1;
  }
  auto end = NOW;
  cout << "cost::error_cost: " << DUR / loop_times << " us" << endl;
}

void time_cost_perf_cost() {
  int loop_times = 1000;
  double w_e = 1.0;
  double w_p = 0.0;
  vector<int> input = {10, 16, 11, 48, 1};
  cost c;
  prog orig(bm0);
  c.init(&orig, N, input, w_e, w_p);
  auto start = NOW;
  for (int i = 0; i < loop_times; i++) {
    c.perf_cost(&orig, N);
  }
  auto end = NOW;
  cout << "cost::perf_cost: " << DUR / loop_times << " us" << endl;
}

void time_mh_sampler() {
  int loop_times = 2;
  auto start = NOW;
  for (int i = 0; i < loop_times; i++) {
    int nrolls = 1000;
    double w_e = 0.45;
    double w_p = 1.55;
    vector<int> inputs(30);
    gen_random_input(inputs, 0, 50);
    mh_sampler mh;
    unordered_map<int, vector<prog*> > prog_freq;
    prog orig(bm0);
    mh._cost.init(&orig, N, inputs, w_e, w_p);
    mh.mcmc_iter(nrolls, orig, prog_freq);
  }
  auto end = NOW;
  cout << "cost::mh_sampler: " << DUR / loop_times << " us" << endl;
}

int main() {
  time_smt_prog();
  time_validator_set_orig();
  time_validator_is_equal_to();
  time_validator_is_smt_valid();
  time_validator_get_orig_output();
  time_interpret();
  time_examples_insert();
  time_cost_init();
  time_cost_error_cost();
  time_cost_perf_cost();
  time_mh_sampler();
  return 0;
}   
