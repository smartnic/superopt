#include <iostream>
#include <fstream>
#include <unordered_map>
#include <random>
#include <algorithm>
#include <unordered_set>
#include <map>
#include <set>
#include <utility>
#include "../prog.h"
#include "../inout.h"
#include "../mh_prog.h"

using namespace std;

string file_config_store = "config";
string file_raw_data_prog = "raw_data_prog";
string file_raw_data_prog_dic = "raw_data_prog_dic";
string file_raw_data_proposal = "raw_data_proposal";
string file_raw_data_proposal_prog = "raw_data_proposal_prog";
string file_raw_data_ex = "raw_data_ex";
vector<inst*> origs;
int origs_len = MAX_PROG_LEN;
vector<int> origs_best_perf_cost;
vector<int> inputs;
std::unordered_map<int, vector<prog*> > prog_dic;
vector<prog*> progs;

default_random_engine gen_mh_test;
uniform_real_distribution<double> unidist_mh_test(0.0, 1.0);

// output = max(input+4, output)
// min_len: 3
// perf_cost = 3 + 1 = 4
inst orig0[MAX_PROG_LEN] = {inst(MOVXC, 2, 4),  /* mov r2, 4  */
                            inst(ADDXY, 0, 2),  /* add r0, r2 */
                            inst(MOVXC, 3, 15),  /* mov r3, 15  */
                            inst(JMPGT, 0, 3, 1),  /* if r0 <= r3: */
                            inst(RETX, 3),      /* ret r3 */
                            inst(RETX, 0),      /* else ret r0 */
                            NOP,  /* control never reaches here */
                           };
// f(x) = min(x, r1, 10) = min(x, 0)
// min_len: 2
// perf_cost = 2
inst orig1[MAX_PROG_LEN] = {inst(JMPLE, 0, 1, 2), // skip r0 <- r1, if r0 <= r1
                            inst(MOVXC, 0, 0),
                            inst(ADDXY, 0, 1),
                            inst(MOVXC, 1, 10),   // r1 <- 10
                            inst(JMPLE, 0, 1, 1), // if r0 <= r1, return r0, else r1
                            inst(RETX, 1),
                            inst(RETX, 0),
                           };
// f(x) = max(2*x, x+4)
// MOVC 1 4
// MAXX 1 0
// ADDXY 0 1
// perf_cost = 3+1=4
inst orig2[MAX_PROG_LEN] = {inst(ADDXY, 1, 0),
                            inst(MOVXC, 2, 4),
                            inst(ADDXY, 1, 2), // r1 = r0+4
                            inst(ADDXY, 0, 0), // r0 += r0
                            inst(MAXX, 1, 0),  // r1 = max(r1, r0)
                            inst(RETX, 1),
                            NOP,
                           };
// f(x) = 6*x
// ADDXY 0 0  r0 = 2*r0
// ADDXY 1 0  r1 = 2*r0
// ADDXY 0 1  r0 = 4*r0
// ADDXY 0 1  r0 = 6*r0
// perf_cost = 4+0 = 4
inst orig3[MAX_PROG_LEN] = {inst(MOVXC, 1, 0),
                            inst(ADDXY, 1, 0), // r1 = 2*r0
                            inst(ADDXY, 0, 1),
                            inst(ADDXY, 0, 1),
                            inst(ADDXY, 0, 1),
                            inst(ADDXY, 0, 1),
                            inst(ADDXY, 0, 1),
                           };

void init_origs() {
  origs.push_back(orig0);
  origs_best_perf_cost.push_back(4);
  origs.push_back(orig1);
  origs_best_perf_cost.push_back(2);
  origs.push_back(orig2);
  origs_best_perf_cost.push_back(4);
  origs.push_back(orig3);
  origs_best_perf_cost.push_back(4);
}

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

void running_sampler(inst* orig_inst, int len,
                     int nrolls, double w_e, double w_p,
                     int strategy_ex, int strategy_eq, int strategy_avg) {
  progs.resize(nrolls);
  mh_sampler mh;
  mh.open_measure_file(file_raw_data_proposal, file_raw_data_ex);
  prog orig(orig_inst);
  mh._cost.init(&orig, len, inputs, w_e, w_p,
                strategy_ex, strategy_eq, strategy_avg);
  mh.mcmc_iter(nrolls, orig, prog_dic, progs);
  mh.close_measure_file();
}

void file_rename(string path, double w_e, double w_p, int orig_id) {
  file_raw_data_prog = path + file_raw_data_prog;
  file_raw_data_prog_dic = path + file_raw_data_prog_dic;
  file_raw_data_proposal_prog = path + file_raw_data_proposal_prog;
  file_raw_data_proposal = path + file_raw_data_proposal;
  file_raw_data_ex = path + file_raw_data_ex;
  string str_w_e = to_string(w_e);
  string str_w_p = to_string(w_p);
  str_w_e.erase(str_w_e.find_last_not_of('0') + 1, string::npos);
  str_w_e.erase(str_w_e.find_last_not_of('.') + 1, string::npos);
  str_w_p.erase(str_w_p.find_last_not_of('0') + 1, string::npos);
  str_w_p.erase(str_w_p.find_last_not_of('.') + 1, string::npos);
  string suffix = "_" + to_string(orig_id) +
                  "_" + str_w_e +
                  "_" + str_w_p +
                  ".txt";
  file_raw_data_prog += suffix;
  file_raw_data_prog_dic += suffix;
  file_raw_data_proposal_prog += suffix;
  file_raw_data_proposal += suffix;
  file_raw_data_ex += suffix;
}

void store_raw_data(double w_e, double w_p, int orig_id) {
  ofstream fout;
  fout.open(file_raw_data_prog, ios::out | ios::trunc);
  for (size_t i = 0; i < progs.size(); i++) {
    fout << progs[i]->_error_cost << " "
         << progs[i]->_perf_cost << " "
         << w_e * (double)progs[i]->_error_cost + w_p * (double)progs[i]->_perf_cost
         << endl;
  }
  fout.close();
  fout.open(file_raw_data_prog_dic, ios::out | ios::trunc);
  for (std::pair<int, vector <prog*> > element : prog_dic) {
    vector<prog*> pl = element.second;
    for (auto p : pl) {
      fout << p->_error_cost << " "
           << p->_perf_cost << " "
           << p->freq_count << endl;
    }
  }
  fout.close();
}

int main(int argc, char* argv[]) {
  int nrolls = 10;
  double w_e = 1.0;
  double w_p = 0.0;
  int orig_id = 0;
  string path = "";
  int strategy_ex = 0;
  int strategy_eq = 0;
  int strategy_avg = 0;
  if (argc > 1)
    orig_id = atoi(argv[1]);
  if (argc > 2)
    nrolls = atoi(argv[2]);
  if (argc > 4) {
    w_e = atof(argv[3]);
    w_p = atof(argv[4]);
  }
  if (argc > 5) {
    path = argv[5];
  }
  if (argc > 6) {
    strategy_ex = atoi(argv[6]);
  }
  if (argc > 7) {
    strategy_eq = atoi(argv[7]);
  }
  if (argc > 8) {
    strategy_avg = atoi(argv[8]);
  }
  cout << "start measuring" << endl;
  file_rename(path, w_e, w_p, orig_id);
  init_origs();
  inputs.resize(30);
  gen_random_input(inputs, -50, 50);
  running_sampler(origs[orig_id], origs_len, nrolls, w_e, w_p,
                  strategy_ex, strategy_eq, strategy_avg);
  store_raw_data(w_e, w_p, orig_id);
  return 0;
}
