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

void cost::init(prog* orig, int len, const vector<inout> &ex_set,
                double w_e, double w_p) {
  set_orig(orig, len);
  _examples.clear();
  for (size_t i = 0; i < ex_set.size(); i++) {
    _examples.insert(ex_set[i]);
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
  double total_cost = 0;
  inst* inst_list = (inst*)synth->inst_list;
  prog_state ps;
  int output1, output2;
  examples counterexs;
  for (int i = 0; i < _examples._exs.size(); i++) {
    output1 = _examples._exs[i].output;
    output2 = interpret(inst_list, len, ps, _examples._exs[i].input);
    cout << "Expected output: " << output1 << " Got output " << output2 << endl;
    // int ex_cost = pop_count_asm(output1 ^ output2);
    int ex_cost = abs(output1 - output2);
    if (!ex_cost) {
      int is_equal = _vld.is_equal_to(inst_list, len);
      if (is_equal == 0) { // not equal
        counterexs.insert(_vld._last_counterex);
        ex_cost = 1;
      } else if (is_equal == 1) { // equal
        ex_cost = 0;
      } else { // synth illegal
        total_cost = ERROR_COST_MAX;
        break;
      }
      cout << "is_equal: " << is_equal << endl;
    }
    total_cost += ex_cost;
  }
  for (size_t i = 0; i < counterexs._exs.size(); i++) {
    _examples.insert(counterexs._exs[i]);
  }
  if (counterexs._exs.size() > 0)
    cout << "new example set is:\n" << _examples._exs << endl;
  return (int)(total_cost);
}

int cost::perf_cost(prog* synth, int len) {
  return MAX_PROG_LEN - _num_real_orig +
         num_real_instructions((inst*)synth->inst_list, len);
}

double cost::total_prog_cost(prog* synth, int len) {
  double err_cost = error_cost(synth, len);
  cout << "Error cost: " << err_cost << endl;
  double per_cost = perf_cost(synth, len);
  cout << "Perf cost: " << per_cost << endl;
  return (_w_e * err_cost) + (_w_p * per_cost);
}
