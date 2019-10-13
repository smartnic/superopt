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
  // return mod_random_inst_operand(*curr);
  return mod_random_inst(*curr);
}

double total_prog_cost(prog* p, const prog &orig, inout* ex_set, int num_ex,
                       double w_e=1.0, double w_p=1.0) {
  double err_cost = error_cost(ex_set, num_ex,
                               (inst*)orig.inst_list,
                               (inst*)p->inst_list);
  cout << "Error cost: " << err_cost << endl;
  double per_cost = perf_cost(ex_set, num_ex,
                              (inst*)orig.inst_list,
                              (inst*)p->inst_list);
  cout << "Perf cost: " << per_cost << endl;
  return (w_e * err_cost) + (w_p * per_cost);
}

/* compute acceptance function */
double alpha(prog* curr, prog* next, const prog &orig, inout* ex_set,
             int num_ex, double w_e, double w_p) {
  double curr_cost = total_prog_cost(curr, orig, ex_set, num_ex, w_e, w_p);
  double next_cost = total_prog_cost(next, orig, ex_set, num_ex, w_e, w_p);
  cout << "Costs: curr " << curr_cost << " next " << next_cost << endl;
  return min(1.0, cost_to_pi(next_cost) / cost_to_pi(curr_cost));
}

prog* mh_next(prog* curr, const prog &orig, inout* ex_set, int num_ex,
              double w_e, double w_p) {
  prog* next = next_proposal(curr);
  double uni_sample = unidist_mh(gen_mh);
  if (uni_sample < alpha(curr, next, orig, ex_set, num_ex, w_e, w_p)) {
    return next;
  } else {
    prog::clear_prog(next);
    return curr;
  }
}

void mcmc_iter(int niter, const prog &orig,
               std::unordered_map<int, vector<prog*> > &prog_freq,
               inout* ex_set, int num_ex, double w_e=1.0, double w_p=1.0) {
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
    next = mh_next(curr, orig, ex_set, num_ex, w_e, w_p);
    /* Insert the next program into frequency dictionary if new */
    int ph = progHash()(*next);
    bool found = false;
    if (prog_freq.find(ph) == prog_freq.end()) {
      prog_freq[ph] = std::vector<prog*>();
    } else {
      vector<prog*> chain = prog_freq[ph];
      for (auto p: chain) {
        if (*p == *next) {
          found = true;
          p->freq_count++;
          break;
        }
      }
    }
    if (! found) {
      prog* next_copy = prog::make_prog(*next);
      next_copy->freq_count++;
      prog_freq[ph].push_back(next_copy);
    }
    if (curr != next) prog::clear_prog(curr);
    curr = next;
  }
}
