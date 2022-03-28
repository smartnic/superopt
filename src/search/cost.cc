#include <iostream>
#include <random>
#include <algorithm>
#include <string>
#include <climits>
#include "cost.h"

int dur_sum;
int dur_sum_long;
int n_sum_long;

cost::cost() {}

cost::~cost() {}

void cost::init(prog* orig, int len, const vector<inout_t> &input,
                double w_e, double w_p, double w_s,
                int strategy_ex, int strategy_eq, int strategy_avg, int strategy_perf,
                bool enable_prog_eq_cache, bool enable_prog_uneq_cache, bool is_win, int functionality) {
  _vld._is_win = is_win;  // enable win eq chk
  smt_var::is_win = is_win;
  _num_real_orig = orig->num_real_instructions();
  if (! is_win) {
    set_orig(orig, len);
    set_examples(input, orig);
  }
  _w_e = w_e;
  _w_p = w_p;
  _w_s = w_s;
  _functionality = functionality;
  _strategy_ex = strategy_ex;
  _strategy_eq = strategy_eq;
  _strategy_avg = strategy_avg;
  _strategy_perf = strategy_perf;
  _meas_new_counterex_gened = false;
  _vld._enable_prog_eq_cache = enable_prog_eq_cache;
  _vld._enable_prog_uneq_cache = enable_prog_uneq_cache;
}

void cost::set_examples(const vector<inout_t> &input, prog* orig) {
  if (logger.is_print_level(LOGGER_DEBUG)) {
    cout << "set_examples" << endl;
  }
  _examples.clear();
  prog_state ps;
  ps.init();
  // reserve 100 elements to ensure _examples._exs won't need to be reallocated
  // this solves the issue of elements' values being changed without writing them.
  _examples._exs.reserve(100);
  _examples._exs.resize(input.size());
  try {
    for (size_t i = 0; i < input.size(); i++) {
      if (logger.is_print_level(LOGGER_DEBUG)) {
        cout << i << ": " << input[i] << endl;
      }
      ps.clear();
      inout_t output;
      output.init();
      // Assume original program can pass the interpreter
      orig->interpret(output, ps, input[i]);
      _examples._exs[i].set_in_out(input[i], output);
    }
  } catch (const string err_msg) {
    cout << "ERROR: set_examples: ";
    cerr << err_msg << endl;
    throw (err_msg);
  }
}

void cost::set_orig(prog* orig, int len, int win_start, int win_end) {
  try {
    _vld.set_orig(orig->inst_list, len, win_start, win_end);
  } catch (const string err_msg) {
    cout << "ERROR: the original program is illegal. ";
    cerr << err_msg << endl;
    throw (err_msg);
    return;
  }
  _num_real_orig = orig->num_real_instructions();
  if (smt_var::is_win) {
    set_perf_cost_base_win(orig, len, win_start, win_end);
  }
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
    res += abs(double(val_list1[i] - val_list2[i]));
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
      else if (is_equal < 0) return ERROR_COST_MAX;
      else return exs_cost;
    case ERROR_COST_STRATEGY_EQ2:
      if (is_equal == 0) return  (1 + (double)(exs_cost + ex_set_size
                                    - num_successful_ex) / avg_value);
      else if (is_equal < 0) return ERROR_COST_MAX;
      else return exs_cost;
    default:
      cout << "ERROR: no error cost equation strategy matches." << endl;
      return ERROR_COST_MAX;
  }
}

