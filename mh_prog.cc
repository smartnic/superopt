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
  _measure_mode = false;
  _measure_count = 0;
}

mh_sampler::~mh_sampler() {}

void mh_sampler::open_measure_file(string file_raw_data_prog,
                                   string file_raw_data_proposal,
                                   string file_raw_data_ex) {
  _f_program.open(file_raw_data_prog, ios::out | ios::trunc);
  _f_proposal.open(file_raw_data_proposal, ios::out | ios::trunc);
  _f_examples.open(file_raw_data_ex, ios::out | ios::trunc);
  _measure_mode = true;
  _measure_count = 0;
}

void mh_sampler::close_measure_file() {
  _f_program.close();
  _f_proposal.close();
  _f_examples.close();
  _measure_mode = false;
  _measure_count = 0;
}

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
  if (_measure_mode) {
    _f_proposal << _cost.error_cost(next, MAX_PROG_LEN) << " "
                << _cost.perf_cost(next, MAX_PROG_LEN) << " "
                << _cost.total_prog_cost(next, MAX_PROG_LEN) << " "
                << _cost._examples._exs.size() << " ";
    _f_examples << _cost._examples._exs.size() << ":" << _cost._examples._exs << endl;
  }
  return min(1.0, cost_to_pi(next_cost) / cost_to_pi(curr_cost));
}

prog* mh_sampler::mh_next(prog* curr) {
  prog* next = next_proposal(curr);
  double uni_sample = unidist_mh(gen_mh);
  double a = alpha(curr, next);
  if (_measure_mode) {
    // use command line to redirect to file
    cout << "iteration " << _measure_count << ": ";
    if (uni_sample < a) cout << "accepted" << endl;
    else cout << "rejected" << endl;
    next->print();
    _measure_count++;
    _f_proposal << uni_sample << " " << a << " ";
    if (uni_sample < a) _f_proposal << "1" << endl;
    else _f_proposal << "0" << endl;
  }
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
  curr = prog::make_prog(orig);

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
    if (_measure_mode) {
      _f_program << meas_p->_error_cost << " "
                 << meas_p->_perf_cost << " "
                 << (_cost._w_e * (double)meas_p->_error_cost +
                     _cost._w_p * (double)meas_p->_perf_cost) << " "
                 << meas_p->freq_count << endl;
    }
    if (curr != next) prog::clear_prog(curr);
    curr = next;
  }
}
