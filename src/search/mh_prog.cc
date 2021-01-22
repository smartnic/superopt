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

mh_sampler_next_win::mh_sampler_next_win() {
  vector<int> win_s_list = {0}, win_e_list = {inst::max_prog_len - 1};
  set_win_lists(win_s_list, win_e_list);
  _cur_win = 0;
  set_max_num_iter(10);
}

void mh_sampler_next_win::set_win_lists(const vector<int>& win_s_list,
                                        const vector<int>& win_e_list) {
  assert(win_s_list.size() == win_e_list.size());
  _win_s_list.resize(win_s_list.size());
  _win_e_list.resize(win_s_list.size());
  cout << "set_win_lists: ";
  for (int i = 0; i < _win_s_list.size(); i++) {
    _win_s_list[i] = win_s_list[i];
    _win_e_list[i] = win_e_list[i];
    cout << "[" << _win_s_list[i] << ", " << _win_e_list[i] << "] ";
  }
  cout << endl;
}

void mh_sampler_next_win::set_max_num_iter(unsigned int max_num_iter) {
  _max_num_iter = max_num_iter;
  cout << "set window max_num_iter as " << _max_num_iter << endl;

}

bool mh_sampler_next_win::whether_to_reset(unsigned int iter_num) {
  // iter_num starts from 0 but not 1
  if ((iter_num % _max_num_iter) == 0) return true;
  else return false;
}

pair<int, int> mh_sampler_next_win::update_and_get_next_win() {
  pair<int, int> win = pair<int, int>(_win_s_list[_cur_win], _win_e_list[_cur_win]);
  _cur_win = (_cur_win + 1) % _win_s_list.size();
  return win;
}

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
      if ((iter_num % _max_num_iter) == 0) return true;
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
      return mod_random_k_cont_insts(*curr, inst::max_prog_len);
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
  set_win(0, inst::max_prog_len - 1);
  _thr_mod_random_inst_operand = 1.0 / 4.0;
  _thr_mod_random_inst = 2.0 / 4.0;
  _thr_mod_random_inst_as_nop = 3.0 / 4.0;
  cout << "probabilities of mod_random_inst_operand, mod_random_inst, "
       << "mod_random_k_cont_insts are all set as "
       << 1.0 / 4.0 << endl;
}

mh_sampler_next_proposal::~mh_sampler_next_proposal() {}

void mh_sampler_next_proposal::set_probability(
  double p_mod_random_inst_operand,
  double p_mod_random_inst,
  double p_mod_random_inst_as_nop) {
  _thr_mod_random_inst_operand = p_mod_random_inst_operand;
  _thr_mod_random_inst = _thr_mod_random_inst_operand + p_mod_random_inst;
  _thr_mod_random_inst_as_nop = _thr_mod_random_inst + p_mod_random_inst_as_nop;
  cout << "probabilities of mod_random_inst_operand, mod_random_inst, "
       << "mod_random_inst_as_nop and mod_random_k_cont_insts are set as "
       << _thr_mod_random_inst_operand << ", "
       << _thr_mod_random_inst - _thr_mod_random_inst_operand << ", "
       << _thr_mod_random_inst_as_nop - _thr_mod_random_inst << ", "
       << 1.0 - _thr_mod_random_inst_as_nop << endl;
}

void mh_sampler_next_proposal::set_win(int start, int end) {
  _win_start = start;
  _win_end = end;
  cout << "window start and end set as ["
       << _win_start << ", " << _win_end << "]" << endl;
}

