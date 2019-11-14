#include <iostream>
#include <fstream>
#include "prog.h"
#include "inout.h"
#include "cost.h"

using namespace std;

class mh_sampler {
 private:
  // when open_measure_file is called, set _measure_mode as true;
  // when close_measure_file is called, set _measure_mode as false;
  bool _measure_mode = false;
  int _measure_count = 0;
  ofstream _f_program;
  ofstream _f_proposal;
  ofstream _f_examples;
  double cost_to_pi(double cost);
  prog* next_proposal(prog* curr);
 public:
  cost _cost;
  double _base = 2;
  mh_sampler();
  ~mh_sampler();
  double alpha(prog* curr, prog* next);
  prog* mh_next(prog* curr);
  void mcmc_iter(int niter, const prog &orig,
                 unordered_map<int, vector<prog*> > &prog_freq);
  void open_measure_file(string file_raw_data_prog,
                         string file_raw_data_proposal,
                         string file_raw_data_ex);
  void close_measure_file();
};
