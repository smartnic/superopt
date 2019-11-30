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

/* class mh_sampler_when_to_restart start */
mh_sampler_when_to_restart::mh_sampler_when_to_restart() {
  set_st_no_restart();
}

mh_sampler_when_to_restart::~mh_sampler_when_to_restart() {}

void mh_sampler_when_to_restart::set_st_no_restart() {
  cout << "set strategy MH_SAMPLER_ST_WHEN_TO_RESTART_NO_RESTART" << endl;
  _st = MH_SAMPLER_ST_WHEN_TO_RESTART_NO_RESTART;
}

void mh_sampler_when_to_restart::set_st_max_iter(unsigned int max_num_iter) {
  cout << "set strategy MH_SAMPLER_ST_WHEN_TO_RESTART_MAX_ITER with max_num_iter "
       << max_num_iter << endl;
  _st = MH_SAMPLER_ST_WHEN_TO_RESTART_MAX_ITER;
  _max_num_iter = max_num_iter;
}

void mh_sampler_when_to_restart::set_st(unsigned int st, unsigned int max_num_iter) {
  switch (st) {
    case MH_SAMPLER_ST_WHEN_TO_RESTART_NO_RESTART: set_st_no_restart(); return;
    case MH_SAMPLER_ST_WHEN_TO_RESTART_MAX_ITER: set_st_max_iter(max_num_iter); return;
    default:
      cout << "ERROR: no when_to_restart strategy matches. "
           << "Set as MH_SAMPLER_ST_WHEN_TO_RESTART_NO_RESTART" << endl;
      set_st_no_restart();
      return;
  }
}

bool mh_sampler_when_to_restart::whether_to_restart(unsigned int iter_num) {
  switch (_st) {
    case MH_SAMPLER_ST_WHEN_TO_RESTART_NO_RESTART: return false;
    case MH_SAMPLER_ST_WHEN_TO_RESTART_MAX_ITER:
      // iter_num starts from 0 but not 1
      if (((iter_num + 1) % _max_num_iter) == 0) return true;
      else return false;
    default:
      cout << "ERROR: no when_to_restart strategy matches." << endl;
      return false;
  }
}
/* class mh_sampler_when_to_restart end */

/* class mh_sampler_next_start_prog start */
mh_sampler_next_start_prog::mh_sampler_next_start_prog() {
  set_st_orig();
}

mh_sampler_next_start_prog::~mh_sampler_next_start_prog() {}

void mh_sampler_next_start_prog::set_st_orig() {
  cout << "set strategy MH_SAMPLER_ST_NEXT_START_PROG_ORIG" << endl;
  _st = MH_SAMPLER_ST_NEXT_START_PROG_ORIG;
}

void mh_sampler_next_start_prog::set_st_all_insts() {
  cout << "set strategy MH_SAMPLER_ST_NEXT_START_PROG_ALL_INSTS" << endl;
  _st = MH_SAMPLER_ST_NEXT_START_PROG_ALL_INSTS;
}

void mh_sampler_next_start_prog::set_st_k_cont_insts() {
  cout << "set strategy MH_SAMPLER_ST_NEXT_START_PROG_K_CONT_INSTS" << endl;
  _st = MH_SAMPLER_ST_NEXT_START_PROG_K_CONT_INSTS;
}

void mh_sampler_next_start_prog::set_st(unsigned int st) {
  switch (st) {
    case MH_SAMPLER_ST_NEXT_START_PROG_ORIG: set_st_orig(); return;
    case MH_SAMPLER_ST_NEXT_START_PROG_ALL_INSTS: set_st_all_insts(); return;
    case MH_SAMPLER_ST_NEXT_START_PROG_K_CONT_INSTS: set_st_k_cont_insts(); return;
    default:
      cout << "ERROR: no next_start_prog strategy matches."
           << "Set as MH_SAMPLER_ST_NEXT_START_PROG_ORIG" << endl;
      set_st_orig();
      return;
  }
}

prog* mh_sampler_next_start_prog::next_start_prog(prog* curr) {
  switch (_st) {
    case MH_SAMPLER_ST_NEXT_START_PROG_ORIG:
      return curr;
    case MH_SAMPLER_ST_NEXT_START_PROG_ALL_INSTS:
      return mod_random_k_cont_insts(*curr, MAX_PROG_LEN);
    case MH_SAMPLER_ST_NEXT_START_PROG_K_CONT_INSTS:
      return mod_random_cont_insts(*curr);
    default:
      cout << "ERROR: no next_start_prog strategy matches." << endl;
      return curr;
  }
}
/* class mh_sampler_next_start_prog end */

