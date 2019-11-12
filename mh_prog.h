#include <iostream>
#include <fstream>
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
  double _base = 2;
  mh_sampler();
  ~mh_sampler();
  double alpha(prog* curr, prog* next);
  prog* mh_next(prog* curr);
  void mcmc_iter(int niter, const prog &orig,
                 unordered_map<int, vector<prog*> > &prog_freq,
                 vector<prog*>& progs);
  ofstream fout1, fout2, fout3, fout4;
  void init(string f1, string f2, string f3, string f4);
  void close();
};
