#include <iostream>
#include <random>
#include <algorithm>
#include <string>
#include <cmath>
#include <unordered_map>
#include "proposals.h"
#include "prog.h"
#include "cost.h"
#include "mh_prog.h"

using namespace std;

default_random_engine gen_mh;
uniform_real_distribution<double> unidist_mh(0.0, 1.0);

mh_sampler::mh_sampler() {}

mh_sampler::~mh_sampler() {}

double mh_sampler::cost_to_pi(double cost) {
  return pow(_base, -1.0 * cost);
}

prog* mh_sampler::next_proposal(prog* curr) {
  // TODO: use proposal generating functions to generate new programs
  // Use mod_random_inst_operand(.) and mod_random_inst(.) to do so
  // return mod_random_inst_operand(*curr);
  return mod_random_inst(*curr);
}

/* compute acceptance function */
double mh_sampler::alpha(prog* curr, prog* next) {
  double curr_cost = _cost.total_prog_cost(curr, MAX_PROG_LEN);
  double next_cost = _cost.total_prog_cost(next, MAX_PROG_LEN);
  cout << "Costs: curr " << curr_cost << " next " << next_cost << endl;
  return min(1.0, cost_to_pi(next_cost) / cost_to_pi(curr_cost));
}

prog* mh_sampler::mh_next(prog* curr) {
  prog* next = next_proposal(curr);
  double uni_sample = unidist_mh(gen_mh);
  if (uni_sample < alpha(curr, next)) {
    return next;
  } else {
    prog::clear_prog(next);
    return curr;
  }
}

void mh_sampler::mcmc_iter(int niter, const prog &orig,
                           unordered_map<int, vector<prog*> > &prog_freq) {
  // contruct the first program by copying the original
  prog *curr, *next;
  curr = prog::make_prog(orig);

  for (int i = 0; i < niter; i++) {
    cout << "Attempting mh iteration " << i << endl;
    next = mh_next(curr);
    /* Insert the next program into frequency dictionary if new */
    int ph = progHash()(*next);
    bool found = false;
    if (prog_freq.find(ph) == prog_freq.end()) {
      prog_freq[ph] = std::vector<prog*>();
    } else {
      vector<prog*> chain = prog_freq[ph];
      for (auto p : chain) {
        if (*p == *next) {
          found = true;
          p->freq_count++;
          break;
        }
      }
    }
    if (! found) {
      prog* next_copy = prog::make_prog(*next);
      next_copy->freq_count++;
      prog_freq[ph].push_back(next_copy);
    }
    if (curr != next) prog::clear_prog(curr);
    curr = next;
  }
}
