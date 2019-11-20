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

mh_sampler::mh_sampler() {
  _meas_data._mode = false;
}

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
  return min(1.0, cost_to_pi(next_cost) / cost_to_pi(curr_cost));
}

prog* mh_sampler::mh_next(prog* curr) {
  prog* next = next_proposal(curr);
  double uni_sample = unidist_mh(gen_mh);
  double a = alpha(curr, next);
  _meas_data.insert_proposal(*next, uni_sample < a);
  int iter_num = _meas_data._proposals.size() - 1;
  if (iter_num == 0) {
    _meas_data.insert_examples(iter_num, _cost._examples);
  } else if (_cost._meas_new_counterex_gened) {
    _meas_data.insert_examples(iter_num, _cost._vld._last_counterex);
    _cost._meas_new_counterex_gened = false;
  };
  if (uni_sample < a) {
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
  curr = prog::make_prog(start);

  for (int i = 0; i < niter; i++) {
    next = mh_next(curr);
    /* Insert the next program into frequency dictionary if new */
    int ph = progHash()(*next);
    prog* meas_p;
    bool found = false;
    if (prog_freq.find(ph) == prog_freq.end()) {
      prog_freq[ph] = std::vector<prog*>();
    } else {
      vector<prog*> chain = prog_freq[ph];
      for (auto p : chain) {
        if (*p == *next) {
          found = true;
          p->freq_count++;
          meas_p = p;
          break;
        }
      }
    }
    if (! found) {
      // TODO: may define new prog copy API
      prog* next_copy = prog::make_prog(*next);
      next_copy->freq_count++;
      next_copy->_error_cost = next->_error_cost;
      next_copy->_perf_cost = next->_perf_cost;
      prog_freq[ph].push_back(next_copy);
      meas_p = next_copy;
    }
    if (i == 0) {
      _meas_data.insert_program(i, *curr);
    }
    if (curr != next) {
      prog::clear_prog(curr);
      _meas_data.insert_program(i, *next);
    }
    curr = next;
  }
  prog::clear_prog(curr);
}

void mh_sampler::turn_on_measure() {
  _meas_data._mode = true;
}

void mh_sampler::turn_off_measure() {
  _meas_data._mode = false;
}
/* class mh_sampler_restart end */

/* class mh_sampler_restart start */
mh_sampler_restart::mh_sampler_restart(int max_iteration) {
  _max_iteration = max_iteration;
}

mh_sampler_restart::~mh_sampler_restart() {}

prog* mh_sampler_restart::next_restart_proposal(prog *curr) {
  return mod_random_k_cont_insts(*curr, MAX_PROG_LEN);
}

void mh_sampler_restart::mcmc_iter(int niter, const prog &orig,
                                   unordered_map<int, vector<prog*> > &prog_freq) {
  prog *curr, *next;
  curr = prog::make_prog(orig);
  int num_max_iter = niter / _max_iteration;
  int num_remain_iter = niter % _max_iteration;
  prog *start;
  start = prog::make_prog(orig);
  for (int i = 0; i < num_max_iter; i++) {
    mh_sampler::mcmc_iter(_max_iteration, *start, prog_freq);
    prog *restart = next_restart_proposal(start);
    prog::clear_prog(start);
    start = restart;
    // cout << "restart:\n";
    // start->print();
  }
  mh_sampler::mcmc_iter(num_remain_iter, *start, prog_freq);
  prog::clear_prog(start);
}
/* class mh_sampler_restart end */

/* class mh_sampler_k_restart start */
mh_sampler_k_restart::mh_sampler_k_restart(int max_iteration) {
  _max_iteration = max_iteration;
}

mh_sampler_k_restart::~mh_sampler_k_restart() {}

prog* mh_sampler_k_restart::next_restart_proposal(prog *curr) {
  return mod_random_cont_insts(*curr);
}
/* class mh_sampler_k_restart end */
