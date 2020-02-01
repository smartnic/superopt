#include <iostream>
#include <unordered_map>
#include <random>
#include <algorithm>
#include <unordered_set>
#include "../../src/utils.h"
#include "mh_prog.h"

using namespace std;

#define N 7
toy_isa_inst instructions[N] = {toy_isa_inst(toy_isa::MOVXC, 2, 4),  /* mov r2, 4  */
                                toy_isa_inst(toy_isa::ADDXY, 0, 2),  /* add r0, r2 */
                                toy_isa_inst(toy_isa::MOVXC, 3, 15),  /* mov r3, 15  */
                                toy_isa_inst(toy_isa::JMPGT, 0, 3, 1),  /* if r0 <= r3: */
                                toy_isa_inst(toy_isa::RETX, 3),      /* ret r3 */
                                toy_isa_inst(toy_isa::RETX, 0),      /* else ret r0 */
                                toy_isa_inst(),  /* control never reaches here */
                               };

vector<reg_t> inputs;

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
  double w_e = 1.0;
  double w_p = 0.0;
  if (argc > 1) {
    nrolls = atoi(argv[1]);
    if (argc > 3) {
      w_e = atof(argv[2]);
      w_p = atof(argv[3]);
    }
  }
  inputs.resize(30);
  gen_random_input(inputs, 0, 50);
  test1(nrolls, w_e, w_p);
  return 0;
}
