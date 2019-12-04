#include <iostream>
#include <fstream>
#include "prog.h"
#include "inout.h"
#include "cost.h"
#include "measure/meas_mh_data.h"

using namespace std;

// 1. when to restart strategy
#define MH_SAMPLER_ST_WHEN_TO_RESTART_NO_RESTART 0
#define MH_SAMPLER_ST_WHEN_TO_RESTART_MAX_ITER 1
// 2. start prog strategy
#define MH_SAMPLER_ST_NEXT_START_PROG_ORIG 0
#define MH_SAMPLER_ST_NEXT_START_PROG_ALL_INSTS 1
#define MH_SAMPLER_ST_NEXT_START_PROG_K_CONT_INSTS 2

class mh_sampler_when_to_restart {
 public:
  unsigned int _st;
  // restart every `_max_num_iter` iterations
  unsigned int _max_num_iter;
  mh_sampler_when_to_restart();
  ~mh_sampler_when_to_restart();
  void set_st(unsigned int st, unsigned int max_num_iter = 1000);
  bool whether_to_restart(unsigned int iter_num);
};

class mh_sampler_next_start_prog {
 public:
  unsigned int _st;
  mh_sampler_next_start_prog();
  ~mh_sampler_next_start_prog();
  void set_st(unsigned int st);
  prog* next_start_prog(prog* curr);
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
  // 2. threshold mod_random_inst is sum of the probablities of mod_random_inst_operand
  // and mod_random_inst
  double _thr_mod_random_inst_operand;
  double _thr_mod_random_inst;
  mh_sampler_next_proposal();
  ~mh_sampler_next_proposal();
  void set_probability(double p_mod_random_inst_operand,
                       double p_mod_random_inst);
  prog* next_proposal(prog* curr);
};

/* Class mh_sampler can be used to generate a chain of sampled programs for a
 * given program. The sampled probability mainly depends on cost function,
 * start program, moves.
 * Example to use a mh_sampler:
 *   mh_sampler mh;        // define a `mh_sampler` variable
 *   mh.[strategy].set_st_*(.) // [optional] set different mh sampler strategy, 
 *                             // the default is used without setting
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
  void print_restart_info(int iter_num, const prog &curr, const prog &restart);
 public:
  mh_sampler_when_to_restart _when_to_restart;
  mh_sampler_next_start_prog _start_prog;
  mh_sampler_next_proposal _next_proposal;
  meas_mh_data _meas_data;
  cost _cost;
  double _base = 2;
  mh_sampler();
  ~mh_sampler();
  double alpha(prog* curr, prog* next);
  prog* mh_next(prog* curr);
  void turn_on_measure();
  void turn_off_measure();
  void mcmc_iter(int niter, const prog &orig,
                 unordered_map<int, vector<prog*> > &prog_freq);
};
