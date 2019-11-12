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


// string file_raw_data_ex = "raw_data_ex";
// string file_raw_data_prog = "raw_data_prog";
// string file_raw_data_error_perf = "raw_data_error_perf";
// string file_raw_data_error_ex = "raw_data_error_ex";
void mh_sampler::init(string f1, string f2, string f3, string f4) {
  fout1.open(f1, ios::out | ios::trunc);
  // fout2.open(f2, ios::out | ios::trunc);
  fout3.open(f3, ios::out | ios::trunc);
  // fout4.open(f4, ios::out | ios::trunc);
}

void mh_sampler::close() {
  fout1.close();
  // fout2.close();
  fout3.close();
  // fout4.close();
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
  // cout << "cal curr" << endl;
  double curr_cost = _cost.total_prog_cost(curr, MAX_PROG_LEN);
  // cout << "cal next" << endl;
  double next_cost = _cost.total_prog_cost(next, MAX_PROG_LEN);
  fout3 << _cost.error_cost(next, MAX_PROG_LEN) << " "
        << _cost.perf_cost(next, MAX_PROG_LEN) << " "
        << _cost.total_prog_cost(next, MAX_PROG_LEN) << " "
        << _cost._examples._exs.size() << " ";
  fout1 << _cost._examples._exs.size() << ":" << _cost._examples._exs << endl;
  // cout << "Costs: curr " << curr_cost << " next " << next_cost << endl;
  return min(1.0, cost_to_pi(next_cost) / cost_to_pi(curr_cost));
}

prog* mh_sampler::mh_next(prog* curr) {
  prog* next = next_proposal(curr);
  next->print();
  double uni_sample = unidist_mh(gen_mh);
  // cout << "uni_sample: " << uni_sample << " ";
  double a = alpha(curr, next);
  fout3 << uni_sample << " " << a << " ";
  if (uni_sample < a) {
    // cout << "move to next prog " << to_string(iter) << endl;
    fout3 << "1" << endl;
    return next;
  } else {
    fout3 << "0" << endl;
    prog::clear_prog(next);
    return curr;
  }
}

void mh_sampler::mcmc_iter(int niter, const prog &orig,
                           unordered_map<int, vector<prog*> > &prog_freq,
                           vector<prog*>& progs) {
  // contruct the first program by copying the original
  prog *curr, *next;
  curr = prog::make_prog(orig);

  for (int i = 0; i < niter; i++) {
    // cout << "Attempting mh iteration " << i << endl;
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
          progs[i] = p; // insert into progs
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
      progs[i] = next_copy; // insert into progs
    }
    if (curr != next) prog::clear_prog(curr);
    curr = next;
  }
}
