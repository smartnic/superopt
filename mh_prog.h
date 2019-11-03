#include <iostream>
#include "prog.h"
#include "inout.h"
#include "cost.h"

using namespace std;

class mh_sampler {
 private:
  double cost_to_pi(double cost);
  prog* next_proposal(prog* curr);
 public:
  cost _cost;
  double _base = 2.0;
  mh_sampler();
  ~mh_sampler();
  double alpha(prog* curr, prog* next);
  prog* mh_next(prog* curr);
  void mcmc_iter(int niter, const prog &orig,
                 unordered_map<int, vector<prog*> > &prog_freq);
};