prog* mh_sampler_next_proposal::next_proposal(prog* curr) {
  double uni_sample = unidist_mh(gen_mh);
  if (uni_sample <= _thr_mod_random_inst_operand) {
    return mod_random_inst_operand(*curr, _win_start, _win_end);
  } else if (uni_sample <= _thr_mod_random_inst) {
    return mod_random_inst(*curr, _win_start, _win_end);
  } else if (uni_sample <= _thr_mod_random_inst_as_nop) {
    return mod_random_inst_as_nop(*curr, _win_start, _win_end);
  } else {
    return mod_random_k_cont_insts(*curr, 2, _win_start, _win_end);
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
double mh_sampler::alpha(prog* curr, prog* next, prog* orig) {
  double curr_cost = _cost.total_prog_cost(orig, inst::max_prog_len, curr, inst::max_prog_len);
  double next_cost = _cost.total_prog_cost(orig, inst::max_prog_len, next, inst::max_prog_len);
  // res = min(1.0, cost_to_pi(next_cost) / cost_to_pi(curr_cost));
  // use equation b^(-x) / b^(-y) = b^(-(x-y)) to simplify the calculation
  double d = next_cost - curr_cost;
  if (d <= 0) return 1;
  else return cost_to_pi(d);
}

prog* mh_sampler::mh_next(prog* curr, prog* orig) {
  prog* next = _next_proposal.next_proposal(curr);
  // print each modification
  if (logger.is_print_level(LOGGER_DEBUG)) {
    for (int i = _next_proposal._win_start; i <= _next_proposal._win_end; i++) {
      cout << i << ": ";
      next->inst_list[i].print();
    }
  }
  // next->canonicalize();
  double uni_sample = unidist_mh(gen_mh);
  double a = alpha(curr, next, orig);
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
    delete next;
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
    prog* next_copy = new prog(next);
    next_copy->freq_count = 1;
    prog_freq[ph].push_back(next_copy);
  }
}

// return best program in input programs
// if there is no program with zero error cost, return nullptr.
// else return best program: smallest performance cost + zero error cost
prog* get_best_pgm_from_pgm_dic(unordered_map<int, vector<prog*> >& pgm_dic) {
  prog* best = nullptr;
  double min_perf_cost = numeric_limits<double>::max();
  // get the minimum performance cost with zero error cost
  for (auto& element : pgm_dic) {
    vector<prog*>& pl = element.second; // list of progs with the same hash
    for (auto p : pl) {
      if (p->_error_cost != 0) continue;
      if (min_perf_cost <= p->_perf_cost) continue;
      min_perf_cost = p->_perf_cost;
      best = p;
    }
  }
  return best;
}


void clear_prog_freq_dic(unordered_map<int, vector<prog*> > &pgm_dic) {
  // get the minimum performance cost with zero error cost
  for (auto& element : pgm_dic) {
    vector<prog*>& pl = element.second; // list of progs with the same hash
    for (auto p : pl) {
      delete p;
    }
  }
  pgm_dic.clear();
}

void mh_sampler::print_restart_info(int iter_num, const prog &restart, double w_e, double w_p) {
  cout << "restart at iteration " << iter_num << endl;
  cout << "  restart w_e, w_p: " << w_e << ", " << w_p << endl;
  cout << "  restart program" << endl;
  restart.print();
}

void mh_sampler::mcmc_iter(top_k_progs& topk_progs, int niter, prog* orig, bool is_win) {
  topk_progs.clear();
  // best is the program with zero error cost and minimum performance cost in MC
  prog *curr, *next, *prog_start, *best;
  // curr->canonicalize();
  auto start = NOW;
  prog_start = new prog(*orig);
  // set the error cost and perf cost of the prog start
  prog_start->_error_cost = 0;
  _cost.perf_cost(prog_start, inst::max_prog_len);
  cout << "original program's perf cost: " << prog_start->_perf_cost << endl;

  best = new prog(*prog_start);
  curr = new prog(*prog_start);
  topk_progs.insert(prog_start);
  if (topk_progs.size() == 0) cout << "ERROR: not able to insert original program" << endl;
  else cout << "insert original program in topk_progs" << endl;

  for (int i = 0; i < niter; i++) {
    if (logger.is_print_level(LOGGER_DEBUG)) {
      cout << "iter: " << i << endl;
    }
    if (_next_win.whether_to_reset(i)) {
      pair<int, int> win = _next_win.update_and_get_next_win();
      cout << "set window at iteration " << i << endl;
      _next_proposal.set_win(win.first, win.second);
      // update prog_start to best and
      // set prog_start as the start program for the new window
      assert(best != nullptr);
      if (prog_start != best) {
        delete prog_start;
        prog_start = new prog(*best);
      }
      if (curr != prog_start) {
        delete curr;
        curr = new prog(*prog_start);
      }
      cout << "start from program (error and performance costs: "
           << prog_start->_error_cost << " " << prog_start-> _perf_cost << "):" << endl;
      prog_start->print();
      if (is_win) { // reset validator original program
        inout_t::start_insn = win.first;
        inout_t::end_insn = win.second;
        static_safety_check_pgm(prog_start->inst_list, inst::max_prog_len);
        _cost.set_orig(prog_start, inst::max_prog_len, win.first, win.second);
        // clear the test cases and generate new test cases
        prog_static_state pss;
        static_analysis(pss, prog_start->inst_list, inst::max_prog_len);
        int num_examples = 30;
        vector<inout_t> examples;
        gen_random_input_for_win(examples, num_examples, pss.static_state[win.first], win.first, win.second);
        _cost.set_examples(examples, prog_start);
      }
    }
    // check whether need restart, if need, update `start`
    if (_restart.whether_to_restart(i)) {
      prog *restart = _restart.next_start_prog(curr);
      if (curr != restart) {
        delete curr;
        curr = restart;
        // curr->canonicalize();
      }
      pair<double, double> restart_we_wp = _restart.next_start_we_wp();
      _cost._w_e = restart_we_wp.first;
      _cost._w_p = restart_we_wp.second;
      print_restart_info(i, *restart, restart_we_wp.first, restart_we_wp.second);
    }
    // sample one program
    next = mh_next(curr, prog_start);
    // update best by next
    if ((next->_error_cost == 0) && (next->_perf_cost < best->_perf_cost)) {
      cout << "find a better program at " << i
           << " cost: " << next->_error_cost << " " << next->_perf_cost << endl;
      for (int i = _next_proposal._win_start; i <= _next_proposal._win_end; i++) {
        cout << i << ": ";
        next->inst_list[i].print();
      }
      delete best;
      best = new prog(*next);
      topk_progs.insert(*best); // also insert better program
    }
    // update measurement data and update current program with the next program
    if (curr != next) {
      delete curr;
      _meas_data.insert_program(i, *next);
    } else if (i == 0) {
      _meas_data.insert_program(i, *curr);
    }
    curr = next;
    topk_progs.insert(curr);

    auto end = NOW;
    if (logger.is_print_level(LOGGER_DEBUG)) {
      cout << "iter_timestamp: " << DUR(start, end) << endl;
    }
  }
  delete curr;
}

void mh_sampler::turn_on_measure() {
  _meas_data._mode = true;
}

void mh_sampler::turn_off_measure() {
  _meas_data._mode = false;
}
/* class mh_sampler_restart end */