/* class mh_sampler_next_proposal start */
mh_sampler_next_proposal::mh_sampler_next_proposal() {
  _thr_mod_random_inst_operand = 1.0 / 3.0;
  _thr_mod_random_inst = 2.0 / 3.0;
  cout << "probabilities of mod_random_inst_operand, mod_random_inst, "
       << "mod_random_k_cont_insts are all set as "
       << 1.0 / 3.0 << endl;
}

mh_sampler_next_proposal::~mh_sampler_next_proposal() {}

void mh_sampler_next_proposal::set_probability(
  double p_mod_random_inst_operand,
  double p_mod_random_inst) {
  _thr_mod_random_inst_operand = p_mod_random_inst_operand;
  _thr_mod_random_inst = _thr_mod_random_inst_operand + p_mod_random_inst;
  cout << "probabilities of mod_random_inst_operand, mod_random_inst, "
       << "mod_random_k_cont_insts are set as "
       <<  _thr_mod_random_inst_operand << ", "
       << _thr_mod_random_inst - _thr_mod_random_inst_operand << ", "
       << 1.0 - _thr_mod_random_inst << endl;
}

prog* mh_sampler_next_proposal::next_proposal(prog* curr) {
  double uni_sample = unidist_mh(gen_mh);
  if (uni_sample <= _thr_mod_random_inst_operand) {
    return mod_random_inst_operand(*curr);
  } else if (uni_sample <= _thr_mod_random_inst) {
    return mod_random_inst(*curr);
  } else {
    return mod_random_k_cont_insts(*curr, 2);
  }
}
/* class mh_sampler_next_proposal end */

/* class mh_sampler start */
mh_sampler::mh_sampler() {
  _meas_data._mode = false;
}

mh_sampler::~mh_sampler() {}

double mh_sampler::cost_to_pi(double cost) {
  return pow(_base, -1.0 * cost);
}

/* compute acceptance function */
double mh_sampler::alpha(prog* curr, prog* next) {
  double curr_cost = _cost.total_prog_cost(curr, MAX_PROG_LEN);
  double next_cost = _cost.total_prog_cost(next, MAX_PROG_LEN);
  return min(1.0, cost_to_pi(next_cost) / cost_to_pi(curr_cost));
}

prog* mh_sampler::mh_next(prog* curr) {
  prog* next = _next_proposal.next_proposal(curr);
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

/* Insert the next program into frequency dictionary if new */
void insert_into_prog_freq_dic(const prog &next,
                               unordered_map<int, vector<prog*> > &prog_freq) {
  int ph = progHash()(next);
  bool found = false;
  if (prog_freq.find(ph) == prog_freq.end()) {
    prog_freq[ph] = std::vector<prog*>();
  } else {
    vector<prog*> chain = prog_freq[ph];
    for (auto p : chain) {
      if (*p == next) {
        found = true;
        p->freq_count++;
        break;
      }
    }
  }
  if (! found) {
    // TODO: may define new prog copy API
    prog* next_copy = prog::make_prog(next);
    next_copy->freq_count++;
    next_copy->_error_cost = next._error_cost;
    next_copy->_perf_cost = next._perf_cost;
    prog_freq[ph].push_back(next_copy);
  }
}

void mh_sampler::print_restart_info(int iter_num, const prog &curr, const prog &restart) {
  cout << "restart at iteration " << iter_num << endl;
  cout << "  current program" << endl;
  curr.print(curr);
  cout << "  restart program" << endl;
  restart.print(restart);
}

void mh_sampler::mcmc_iter(int niter, const prog &orig,
                           unordered_map<int, vector<prog*> > &prog_freq) {
  prog *curr, *next;
  curr = prog::make_prog(orig);
  for (int i = 0; i < niter; i++) {
    // check whether need restart, if need, update `start`
    if (_when_to_restart.whether_to_restart(i)) {
      prog *restart = _start_prog.next_start_prog(curr);
      print_restart_info(i, *curr, *restart);
      if (curr != restart) {
        prog::clear_prog(curr);
        curr = restart;
      }
    }
    // sample one program
    next = mh_next(curr);
    // insert the next program into frequency dictionary of programs
    insert_into_prog_freq_dic(*next, prog_freq);
    // update measurement data and update current program with the next program
    if (curr != next) {
      prog::clear_prog(curr);
      _meas_data.insert_program(i, *next);
    } else if (i == 0) {
      _meas_data.insert_program(i, *curr);
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
