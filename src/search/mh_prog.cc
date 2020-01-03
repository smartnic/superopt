#include <iostream>
#include <random>
#include <algorithm>
#include <string>
#include <cmath>
#include <unordered_map>
#include "mh_prog.h"

using namespace std;

default_random_engine gen_mh;
uniform_real_distribution<double> unidist_mh(0.0, 1.0);

/* class mh_sampler_restart start */
mh_sampler_restart::mh_sampler_restart() {
  set_st_when_to_restart(MH_SAMPLER_ST_WHEN_TO_RESTART_NO_RESTART);
  set_st_next_start_prog(MH_SAMPLER_ST_NEXT_START_PROG_ORIG);
  set_we_wp_list(vector<double> {1}, vector<double> {0});
}

mh_sampler_restart::~mh_sampler_restart() {}

void mh_sampler_restart::set_st_when_to_restart(unsigned int st, unsigned int max_num_iter) {
  switch (st) {
    case MH_SAMPLER_ST_WHEN_TO_RESTART_NO_RESTART:
      cout << "set strategy MH_SAMPLER_ST_WHEN_TO_RESTART_NO_RESTART" << endl;
      _st_when_to_start = MH_SAMPLER_ST_WHEN_TO_RESTART_NO_RESTART;
      _max_num_iter = 0;
      return;
    case MH_SAMPLER_ST_WHEN_TO_RESTART_MAX_ITER:
      cout << "set strategy MH_SAMPLER_ST_WHEN_TO_RESTART_MAX_ITER with max_num_iter "
           << max_num_iter << endl;
      _st_when_to_start = MH_SAMPLER_ST_WHEN_TO_RESTART_MAX_ITER;
      _max_num_iter = max_num_iter;
      return;
    default:
      cout << "ERROR: no when_to_restart strategy matches." << endl;
      set_st_when_to_restart(MH_SAMPLER_ST_WHEN_TO_RESTART_NO_RESTART);
      return;
  }
}

void mh_sampler_restart::set_st_next_start_prog(unsigned int st) {
  switch (st) {
    case MH_SAMPLER_ST_NEXT_START_PROG_ORIG:
      cout << "set strategy MH_SAMPLER_ST_NEXT_START_PROG_ORIG" << endl;
      _st_next_start_prog = MH_SAMPLER_ST_NEXT_START_PROG_ORIG;
      return;
    case MH_SAMPLER_ST_NEXT_START_PROG_ALL_INSTS:
      cout << "set strategy MH_SAMPLER_ST_NEXT_START_PROG_ALL_INSTS" << endl;
      _st_next_start_prog = MH_SAMPLER_ST_NEXT_START_PROG_ALL_INSTS;
      return;
    case MH_SAMPLER_ST_NEXT_START_PROG_K_CONT_INSTS:
      cout << "set strategy MH_SAMPLER_ST_NEXT_START_PROG_K_CONT_INSTS" << endl;
      _st_next_start_prog = MH_SAMPLER_ST_NEXT_START_PROG_K_CONT_INSTS;
      return;
    default:
      cout << "ERROR: no next_start_prog strategy matches." << endl;
      set_st_next_start_prog(MH_SAMPLER_ST_NEXT_START_PROG_ORIG);
      return;
  }
}

void mh_sampler_restart::set_we_wp_list(const vector<double> &w_e_list,
                                        const vector<double> &w_p_list) {
  _w_e_list.clear();
  _w_p_list.clear();
  _cur_w_pointer = 0;
  int len = min((int)w_e_list.size(), (int)w_p_list.size());
  for (int i = 0; i < len; i++) {
    _w_e_list.push_back(w_e_list[i]);
    _w_p_list.push_back(w_p_list[i]);
  }
  cout << "set w_e and w_p pairs as: ";
  for (int i = 0; i < _w_e_list.size(); i++) {
    cout << _w_e_list[i] << "," << _w_p_list[i] << " ";
  }
  cout << endl;
}

bool mh_sampler_restart::whether_to_restart(unsigned int iter_num) {
  switch (_st_when_to_start) {
    case MH_SAMPLER_ST_WHEN_TO_RESTART_NO_RESTART: return false;
    case MH_SAMPLER_ST_WHEN_TO_RESTART_MAX_ITER:
      // iter_num starts from 0 but not 1
      if (((iter_num + 1) % _max_num_iter) == 0) return true;
      else return false;
    default:
      cout << "ERROR: no when_to_restart strategy matches. "
           << "return false" << endl;
      return false;
  }
}

prog* mh_sampler_restart::next_start_prog(prog* curr) {
  switch (_st_next_start_prog) {
    case MH_SAMPLER_ST_NEXT_START_PROG_ORIG:
      return curr;
    case MH_SAMPLER_ST_NEXT_START_PROG_ALL_INSTS:
      return mod_random_k_cont_insts(*curr, (*curr).get_max_prog_len());
    case MH_SAMPLER_ST_NEXT_START_PROG_K_CONT_INSTS:
      return mod_random_cont_insts(*curr);
    default:
      cout << "ERROR: no next_start_prog strategy matches. "
           << "return the same program" << endl;
      return curr;
  }
}

pair<double, double> mh_sampler_restart::next_start_we_wp() {
  pair<double, double> we_wp(_w_e_list[_cur_w_pointer], _w_p_list[_cur_w_pointer]);
  _cur_w_pointer = (_cur_w_pointer + 1) % _w_e_list.size();
  return we_wp;
}
/* class mh_sampler_restart end */

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
  double curr_cost = _cost.total_prog_cost(curr, (*curr).get_max_prog_len());
  double next_cost = _cost.total_prog_cost(next, (*next).get_max_prog_len());
  return min(1.0, cost_to_pi(next_cost) / cost_to_pi(curr_cost));
}

prog* mh_sampler::mh_next(prog* curr) {
  prog* next = _next_proposal.next_proposal(curr);
  next->canonicalize();
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

void mh_sampler::print_restart_info(int iter_num, const prog &restart, double w_e, double w_p) {
  cout << "restart at iteration " << iter_num << endl;
  cout << "  restart w_e, w_p: " << w_e << ", " << w_p << endl;
  cout << "  restart program" << endl;
  restart.print(restart);
}

void mh_sampler::mcmc_iter(int niter, const prog &orig,
                           unordered_map<int, vector<prog*> > &prog_freq) {
  prog *curr, *next;
  curr = prog::make_prog(orig);
  curr->canonicalize();
  for (int i = 0; i < niter; i++) {
    // check whether need restart, if need, update `start`
    if (_restart.whether_to_restart(i)) {
      prog *restart = _restart.next_start_prog(curr);
      if (curr != restart) {
        prog::clear_prog(curr);
        curr = restart;
        curr->canonicalize();
      }
      pair<double, double> restart_we_wp = _restart.next_start_we_wp();
      _cost._w_e = restart_we_wp.first;
      _cost._w_p = restart_we_wp.second;
      print_restart_info(i, *restart, restart_we_wp.first, restart_we_wp.second);
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