// 1. check window is in one block.
// 2. If it is an end block and win_end is the last block insn,
//    original win should be an end block and win_end is also the last block insn
bool is_win_legal(inst* orig, int len1, inst* synth, int len2,
                  int win_start, int win_end) {
  graph g;
  g.gen_graph(synth, len2);
  // cout << win_start << " " << win_end << endl;
  int win_block = -1;
  for (int i = 0; i <= g.nodes.size(); i++) {
    if ((g.nodes[i]._start <= win_start) && (win_start <= g.nodes[i]._end)) {
      win_block = i;
      break;
    }
  }

  if (win_end > g.nodes[win_block]._end) { // not in the same basic block
    return false;
  }

  // check win_end is the end insn of the block or whether synth win is an end block
  if ((win_end < g.nodes[win_block]._end) || (g.nodes_out[win_block].size() != 0)) {
    return true;
  }

  graph g_orig;
  g_orig.gen_graph(orig, len1);
  bool is_orig_win_end_block = false;
  for (int i = 0; i <= g_orig.nodes.size(); i++) {
    if ((win_end == g_orig.nodes[i]._end) && (g_orig.nodes_out[i].size() == 0)) {
      is_orig_win_end_block = true;
      break;
    }
  }

  bool synth_win_can_be_end = is_orig_win_end_block;
  return synth_win_can_be_end;
}
/*
*This safety_cost_repair only measures the safety of the synth program
*
*/
double cost::safety_cost_repair(prog* orig, int len1, prog* synth, int len2) {
  if (synth->_error_cost != -1) return synth->_error_cost;
  //boolean variable: true: pass static safety, fals: fails
  int total_safety_cost = 0;
  bool pass_static_safety;
  try {
    if (! smt_var::is_win) {
      static_safety_check_pgm(synth->inst_list, len2);
    } else {
      static_safety_check_win(synth->inst_list,
                              inout_t::start_insn, inout_t::end_insn,
                              _vld._pss_orig);
    }
    pass_static_safety = true;
  } catch (const string err_msg) {
    
    //instead of error_cost_max, set it as 1 or something
    //modify to safety_cost_max
    pass_static_safety = false;
    //total_safety_cost += 1;
    //synth->set_safety_cost(ERROR_COST_MAX);
    //return ERROR_COST_MAX;
  }

  //double total_cost = 0;
  //inout_t output1
  inout_t output2;
  //output1.init();
  output2.init();
  //int num_successful_ex = 0;
  prog_state ps;
  ps.init();

  // variable to keep track of number of tests for which output is not observed
  int num_of_unsucc_ex = 0;
  // process total_cost with example set
  for (int i = 0; i < _examples._exs.size(); i++) {
    //Not req
    //output1 = _examples._exs[i].output;
    
    try {
      synth->interpret(output2, ps, _examples._exs[i].input);
    } catch (const string err_msg) {
      // illegal program
      //synth->set_safety_cost(ERROR_COST_MAX);
      //return ERROR_COST_MAX;
      num_of_unsucc_ex ++;
    }

    //double ex_cost = get_ex_error_cost(output1, output2);
    //if (ex_cost == 0) num_successful_ex++;
    // else if (ex_cost >= ERROR_COST_MAX) {
    //   // synthesis whose test case error cost >= ERROR_COST_MAX
    //   synth->set_error_cost(ERROR_COST_MAX);
    //   return ERROR_COST_MAX;
    // }

    //total_cost += ex_cost;
  }

  int is_equal = 0;
  //int ex_set_size = _examples._exs.size();
  //boolean variable to check whether it passes validator or not
  bool pass_validator = false;
  //condition: passed previous 2 modules (true, num_of_succ_ex == ex_size)
  if (pass_static_safety && (num_of_unsucc_ex == 0)) {
    
    auto t1 = NOW;
    try {
      is_equal = _vld.is_equal_to(orig->inst_list, len1, synth->inst_list, len2);
      //Check this: Return: 0(unequal), 1(equal), -1(unsafe) 
      // if (smt_var::is_win) {
      //   // check win prog eq check result
      //   validator vld1;
      //   vld1._is_win = false;
      //   smt_var::is_win = false;
      //   vld1.set_orig(orig->inst_list, len1);
      //   int is_equal_expected = vld1.is_equal_to(orig->inst_list, len1, synth->inst_list, len2);
      //   cout << "win prog check: " << is_equal << " " << is_equal_expected << endl;
      //   if (is_equal_expected != is_equal) {
      //     cout << "win prog check fail: " << is_equal << " " << is_equal_expected << endl;
      //   }
      //   smt_var::is_win = true;
      // }
      pass_validator = true;
    } catch (const string err_msg) {
      // illegal program
      //synth->set_error_cost(ERROR_COST_MAX);
      //synth->set_safety_cost(ERROR_COST_MAX);
      //return ERROR_COST_MAX;
      pass_validator = false;
    }
    auto t2 = NOW;
    auto dur = DUR(t1, t2);
    // cout << dur << endl;
    dur_sum += dur;
    if (dur > 50000) {
      dur_sum_long += dur;
      n_sum_long++;
      // synth->print();
    }
  }
  

  //int avg_value = get_avg_value(ex_set_size);
  //total_cost = get_final_error_cost(total_cost, is_equal,
                                    //ex_set_size, num_successful_ex,
                                    //avg_value);
  // process counterexamples
  // If num_successful_ex < (int)_examples._exs.size(),
  // it shows the example that synth fails in the example set is a counterexample.
  // The counterexample generated from this synth may have already been in the examples set.
  // Thus, only when num_successful_ex == (int)_examples._exs.size(),
  // the counterexample generated from this synth must can be added into the example set.
  // But it should ensure that the number of initial example set is big enough.
  // case 1: gen_counterex_flag = (is_equal == 0);
  // case 2: gen_counterex_flag = (is_equal == 0) && (num_successful_ex == (int)_examples._exs.size());
  
  //CONDITION: ONLY IF ITS UNSAFE BUT PASSES FIRST 2 MODULES
  //VALIDATOR GENERATES COUNTEREXAMPLE 
  //if (((is_equal == 0) || (is_equal == ILLEGAL_CEX)) &&
  //    (num_successful_ex == (int)_examples._exs.size())) {
  if(pass_static_safety && (num_of_unsucc_ex == 0) && ((pass_validator == false)||is_equal==-1)){
    _examples.insert(_vld._last_counterex);
    _meas_new_counterex_gened = true;
    cout << "counterexample " << _examples.size() << ":" << endl;
    cout << _vld._last_counterex.input << endl;
    cout << _vld._last_counterex.output << endl;
  }
  // in case there is overflow which makes a positive value become a negative value or
  // total_cost > ERROR_COST_MAX
  // if ((total_cost > ERROR_COST_MAX) || (total_cost < 0)) {
  //   synth->set_error_cost(ERROR_COST_MAX);
  //   return ERROR_COST_MAX;
  // }

  //calculate safety cost based on the 3 variables
  //think of some good equation
  //set_safety_cost below:
  //synth->set_error_cost(total_cost);
  //return total_cost;
  //Some function of: pass_satic_safety, num_of_unsucc_ex, pass_validator, isEqual

  //The below is a very rough calculation as a placeholder for now
  //Later the safety cost will be made more continuous and accurate

  if(pass_static_safety && (num_of_unsucc_ex == 0)){
    if(pass_validator == false){
        total_safety_cost += 2;
    }else if(is_equal < 0){
        total_safety_cost += 1;
    }
  }else{
    if(!pass_static_safety){
      total_safety_cost += 3;
    }
    total_safety_cost += num_of_unsucc_ex;
  }

  synth->set_safety_cost(total_safety_cost);
  return total_safety_cost;
}

