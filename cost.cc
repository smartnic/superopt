#include <iostream>
#include <random>
#include <algorithm>
#include <string>
#include <climits>
#include "inst.h"
#include "inout.h"
#include "cost.h"

#define ERROR_COST_MAX 100000

/* Requires support for advanced bit manipulation (ABM) instructions on the
 * architecture where this program is run. */
unsigned int pop_count_asm(unsigned int x) {
  unsigned int y = x;
  unsigned int z;
  asm ("popcnt %1, %0"
       : "=a" (z)
       : "b" (y)
      );
  return z;
}

cost::cost() {}

cost::~cost() {}

void cost::init(prog* orig, int len, const vector<int> &input,
                double w_e, double w_p) {
  set_orig(orig, len);
  _examples.clear();
  for (size_t i = 0; i < input.size(); i++) {
    inout example;
    example.set_in_out(input[i], _vld.get_orig_output(input[i]));
    _examples.insert(example);
  }
  _w_e = w_e;
  _w_p = w_p;
}

int cost::num_real_instructions(inst* program, int len) {
  int count = 0;
  for (int i = 0; i < len; i++) {
    if (program[i]._opcode != NOP) count++;
  }
  return count;
}

void cost::set_orig(prog* orig, int len) {
  try {
    _vld.set_orig((inst*)orig->inst_list, len);
  } catch (const string err_msg) {
    throw (err_msg);
    return;
  }
  _num_real_orig = num_real_instructions((inst*)orig->inst_list, len);
}

int cost::error_cost(prog* synth, int len) {
  if (synth->_error_cost_flag) return synth->_error_cost;
  double total_cost = 0;
  inst* inst_list = (inst*)synth->inst_list;
  prog_state ps;
  int output1, output2;
  int verify_times = 0;
  // process total_cost with example set
  for (int i = 0; i < _examples._exs.size(); i++) {
    output1 = _examples._exs[i].output;
    output2 = interpret(inst_list, len, ps, _examples._exs[i].input);
    cout << "Expected output: " << output1 << " Got output " << output2 << endl;
    // int ex_cost = pop_count_asm(output1 ^ output2);
    int ex_cost = abs(output1 - output2);
    if (! ex_cost) verify_times++;
    total_cost += ex_cost;
  }
  // if verifying is needed, process total_cost with verify result
  int is_equal = 0;
  if (verify_times > 0) {
    if (synth->_verfiy_res_flag) {
      is_equal = synth->_verfiy_res;
    } else {
      is_equal = _vld.is_equal_to(inst_list, len);
      synth->set_verify_res(is_equal);
    }
    // equal: total_cost += 0
    // not equal: total_cost += verify_times * 1
    // synth illegal: total_cost = ERROR_COST_MAX
    if (is_equal == 0) { // not equal
      total_cost += verify_times;
    } else if (is_equal == -1) { // synth illegal
      total_cost = ERROR_COST_MAX;
    }
  }
  synth->set_error_cost((int)total_cost);

  bool gen_counterex_flag = (is_equal == 0);
  // If verify_times < (int)_examples._exs.size(),
  // it shows the example that synth fails in the example set is a counterexample.
  // The counterexample generated from this synth may have already been in the examples set.
  // Thus, only when verify_times == (int)_examples._exs.size(),
  // the counterexample generated from this synth must can be added into the example set.
  // But it should ensure that the number of initial example set is big enough. 
  // bool gen_counterex_flag = (is_equal == 0) && (verify_times == (int)_examples._exs.size());

  // process counterexamples
  if (gen_counterex_flag) {
    _examples.insert(_vld._last_counterex);
  }
  if (gen_counterex_flag)
    cout << "new example set is:\n" << _examples._exs << endl;
  return (int)(total_cost);
}

int cost::perf_cost(prog* synth, int len) {
  if (synth->_perf_cost_flag) return synth->_perf_cost;
  int total_cost =  MAX_PROG_LEN - _num_real_orig +
                    num_real_instructions((inst*)synth->inst_list, len);
  synth->set_perf_cost(total_cost);
  return total_cost;
}

double cost::total_prog_cost(prog* synth, int len) {
  double err_cost = error_cost(synth, len);
  cout << "Error cost: " << err_cost << endl;
  double per_cost = perf_cost(synth, len);
  cout << "Perf cost: " << per_cost << endl;
  return (_w_e * err_cost) + (_w_p * per_cost);
}
