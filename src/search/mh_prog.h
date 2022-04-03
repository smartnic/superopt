#include <iostream>
#include <fstream>
#include <utility>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "../../src/utils.h"
#include "../../src/inout.h"
#include "../../src/isa/inst_header.h"
#include "../../src/isa/prog.h"
#include "proposals.h"
#include "cost.h"
#include "../../measure/meas_mh_bhv.h"

using namespace std;

// 1. when to restart strategy
#define MH_SAMPLER_ST_WHEN_TO_RESTART_NO_RESTART 0
#define MH_SAMPLER_ST_WHEN_TO_RESTART_MAX_ITER 1
// 2. start prog strategy
#define MH_SAMPLER_ST_NEXT_START_PROG_ORIG 0
#define MH_SAMPLER_ST_NEXT_START_PROG_ALL_INSTS 1
#define MH_SAMPLER_ST_NEXT_START_PROG_K_CONT_INSTS 2

class mh_sampler_next_win {
public:
  unsigned int _st_next_win;
  unsigned int _max_num_iter;
  vector<int> _win_s_list;
  vector<int> _win_e_list;
  unsigned int _cur_win;
  mh_sampler_next_win();
  void set_win_lists(const vector<int>& win_s_list, const vector<int>& win_e_list);
  void set_max_num_iter(unsigned int max_num_iter);
  bool whether_to_reset(unsigned int iter_num);
  pair<int, int> update_and_get_next_win();
};

class mh_sampler_restart {
public:
  unsigned int _st_when_to_start;
  // restart every `_max_num_iter` iterations
  unsigned int _max_num_iter;
  unsigned int _st_next_start_prog;
  vector<double> _w_e_list;
  vector<double> _w_p_list;
  vector<double> _w_s_list;
  size_t _cur_w_pointer;
  mh_sampler_restart();
  ~mh_sampler_restart();
  void set_st_when_to_restart(unsigned int st, unsigned int max_num_iter = 0);
  void set_st_next_start_prog(unsigned int st);
  void set_we_wp_ws_list(const vector<double> &w_e_list, const vector<double> &w_p_list, const vector<double> &w_s_list);
  bool whether_to_restart(unsigned int iter_num);
  prog* next_start_prog(prog* curr);
  pair<double, double> next_start_we_wp();
};

/* The main function of class mh_sampler_next_proposal is to
 * generate next proposal program according to the probability of different methods.
 *
 * Next proposal program can be generate by function next_proposal(.), noting that
 * when generate next proposal program, the sum of all probabilities is assumed as 1.
 *
 * Three methods are supported now, that is,
 * modify random instrution operand, instruction and two continuous instructions.
 *
 * The probabilities three methods can be set by set_probability(.).
 */
class mh_sampler_next_proposal {
public:
  // `_thr_*` variables are used as thresholds when using uniform sample to
  // randomly choose different proposal generating methods. View more details in next_proposal(.)
  // 1. threshold mod_random_inst_operand is the probablity of mod_random_inst_operand
  // 2. threshold mod_random_inst is sum of the probablities of mod_random_inst_operand and mod_random_inst
  // 3. threshold mod_random_inst_as_nop is the probablity of mod_random_inst_operand,
  //    mod_random_inst and mod_random_inst_as_nop
  double _thr_mod_random_inst_operand;
  double _thr_mod_random_inst;
  double _thr_mod_random_inst_as_nop;
  int _win_start, _win_end;
  mh_sampler_next_proposal();
  ~mh_sampler_next_proposal();
  void set_probability(double p_mod_random_inst_operand,
                       double p_mod_random_inst,
                       double p_mod_random_inst_as_nop);
  void set_win(int start, int end);
  prog* next_proposal(prog* curr);
};

/* Class mh_sampler can be used to generate a chain of sampled programs for a
 * given program. The sampled probability mainly depends on cost function,
 * start program, moves.
 * Example to use a mh_sampler:
 *   mh_sampler mh;        // define a `mh_sampler` variable
 *   mh._restart.set_st_*(.) // [optional] set different mh sampler strategies,
 *                           // the default values are used without setting
 *   mh._next_proposal.set_probability(.) // [optional] set probabilities of different moves
 *                                        // the default values are used without setting
 *   mh.turn_on_measure(); // [optional] turn on measure mode if measurement needed
 *   mh._cost.init(.);     // initialize the parameters of cost function
 *                         // view `cost.h` for more details
 *   mh.mcmc_iter(.);      // sample programs
 *   store_*_to_file(.)    // [optional] call data store function(s) to store
 *                         // measurement data; view `meas_mh_data.h` for more details
 *   mh.turn_off_measure();// [optional] if turn on measure mode, should turn off
 */
class mh_sampler {
private:
  double cost_to_pi(double cost);
  void print_restart_info(int iter_num, const prog &restart, double w_e, double w_p);
public:
  mh_sampler_next_win _next_win;
  mh_sampler_restart _restart;
  mh_sampler_next_proposal _next_proposal;
  meas_mh_data _meas_data;
  cost _cost;
  double _base = 2;
  bool _enable_better_store = true;  // enable store the program when a better program is found
  string _better_store_path = "output/"; // path to store the programwhen a better program is found
  int _max_n_better_store = 1; // max number of programs to be stored
  mh_sampler();
  ~mh_sampler();
  double alpha(prog* curr, prog* next, prog* orig);
  prog* mh_next(prog* curr, prog* orig);
  void turn_on_measure();
  void turn_off_measure();
  void mcmc_iter(top_k_progs& topk_progs, int niter, prog* orig, bool is_win = false);
};

void write_optimized_prog_to_file(prog* current_program, int id, string path_out);
