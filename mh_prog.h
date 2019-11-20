#include <iostream>
#include <fstream>
#include "prog.h"
#include "inout.h"
#include "cost.h"
#include "measure/meas_mh_data.h"

using namespace std;

/* Class mh_sampler can be used to generate a chain of sampled programs for a
 * given program. The sampled probability mainly depends on cost function,
 * start program, moves.
 * Example to use a mh_sampler:
 *   mh_sampler mh;        // define a `mh_sampler` variable
 *   mh.turn_on_measure(); // [optional] turn on measure mode if measurement needed
 *   mh._cost.init(.);     // initialize the parameters of cost function
 *                         // view `cost.h` for more details
 *   mh.mcmc_iter(.);      // sample programs
 *   store_*_to_file(.)    // [optional] call data store function(s) to store
 *                         // measurement data; view `meas_mh_data.h` for more details
 *   mh.turn_off_measure();// [optional] if turn on measure mode, should turn off
 */
class mh_sampler {
 protected:
  double cost_to_pi(double cost);
  prog* next_proposal(prog* curr);
  void insert_into_prog_dic(const prog *next,
                            unordered_map<int, vector<prog*> > &prog_freq);
 public:
  meas_mh_data _meas_data;
  cost _cost;
  double _base = 2;
  mh_sampler();
  ~mh_sampler();
  double alpha(prog* curr, prog* next);
  prog* mh_next(prog* curr);
  void turn_on_measure();
  void turn_off_measure();
  virtual void mcmc_iter(int niter, const prog &orig,
                         unordered_map<int, vector<prog*> > &prog_freq);
};

class mh_sampler_restart: public mh_sampler {
 protected:
  virtual prog* next_restart_proposal(prog* curr);
 public:
  unsigned int _max_iteration;
  mh_sampler_restart(int max_iteration);
  ~mh_sampler_restart();
  void mcmc_iter(int niter, const prog &orig,
                 unordered_map<int, vector<prog*> > &prog_freq);
};

class mh_sampler_k_restart: public mh_sampler_restart {
 private:
  prog* next_restart_proposal(prog *curr);
 public:
  mh_sampler_k_restart(int max_iteration);
  ~mh_sampler_k_restart();
};
