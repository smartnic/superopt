#include <iostream>
#include <unordered_map>
#include <random>
#include <algorithm>
#include <unordered_set>
#include "prog.h"
#include "inout.h"
#include "mh_prog.h"

using namespace std;

default_random_engine gen_mh_test;
uniform_real_distribution<double> unidist_mh_test(0.0, 1.0);

#define N 7
inst instructions[N] = {inst(MOVXC, 2, 4),  /* mov r2, 4  */
                        inst(ADDXY, 0, 2),  /* add r0, r2 */
                        inst(MOVXC, 3, 15),  /* mov r3, 15  */
                        inst(JMPGT, 0, 3, 1),  /* if r0 <= r3: */
                        inst(RETX, 3),      /* ret r3 */
                        inst(RETX, 0),      /* else ret r0 */
                        NOP,  /* control never reaches here */
                       };

vector<int> inputs;

void gen_random_input(vector<int>& inputs, int min, int max) {
  unordered_set<int> input_set;
  for (size_t i = 0; i < inputs.size();) {
    int input = min + (max - min) * unidist_mh_test(gen_mh_test);
    if (input_set.find(input) == input_set.end()) {
      input_set.insert(input);
      inputs[i] = input;
      i++;
    }
  }
}

void test1(int nrolls, double w_e, double w_p)  {
  mh_sampler mh;
  std::unordered_map<int, vector<prog*> > prog_freq;
  vector<prog*> progs(nrolls);
  prog orig(instructions);
  mh._cost.init(&orig, N, inputs, w_e, w_p);
  mh.mcmc_iter(nrolls, orig, prog_freq, progs);
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
  cout << "Total cost: " << mh._cost.total_prog_cost(best, 7) << endl;
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
