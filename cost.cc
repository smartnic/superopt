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
                double w_e, double w_p,
                int strategy_ex, int strategy_eq, int strategy_avg) {
  set_orig(orig, len);
  _examples.clear();
  for (size_t i = 0; i < input.size(); i++) {
    prog_state ps;
    int output = interpret((inst*)orig->inst_list, len, ps, input[i]);
    inout example;
    example.set_in_out(input[i], output);
    _examples.insert(example);
  }
  _w_e = w_e;
  _w_p = w_p;
  _strategy_ex = strategy_ex;
  _strategy_eq = strategy_eq;
  _strategy_avg = strategy_avg;
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
    cout << "ERROR: the original program is illegal. ";
    cerr << err_msg << endl;
    throw (err_msg);
    return;
  }
  _num_real_orig = num_real_instructions((inst*)orig->inst_list, len);
}

int cost::get_ex_error_cost(int output1, int output2) {
  switch (_strategy_ex) {
    case ERROR_COST_STRATEGY_ABS: return abs(output1 - output2);
    case ERROR_COST_STRATEGY_POP: return pop_count_asm(output1 ^ output2);
    default:
      cout << "ERROR: no error cost example strategy matches.";
      return ERROR_COST_MAX;
  }
}

int cost::get_avg_value(int ex_set_size) {
  switch (_strategy_avg) {
    case ERROR_COST_STRATEGY_AVG: return ex_set_size;
    case ERROR_COST_STRATEGY_NAVG: return 1;
    default:
      cout << "ERROR: no error cost average strategy matches.";
      return 1;
  }
}

double cost::get_final_error_cost(int exs_cost, int is_equal,
                                  int ex_set_size, int num_successful_ex,
                                  int avg_value) {
  switch (_strategy_eq) {
    case ERROR_COST_STRATEGY_EQ1:
      if (is_equal == 0) return ((double)(exs_cost + num_successful_ex) / avg_value);
      else if (is_equal == -1) return ERROR_COST_MAX;
      else return exs_cost;
    case ERROR_COST_STRATEGY_EQ2:
      if (is_equal == 0) return  (1 + (double)(exs_cost + ex_set_size
                                    - num_successful_ex) / avg_value);
      else if (is_equal == -1) return ERROR_COST_MAX;
      else return exs_cost;
    default:
      cout << "ERROR: no error cost equation strategy matches.";
      return ERROR_COST_MAX;
  }
}

double cost::error_cost(prog* synth, int len) {
  if (synth->_error_cost != -1) return synth->_error_cost;
  double total_cost = 0;
  inst* inst_list = (inst*)synth->inst_list;
  prog_state ps;
  int output1, output2;
  int num_successful_ex = 0;
  // process total_cost with example set
  for (int i = 0; i < _examples._exs.size(); i++) {
    output1 = _examples._exs[i].output;
    output2 = interpret(inst_list, len, ps, _examples._exs[i].input);
    int ex_cost = get_ex_error_cost(output1, output2);
    if (! ex_cost) num_successful_ex++;
    total_cost += ex_cost;
  }
  int is_equal = 0;
  int ex_set_size = _examples._exs.size();
  if (num_successful_ex == ex_set_size) {
    is_equal = _vld.is_equal_to(inst_list, len);
  }
  
  int avg_value = get_avg_value(ex_set_size);
  total_cost = get_final_error_cost(total_cost, is_equal,
                                    ex_set_size, num_successful_ex,
                                    avg_value);
  synth->set_error_cost(total_cost);
  // process counterexamples
  // If num_successful_ex < (int)_examples._exs.size(),
  // it shows the example that synth fails in the example set is a counterexample.
  // The counterexample generated from this synth may have already been in the examples set.
  // Thus, only when num_successful_ex == (int)_examples._exs.size(),
  // the counterexample generated from this synth must can be added into the example set.
  // But it should ensure that the number of initial example set is big enough.
  // case 1: gen_counterex_flag = (is_equal == 0);
  // case 2: gen_counterex_flag = (is_equal == 0) && (num_successful_ex == (int)_examples._exs.size());
  if ((is_equal == 0) && (num_successful_ex == (int)_examples._exs.size())) {
    _examples.insert(_vld._last_counterex);
  }
  return total_cost;
}

double cost::perf_cost(prog* synth, int len) {
  if (synth->_perf_cost != -1) return synth->_perf_cost;
  int total_cost =  MAX_PROG_LEN - _num_real_orig +
                    num_real_instructions((inst*)synth->inst_list, len);
  synth->set_perf_cost(total_cost);
  return total_cost;
}

double cost::total_prog_cost(prog* synth, int len) {
  double err_cost = error_cost(synth, len);
  double per_cost = perf_cost(synth, len);
  return (_w_e * err_cost) + (_w_p * per_cost);
}
