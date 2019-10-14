#include <iostream>
#include "prog.h"
#include "inout.h"

using namespace std;

// TODO: fix the parameter list for this with better data structures
prog* mh_next(prog* curr, const prog &orig, inout* ex_set, int num_ex);
void mcmc_iter(int niter, const prog &orig,
               std::unordered_map<int, vector<prog*> > &progFreq,
               inout* ex_set, int num_ex, double w_e, double w_p);
double total_prog_cost(prog* curr, const prog& orig, inout* ex_set, int num_ex,
                       double w_e=1.0, double w_p=1.0);
