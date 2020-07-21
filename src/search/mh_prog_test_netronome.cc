#include <iostream>
#include <unordered_map>
#include <random>
#include <algorithm>
#include <unordered_set>
#include "../../src/utils.h"
#include "mh_prog.h"

using namespace std;

// a complicated program that returns the number 21
inst instructions[] = {
    inst(IMMED, 5, 4),
    inst(IMMED, 16, 5),
    inst(IMMED, 6, 2),
    inst(ALU, 4, 5, ALU_PLUS, 16),
    inst(ALU, 3, 4, ALU_MINUS, 6),
    inst(IMMED, 0, 21),
    inst(NOP),
};

int N = sizeof(instructions) / sizeof(inst);

vector<inout_t> inputs;

void mh_sampler_res_print(int nrolls,
                          unordered_map<int, vector<prog*> > prog_freq)  {
  // Get the best program(s)
  int max = 0;
  int concurrent_max = 0;
  prog *best;
  int nprogs = 0;
  for (std::pair<int, vector <prog*> > element : prog_freq) {
    vector<prog*> pl = element.second; // list of progs with the same hash
    for (auto p : pl) {
      nprogs++;
      if (p->freq_count > max) {
        concurrent_max = 1;
        best = p;
        max = p->freq_count;
      } else if (p->freq_count == max) {
        concurrent_max++;
      }
    }
  }
  cout << "number of unique hashes observed: " << prog_freq.size() << endl;
  cout << "number of unique programs observed: " << nprogs << endl;
  cout << "Number of concurrently best programs:" << concurrent_max << endl;
  cout << "One of the best programs: " << endl;
  cout << "Observed frequency " << max << " out of " << nrolls << endl;
  best->print();
}

void test1(int nrolls, double w_e, double w_p)  {
  mh_sampler mh;
  mh._restart.set_st_when_to_restart(MH_SAMPLER_ST_WHEN_TO_RESTART_MAX_ITER, 5);
  mh._restart.set_st_next_start_prog(MH_SAMPLER_ST_NEXT_START_PROG_K_CONT_INSTS);
  mh._restart.set_we_wp_list(vector<double> {0.5, 1.5}, vector<double> {1.5, 0.5});
  mh._next_proposal.set_probability(0.3, 0.5);
  std::unordered_map<int, vector<prog*> > prog_freq;
  prog orig(instructions);
  mh._cost.init(&orig, N, inputs, w_e, w_p);
  mh.mcmc_iter(nrolls, orig, prog_freq);
  mh_sampler_res_print(nrolls, prog_freq);
}

int main(int argc, char* argv[]) {
  int nrolls = 10;
  double w_e = 1.0; // weight of error cost
  double w_p = 0.0; // weight of performance cost
  if (argc > 1) {
    nrolls = atoi(argv[1]);
    if (argc > 3) {
      w_e = atof(argv[2]);
      w_p = atof(argv[3]);
    }
  }
  vector<reg_t> inputs_reg(30);
  gen_random_input(inputs_reg, 0, 50);
  inputs.resize(30);
  for (int i = 0; i < inputs.size(); i++) {
    inputs[i].init();
    inputs[i].reg = inputs_reg[i];
  }
  test1(nrolls, w_e, w_p);
  return 0;
}