/*
*This error_cost_repair only measures the error or distance from original program, not safety
*
*/
double cost::error_cost_repair(prog* orig, int len1, prog* synth, int len2) {
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
    //2 VARIABLES
    //1: NUM OF TEST CASES THAT CAN GET OUTPUT
    //2: OUTPUTS TO COMPARE TO ORIGINAL PROGRAMS OUTPUT
    //TRY TO GENERATE PRIVATE/TEMPORARY MEMORY TO SHARE RESULTS BETWEEN ERROR AND SAFETY COST
    //CALCULATE RESULT AND PASS TO THE TWO FUNCTIONS TO SAVE TIME.
    try {
      synth->interpret(output2, ps, _examples._exs[i].input);
    } catch (const string err_msg) {
      // illegal program
      //synth->set_error_cost(ERROR_COST_MAX);
      //synth->set_safety_cost(ERROR_COST_MAX);
      //return ERROR_COST_MAX;
      //This cost has already been added for safety before.
      //if this happens, we just move on to the next test case
      continue;
    }
    double ex_cost = get_ex_error_cost(output1, output2);
    if (ex_cost == 0) num_successful_ex++;
    // else if (ex_cost >= ERROR_COST_MAX) {
    //   // synthesis whose test case error cost >= ERROR_COST_MAX
    //   synth->set_error_cost(ERROR_COST_MAX);
    //   return ERROR_COST_MAX;
    // }

    total_cost += ex_cost;
  }
  int is_equal = 0;
  int ex_set_size = _examples._exs.size();
  if (num_successful_ex == ex_set_size) {
    auto t1 = NOW;
    try {
      is_equal = _vld.is_equal_to(orig->inst_list, len1, synth->inst_list, len2);
      // if (smt_var::is_win) {
      //   // check win prog eq check result
      //   validator vld1;
      //   vld1._is_win = false;
      //   smt_var::is_win = false;
      //   vld1.set_orig(orig->inst_list, len1);
      //   int is_equal_expected = vld1.is_equal_to(orig->inst_list, len1, synth->inst_list, len2);
      //   cout << "win prog check: " << is_equal << " " << is_equal_expected << endl;
      //   if (is_equal_expected != is_equal) {
      //     cout << "win prog check fail: " << is_equal << " " << is_equal_expected << endl;
      //   }
      //   smt_var::is_win = true;
      // }
    } catch (const string err_msg) {
      // illegal program
      //synth->set_error_cost(ERROR_COST_MAX);
      //synth->set_safety_cost(ERROR_COST_MAX);
      //return ERROR_COST_MAX;
      //already took care of this situation for safety
    }
    auto t2 = NOW;
    auto dur = DUR(t1, t2);
    // cout << dur << endl;
    dur_sum += dur;
    if (dur > 50000) {
      dur_sum_long += dur;
      n_sum_long++;
      // synth->print();
    }
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
  //if (((is_equal == 0) || (is_equal == ILLEGAL_CEX)) &&
   //   (num_successful_ex == (int)_examples._exs.size())) {
  //we already take care of is_equal < 0 for safety
  //New condition:  
  if((num_successful_ex == (int)_examples._exs.size()) && (is_equal == 0)){
    _examples.insert(_vld._last_counterex);
    _meas_new_counterex_gened = true;
    cout << "counterexample " << _examples.size() << ":" << endl;
    cout << _vld._last_counterex.input << endl;
    cout << _vld._last_counterex.output << endl;
  }
  // in case there is overflow which makes a positive value become a negative value or
  // total_cost > ERROR_COST_MAX
  // if ((total_cost > ERROR_COST_MAX) || (total_cost < 0)) {
  //   synth->set_error_cost(ERROR_COST_MAX);
  //   return ERROR_COST_MAX;
  // }

  synth->set_error_cost(total_cost);
  return total_cost;
}

