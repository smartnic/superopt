#include <iostream>
#include <random>
#include <algorithm>
#include <string>
#include <cmath>
#include <unordered_map>
#include "proposals.h"
#include "prog.h"
#include "cost.h"

using namespace std;

default_random_engine gen_mh;
uniform_real_distribution<double> unidist_mh(0.0, 1.0);

double cost_to_pi(double cost) {
  return pow(2, -1.0 * cost);
}

prog* next_proposal(prog* curr) {
  // TODO: use proposal generating functions to generate new programs
  // Use mod_random_inst_operand(.) and mod_random_inst(.) to do so
  return mod_random_inst_operand(*curr);
}

double total_prog_cost(prog* p, const prog &orig, inout* ex_set, int num_ex) {
  return error_cost(ex_set, num_ex,
                    orig.inst_list, orig.prog_length,
                    p->inst_list, p->prog_length);
}

/* compute acceptance function */
double alpha(prog* curr, prog* next, const prog &orig, inout* ex_set, int num_ex) {
  double curr_cost = total_prog_cost(curr, orig, ex_set, num_ex);
  double next_cost = total_prog_cost(next, orig, ex_set, num_ex);
  return min(1.0, cost_to_pi(next_cost) / cost_to_pi(curr_cost));
}

prog* mh_next(prog* curr, const prog &orig, inout* ex_set, int num_ex) {
  prog* next = next_proposal(curr);
  double uni_sample = unidist_mh(gen_mh);
  if (uni_sample < alpha(curr, next, orig, ex_set, num_ex)) {
    return next;
  } else {
    prog::clear_prog(next);
    return curr;
  }
}

void mcmc_iter(int niter, const prog &orig,
               std::unordered_map<prog, int, progHash> &prog_freq,
               inout* ex_set, int num_ex) {
  // contruct the first program by copying the original
  prog *curr, *next;
  curr = prog::make_prog(orig);

  // MCMC iterations
  // IMPORTANT MEM ALLOC NOTE: The following possibilities have been tested
  // on os x g++ version 4.2.1:
  // - the memory of the provided key object is not used as is by storing a
  //   pointer in the map (later `free' of the key object from the map
  //   produced an "object not allocated" error)
  // - the object is not shallow copied into a fixed-size key object in the
  //   buckets of the map (`free' of enclosed allocated objects in the key
  //   produced an "object not allocated" error)
  // - the existing object looks to be deep-copied to form the new key.
  // I couldn't find the documented behavior of the standard allocator used by
  // operator[] of unordered_map. My approach right now is to assume that a
  // deep copy does indeed happen through the new_prog(provided_prog) copy
  // constructor.  This code needs to be investigated further for memory
  // leaks.
  for (int i=0; i<niter; i++) {
    cout << "Attempting mh iteration " << i << endl;
    next = mh_next(curr, orig, ex_set, 2);
    if (prog_freq.find(*next) == prog_freq.end()) prog_freq[*next] = 1;
    else prog_freq[*next]++;
    if (curr != next) prog::clear_prog(curr);
    else curr = next;
  }
}
