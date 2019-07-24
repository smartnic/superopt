#include <iostream>
#include <random>
#include <algorithm>
#include <string>
#include <cmath>
#include "proposals.h"
#include "prog.h"
#include "cost.h"

using namespace std;

default_random_engine gen_mh;
uniform_real_distribution<double> unidist_mh(0.0, 1.0);

double cost_to_pi(double cost) {
  return pow(2, -1.0 * cost);
}

prog* next_proposal(prog* curr) {
  // TODO: use proposal generating functions to generate new programs
  // Use mod_random_inst_operand(.) and mod_random_inst(.) to do so
  return mod_random_inst_operand(*curr);
}

double total_prog_cost(prog* p, const prog &orig, inout* ex_set, int num_ex) {
  return error_cost(ex_set, num_ex,
                    orig.inst_list, orig.prog_length,
                    p->inst_list, p->prog_length);
}

/* compute acceptance function */
double alpha(prog* curr, prog* next, const prog &orig, inout* ex_set, int num_ex) {
  double curr_cost = total_prog_cost(curr, orig, ex_set, num_ex);
  double next_cost = total_prog_cost(next, orig, ex_set, num_ex);
  return min(1.0, cost_to_pi(next_cost) / cost_to_pi(curr_cost));
}

prog* mh_next(prog* curr, const prog &orig, inout* ex_set, int num_ex) {
  prog* next = next_proposal(curr);
  double uni_sample = unidist_mh(gen_mh);
  if (uni_sample < alpha(curr, next, orig, ex_set, num_ex)) {
    return next;
  } else {
    prog::clear_prog(next);
    return curr;
  }
}
