#include <iostream>
#include <random>
#include <algorithm>
#include <string>
#include <climits>
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
    toy_isa_prog_state ps;
    int output = orig->inst_list->interpret(len, ps, input[i]);
    inout example;
    example.set_in_out(input[i], output);
    _examples.insert(example);
  }
  _w_e = w_e;
  _w_p = w_p;
  _strategy_ex = strategy_ex;
  _strategy_eq = strategy_eq;
  _strategy_avg = strategy_avg;
  _meas_new_counterex_gened = false;
}

void cost::set_orig(prog* orig, int len) {
  try {
    _vld.set_orig(orig->inst_list, len);
  } catch (const string err_msg) {
    cout << "ERROR: the original program is illegal. ";
    cerr << err_msg << endl;
    throw (err_msg);
    return;
  }
  _num_real_orig = orig->num_real_instructions();
}

int cost::get_ex_error_cost(int output1, int output2) {
  switch (_strategy_ex) {
    case ERROR_COST_STRATEGY_ABS: return abs(output1 - output2);
    case ERROR_COST_STRATEGY_POP: return pop_count_asm(output1 ^ output2);
    default:
      cout << "ERROR: no error cost example strategy matches." << endl;
      return ERROR_COST_MAX;
  }
}

int cost::get_avg_value(int ex_set_size) {
  switch (_strategy_avg) {
    case ERROR_COST_STRATEGY_AVG: return ex_set_size;
    case ERROR_COST_STRATEGY_NAVG: return 1;
    default:
      cout << "ERROR: no error cost average strategy matches." << endl;
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
      cout << "ERROR: no error cost equation strategy matches." << endl;
      return ERROR_COST_MAX;
  }
}

/*
 * Steps for error cost computation:
 * 1. Compute c_ex, the error cost from EACH example
 *   Two strategy for c_ex:
 *     a. ERROR_COST_STRATEGY_ABS: c_ex = abs(output_orig - output_synth)
 *     b. ERROR_COST_STRATEGY_POP: c_ex = pop_count(output_orig XOR output_synth)
 * 2. Get average value `avg_v` according to the choice from following two strategies
 *     a.ERROR_COST_STRATEGY_NAVG: avg_v = 1, which means no averaging process
 *     b.ERROR_COST_STRATEGY_AVG: avg_v = #examples
 * 3. Compute total error cost:
 *     For valid synthesis:
 *       a. ERROR_COST_STRATEGY_EQ1:
 *         error_cost = [unequal*(#succ_ex) + sum(c_ex list)]/avg_v
 *       b. ERROR_COST_STRATEGY_EQ2:
 *         error_cost = unequal + [unequal*(#fail_ex) + sum(c_ex list)]/avg_v
 *       where unequal = 1 if synthesis is unequal to the original.
 *     For invalid synthesis:
 *       error_cost = ERROR_COST_MAX
 */
double cost::error_cost(prog* synth, int len) {
  if (synth->_error_cost != -1) return synth->_error_cost;
  double total_cost = 0;
  toy_isa_prog_state ps;
  int output1, output2;
  int num_successful_ex = 0;
  // process total_cost with example set
  for (int i = 0; i < _examples._exs.size(); i++) {
    output1 = _examples._exs[i].output;
    output2 = synth->inst_list->interpret(len, ps, _examples._exs[i].input);
    int ex_cost = get_ex_error_cost(output1, output2);
    if (ex_cost == 0) num_successful_ex++;
    total_cost += ex_cost;
  }
  int is_equal = 0;
  int ex_set_size = _examples._exs.size();
  if (num_successful_ex == ex_set_size) {
    is_equal = _vld.is_equal_to(synth->inst_list, len);
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
    _meas_new_counterex_gened = true;
  }
  return total_cost;
}

double cost::perf_cost(prog* synth, int len) {
  if (synth->_perf_cost != -1) return synth->_perf_cost;
  int total_cost =  synth->get_max_prog_len() - _num_real_orig +
                    synth->num_real_instructions();
  synth->set_perf_cost(total_cost);
  return total_cost;
}

double cost::total_prog_cost(prog* synth, int len) {
  double err_cost = error_cost(synth, len);
  double per_cost = perf_cost(synth, len);
  return (_w_e * err_cost) + (_w_p * per_cost);
}
