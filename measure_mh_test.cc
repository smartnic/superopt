#include <iostream>
#include <fstream>
#include <unordered_map>
#include <random>
#include <algorithm>
#include <unordered_set>
#include <map>
#include <set>
#include <utility>
#include "prog.h"
#include "inout.h"
#include "mh_prog.h"

using namespace std;

string file_cost_sample;
string file_best_iternum;
int file_best_iternum_topk;

default_random_engine gen_mh_test;
uniform_real_distribution<double> unidist_mh_test(0.0, 1.0);

#define N 7
// output = max(input+4, output)
inst instructions[N] = {inst(MOVXC, 2, 4),  /* mov r2, 4  */
                        inst(ADDXY, 0, 2),  /* add r0, r2 */
                        inst(MOVXC, 3, 15),  /* mov r3, 15  */
                        inst(JMPGT, 0, 3, 1),  /* if r0 <= r3: */
                        inst(RETX, 3),      /* ret r3 */
                        inst(RETX, 0),      /* else ret r0 */
                        NOP,  /* control never reaches here */
                       };
#define N1 11
// f(x) = max(x, r1, r2, 10) = max(x, 10)
inst instructions1[N1] = {inst(JMPGT, 0, 1, 2), // skip r0 <- r1, if r0 > r1
                          inst(MOVXC, 0, 0),
                          inst(ADDXY, 0, 1),
                          inst(JMPGT, 0, 2, 2), // skip r0 <- r2, if r0 > r2
                          inst(MOVXC, 0, 0),
                          inst(ADDXY, 0, 2),
                          inst(MOVXC, 1, 10),   // r1 <- 10
                          inst(JMPGT, 0, 1, 2), // skip r0 <- r1, if r0 > r1
                          inst(MOVXC, 0, 0),
                          inst(ADDXY, 0, 1),
                          inst(RETX, 0),        // ret r0
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

void test1(inst* orig_inst, int len_orig,
           vector<int>& inputs,
           double w_e, double w_p,
           int nrolls)  {
  mh_sampler mh;
  std::unordered_map<int, vector<prog*> > prog_dic;
  vector<prog*> progs(nrolls);
  prog orig(orig_inst);
  mh._cost.init(&orig, N, inputs, w_e, w_p);
  mh.mcmc_iter(nrolls, orig, prog_dic, progs);

  // the relationship between number of sample(program) and cost value
  // num_sample = f(cost)
  map<double, int> map_cost_sample; // key: cost, value: number
  for (std::pair<int, vector <prog*> > element : prog_dic) {
    vector<prog*> pl = element.second; // list of progs with the same hash
    for (auto p : pl) {
      double cost = w_e * (double)p->_error_cost + w_p * (double)p->_perf_cost;
      auto it = map_cost_sample.find(cost);
      if (it == map_cost_sample.end()) map_cost_sample.insert(make_pair(cost, 1));
      else it->second++;
    }
  }
  // redirect to file
  ofstream fout;
  fout.open(file_cost_sample, ios::out | ios::trunc);
  for (auto element : map_cost_sample) {
    fout << element.first << "," << element.second << " ";
    // cout << element.first << "\t" << element.second << "\n";
  }
  fout.close();

  // Get the best program: 0 error cost with smallest perf cost
  vector<int> best_iternum(nrolls, 0);
  set<int> perf_sort; //perf cost with 0 error cost
  for (std::pair<int, vector <prog*> > element : prog_dic) {
    vector<prog*> pl = element.second; // list of progs with the same hash
    for (auto p : pl) {
      if (! p->_error_cost)
        perf_sort.insert(p->_perf_cost);
    }
  }
  cout << "perf_sort is\n";
  for (auto it = perf_sort.begin(); it != perf_sort.end(); it++) {
    cout << *it << " ";
  }
  cout << endl;
  int threshold = 0;
  int k = file_best_iternum_topk; // top k
  int i = 0;
  for (auto it = perf_sort.begin(); it != perf_sort.end(); it++, i++) {
    if (k == (i + 1)) {
      threshold = *it;
      break;
    }
  }
  // cout << "threshold: " << threshold << endl;
  best_iternum[0] = (! progs[0]->_error_cost) &&
                    (progs[0]->_perf_cost <= threshold);
  for (size_t i = 1; i < progs.size(); i++) {
    if (! progs[i]->_error_cost) {
      // cout << i << ":" << progs[i]->_perf_cost <<endl;
      if (progs[i]->_perf_cost <= threshold)
        best_iternum[i] = best_iternum[i - 1] + 1;
      else
        best_iternum[i] = best_iternum[i - 1];
    }
  }
  // cout << endl;
  fout.open(file_best_iternum, ios::out | ios::trunc);
  for (size_t i = 0; i < best_iternum.size(); i++) {
    fout << best_iternum[i] << " ";
  }
  fout << endl;
  fout.close();
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
      if (argc > 6) {
        file_cost_sample = argv[4];
        file_best_iternum = argv[5];
        file_best_iternum_topk = atoi(argv[6]);
      }
    }
  }
  inputs.resize(30);
  gen_random_input(inputs, 0, 50);
  // test1(instructions, N, inputs, w_e, w_p, nrolls);
  test1(instructions1, N1, inputs, w_e, w_p, nrolls);
  return 0;
}
