#include <iostream>
#include <random>
#include <algorithm>
#include <string>
#include <climits>
#include "cost.h"

#define ERROR_COST_MAX 100000

cost::cost() {}

cost::~cost() {}

void cost::init(prog* orig, int len, const vector<inout_t> &input,
                double w_e, double w_p,
                int strategy_ex, int strategy_eq, int strategy_avg) {
  set_orig(orig, len);
  _examples.clear();
  prog_state ps;
  ps.init();
  for (size_t i = 0; i < input.size(); i++) {
    ps.clear();
    inout_t output;
    output.init();
    // Assume original program can pass the interpreter
    orig->interpret(output, ps, input[i]);
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

unsigned int pop_count_outputs(reg_t output1, reg_t output2) {
  int gap = 32;
  unsigned int count = 0;
  int n = 1 + (NUM_REG_BITS - 1) / gap;
  for (int i = 0; i < n; i++) {
    count += pop_count_asm((uint32_t)output1 ^ (uint32_t)output2);
    output1 >>= gap;
    output2 >>= gap;
  }
  return count;
}

double cost::get_ex_error_cost_from_val_lists_abs(vector<reg_t>& val_list1, vector<reg_t>& val_list2) {
  double res = 0;
  assert(val_list1.size() == val_list2.size());
  for (int i = 0; i < val_list1.size(); i++) {
    res += abs((double)val_list1[i] - (double)val_list2[i]);
  }
  return res;
}

double cost::get_ex_error_cost_from_val_lists_pop(vector<reg_t>& val_list1, vector<reg_t>& val_list2) {
  double res = 0;
  assert(val_list1.size() == val_list2.size());
  for (int i = 0; i < val_list1.size(); i++) {
    res += pop_count_outputs(val_list1[i], val_list2[i]);
  }
  return res;
}

double cost::get_ex_error_cost(inout_t& output1, inout_t& output2) {
  vector<reg_t> val_list1, val_list2;
  get_cmp_lists(val_list1, val_list2, output1, output2);
  double res = 0;
  switch (_strategy_ex) {
    // `double`: in case there is overflow which makes a positive value
    // become a negative value
    case ERROR_COST_STRATEGY_ABS: return get_ex_error_cost_from_val_lists_abs(val_list1, val_list2);
    case ERROR_COST_STRATEGY_POP: return get_ex_error_cost_from_val_lists_pop(val_list1, val_list2);
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

double cost::get_final_error_cost(double exs_cost, int is_equal,
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
double cost::error_cost(prog* orig, int len1, prog* synth, int len2) {
  if (synth->_error_cost != -1) return synth->_error_cost;
  double total_cost = 0;
  inout_t output1, output2;
  output1.init();
  output2.init();
  int num_successful_ex = 0;
  prog_state ps;
  ps.init();
  // process total_cost with example set
  for (int i = 0; i < _examples._exs.size(); i++) {
    output1 = _examples._exs[i].output;
    try {
      synth->interpret(output2, ps, _examples._exs[i].input);
    } catch (const string err_msg) {
      // illegal program
      synth->set_error_cost(ERROR_COST_MAX);
      return ERROR_COST_MAX;
    }
    double ex_cost = get_ex_error_cost(output1, output2);
    if (ex_cost == 0) num_successful_ex++;
    else if (ex_cost >= ERROR_COST_MAX) {
      // synthesis whose test case error cost >= ERROR_COST_MAX
      synth->set_error_cost(ERROR_COST_MAX);
      return ERROR_COST_MAX;
    }

    total_cost += ex_cost;
  }
  int is_equal = 0;
  int ex_set_size = _examples._exs.size();

  if (num_successful_ex == ex_set_size) {
    is_equal = _vld.is_equal_to(orig->inst_list, len1, synth->inst_list, len2);
  }

  int avg_value = get_avg_value(ex_set_size);
  total_cost = get_final_error_cost(total_cost, is_equal,
                                    ex_set_size, num_successful_ex,
                                    avg_value);
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
  // in case there is overflow which makes a positive value become a negative value or
  // total_cost > ERROR_COST_MAX
  if ((total_cost > ERROR_COST_MAX) || (total_cost < 0)) {
    synth->set_error_cost(ERROR_COST_MAX);
    return ERROR_COST_MAX;
  }

  synth->set_error_cost(total_cost);
  return total_cost;
}

double cost::perf_cost(prog* synth, int len) {
  if (synth->_perf_cost != -1) return synth->_perf_cost;
  int total_cost =  MAX_PROG_LEN - _num_real_orig +
                    synth->num_real_instructions();
  synth->set_perf_cost(total_cost);
  return total_cost;
}

double cost::total_prog_cost(prog* orig, int len1, prog* synth, int len2) {
  double err_cost = error_cost(orig, len1, synth, len2);
  double per_cost = perf_cost(synth, len2);
  return (_w_e * err_cost) + (_w_p * per_cost);
}