/*
 * This error cost has both safety and error costs incorporated.
 * Steps for error cost computation:
 *
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
  try {
    if (! smt_var::is_win) {
      static_safety_check_pgm(synth->inst_list, len2);
    } else {
      static_safety_check_win(synth->inst_list,
                              inout_t::start_insn, inout_t::end_insn,
                              _vld._pss_orig);
    }
  } catch (const string err_msg) {
    synth->set_error_cost(ERROR_COST_MAX);
    return ERROR_COST_MAX;
  }

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
    // else if (ex_cost >= ERROR_COST_MAX) {
    //   // synthesis whose test case error cost >= ERROR_COST_MAX
    //   synth->set_error_cost(ERROR_COST_MAX);
    //   return ERROR_COST_MAX;
    // }

    total_cost += ex_cost;
  }
  int is_equal = 0;
  int ex_set_size = _examples._exs.size();
  if (num_successful_ex == ex_set_size) {
    auto t1 = NOW;
    try {
      is_equal = _vld.is_equal_to(orig->inst_list, len1, synth->inst_list, len2);
      // if (smt_var::is_win) {
      //   // check win prog eq check result
      //   validator vld1;
      //   vld1._is_win = false;
      //   smt_var::is_win = false;
      //   vld1.set_orig(orig->inst_list, len1);
      //   int is_equal_expected = vld1.is_equal_to(orig->inst_list, len1, synth->inst_list, len2);
      //   cout << "win prog check: " << is_equal << " " << is_equal_expected << endl;
      //   if (is_equal_expected != is_equal) {
      //     cout << "win prog check fail: " << is_equal << " " << is_equal_expected << endl;
      //   }
      //   smt_var::is_win = true;
      // }
    } catch (const string err_msg) {
      // illegal program
      synth->set_error_cost(ERROR_COST_MAX);
      return ERROR_COST_MAX;
    }
    auto t2 = NOW;
    auto dur = DUR(t1, t2);
    // cout << dur << endl;
    dur_sum += dur;
    if (dur > 50000) {
      dur_sum_long += dur;
      n_sum_long++;
      // synth->print();
    }
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
  if (((is_equal == 0) || (is_equal == ILLEGAL_CEX)) &&
      (num_successful_ex == (int)_examples._exs.size())) {
    _examples.insert(_vld._last_counterex);
    _meas_new_counterex_gened = true;
    cout << "counterexample " << _examples.size() << ":" << endl;
    cout << _vld._last_counterex.input << endl;
    cout << _vld._last_counterex.output << endl;
  }
  // in case there is overflow which makes a positive value become a negative value or
  // total_cost > ERROR_COST_MAX
  // if ((total_cost > ERROR_COST_MAX) || (total_cost < 0)) {
  //   synth->set_error_cost(ERROR_COST_MAX);
  //   return ERROR_COST_MAX;
  // }

  synth->set_error_cost(total_cost);
  return total_cost;
}

void cost::set_perf_cost_base_win(prog* orig, int len, int win_start, int win_end) {
  double cost = 0;
  if (_strategy_perf == PERF_COST_STRATEGY_LEN) {
    double orig_cost = orig->num_real_instructions();
    double win_cost = num_real_instructions(&orig->inst_list[win_start], win_end - win_start + 1);
    cost = orig_cost - win_cost;
  } else if (_strategy_perf == PERF_COST_STRATEGY_RUNTIME) {
    double orig_cost = orig->instructions_runtime();
    double win_cost = orig->instructions_runtime(win_start, win_end);
    cost = orig_cost - win_cost;
  }
  _perf_cost_base_win = cost;
  cout << "[cost] perf_cost_base_win: " << _perf_cost_base_win << endl;
}

double cost::perf_cost(prog* synth, int len, bool set_win) {
  if (synth->_perf_cost != -1) return synth->_perf_cost;
  double total_cost;
  if (_strategy_perf == PERF_COST_STRATEGY_LEN) {
    if (smt_var::is_win && set_win) {
      total_cost = _perf_cost_base_win;
      total_cost += num_real_instructions(&synth->inst_list[inout_t::start_insn],
                                          inout_t::end_insn - inout_t::start_insn + 1);
    } else {
      total_cost = synth->num_real_instructions();
    }
  } else if (_strategy_perf == PERF_COST_STRATEGY_RUNTIME) {
    if (smt_var::is_win && set_win) {
      total_cost = _perf_cost_base_win;
      total_cost += synth->instructions_runtime(inout_t::start_insn, inout_t::end_insn);
    } else {
      total_cost = synth->instructions_runtime();
    }
  } else {
    string err_msg = "ERROR: no performance cost strategy matches.";
    throw (err_msg);
  }
  synth->set_perf_cost(total_cost);
  return total_cost;
}

//called by alpha
double cost::total_prog_cost(prog * orig, int len1, prog * synth, int len2) {
  bool flag = (synth->_error_cost == -1);
  double err_cost = 0.0;
  double per_cost = 0.0;
  double safe_cost = 0.0;

  if (_functionality == 1){
    //for repair
    err_cost = error_cost_repair(orig, len1, synth, len2);
    safe_cost = safety_cost_repair(orig, len1, synth, len2);

    return (_w_e * err_cost) + (_w_s * safe_cost);

  }else{
    //for optimize
    err_cost = error_cost(orig, len1, synth, len2);
    per_cost = perf_cost(synth, len2, true);
    if (flag && logger.is_print_level(LOGGER_DEBUG)) {
      cout << "cost: " << err_cost << " " << per_cost << " "
           << (_w_e * err_cost) + (_w_p * per_cost) << endl;
    }

    return (_w_e * err_cost) + (_w_p * per_cost);

  }
  
}

