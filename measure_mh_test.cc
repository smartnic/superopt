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

string file_config_store = "config";
string file_cost_sample = "cost_sample";
string file_top_iternum = "top_iternum";
string file_raw_data_1 = "raw_data_progs";
string file_raw_data_2 = "raw_data_prog_dic";
string file_raw_data_ex = "raw_data_ex";
string file_raw_data_prog = "raw_data_prog";
string file_raw_data_error_perf = "raw_data_error_perf";
string file_raw_data_error_ex = "raw_data_error_ex";
int file_cost_sample_start = 0;
int file_top_iternum_k = 1;
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
                     int nrolls, double w_e, double w_p) {
  progs.resize(nrolls);
  mh_sampler mh;
  mh.init(file_raw_data_ex, file_raw_data_prog,
          file_raw_data_error_perf, file_raw_data_error_ex);
  prog orig(orig_inst);
  // cout << " running_sampler 1...." << endl;
  mh._cost.init(&orig, len, inputs, w_e, w_p);
  // cout << " running_sampler 2...." << endl;
  mh.mcmc_iter(nrolls, orig, prog_dic, progs);
  mh.close();
}

void relationship_cost_and_num_samples(double w_e, double w_p) {
  map<double, int> map_cost_sample; // key: cost, value: number
  for (size_t i = file_cost_sample_start; i < progs.size(); i++) {
    double cost = w_e * (double)progs[i]->_error_cost + w_p * (double)progs[i]->_perf_cost;
    auto it = map_cost_sample.find(cost);
    if (it == map_cost_sample.end()) map_cost_sample.insert(make_pair(cost, 1));
    else it->second++;
  }
  // redirect to file
  ofstream fout;
  fout.open(file_cost_sample, ios::out | ios::trunc);
  for (auto element : map_cost_sample) {
    fout << element.first << "," << element.second << " ";
    // cout << element.first << "\t" << element.second << "\n";
  }
  fout.close();
}

void relationship_top_k_progs_and_iter_num(int nrolls, int orig_id, int k) {
  vector<int> top_iternum(nrolls, 0);
  set<double> perf_sort; //perf cost with 0 error cost
  for (std::pair<int, vector <prog*> > element : prog_dic) {
    vector<prog*> pl = element.second; // list of progs with the same hash
    for (auto p : pl) {
      if (p->_error_cost == 0) {
        perf_sort.insert(p->_perf_cost);
      }
    }
  }
  double threshold = 100000;
  int i = 0;
  cout << "perf cost: ";
  for (auto it = perf_sort.begin(); it != perf_sort.end(); it++, i++) {
    if (k == (i + 1)) {
      threshold = *it;
      break;
    }
  }
  cout << "threshold: " << threshold << endl;
  top_iternum[0] = (progs[0]->_error_cost == 0) &&
                   (progs[0]->_perf_cost <= threshold);
  for (size_t i = 1; i < progs.size(); i++) {
    if (progs[i]->_error_cost == 0) {
      if (progs[i]->_perf_cost <= threshold)
        top_iternum[i] = top_iternum[i - 1] + 1;
      else
        top_iternum[i] = top_iternum[i - 1];
    }
  }
  ofstream fout;
  string file_name = file_top_iternum;
  if (k == 1) fout.open(file_name, ios::out | ios::trunc);
  else fout.open(file_name, ios::out | ios::app);
  for (size_t i = 0; i < top_iternum.size(); i++) {
    fout << top_iternum[i] << " ";
  }
  fout << endl;
  fout.close();

  // print top1 programs
  if (k != 1) return;
  cout << "best performance cost: " +
       to_string((double)origs_best_perf_cost[orig_id] / (double)PERF_COST_NORMAL) << endl;
  cout << "the best performance cost has found is " + to_string(threshold) << endl;
  cout << "these programs are:\n";
  for (std::pair<int, vector <prog*> > element : prog_dic) {
    vector<prog*> pl = element.second; // list of progs with the same hash
    for (auto p : pl) {
      if ((p->_error_cost == 0) && (p->_perf_cost <= threshold)) {
        p->print();
      }
    }
  }
}

void relationship_top_progs_and_iter_num(int nrolls, int orig_id) {
  for (int k = 1; k <= file_top_iternum_k; k++) {
    relationship_top_k_progs_and_iter_num(nrolls, orig_id, k);
  }
}

void store_config(double w_e, double w_p, int orig_id, int nrolls) {
  ofstream fout;
  fout.open(file_config_store, ios::out | ios::trunc);
  fout << "orig_id " << orig_id << endl;
  fout << "nrolls " << nrolls << endl;
  fout << "w_e " << w_e << endl;
  fout << "w_p " << w_p << endl;
  fout << "file_cost_sample " << file_cost_sample << endl;
  fout << "file_top_iternum " << file_top_iternum << endl;
  fout << "file_top_iternum_k " << file_top_iternum_k << endl;
  fout.close();
}

void file_rename(string path, double w_e, double w_p, int orig_id) {
  file_config_store = path + file_config_store;
  file_cost_sample = path + file_cost_sample;
  file_top_iternum = path + file_top_iternum;
  file_raw_data_1 = path + file_raw_data_1;
  file_raw_data_2 = path + file_raw_data_2;
  file_raw_data_ex = path + file_raw_data_ex;
  file_raw_data_prog = path + file_raw_data_prog;
  file_raw_data_error_perf = path + file_raw_data_error_perf;
  file_raw_data_error_ex = path + file_raw_data_error_ex;
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
  file_config_store += suffix;
  file_cost_sample += suffix;
  file_top_iternum += suffix;
  file_raw_data_1 += suffix;
  file_raw_data_2 += suffix;
  file_raw_data_ex += suffix;
  file_raw_data_prog += suffix;
  file_raw_data_error_perf += suffix;
  file_raw_data_error_ex += suffix;
}

void store_raw_data(double w_e, double w_p, int orig_id) {
  ofstream fout;
  fout.open(file_raw_data_1, ios::out | ios::trunc);
  for (size_t i = 0; i < progs.size(); i++) {
    fout << progs[i]->_error_cost << " " << progs[i]->_perf_cost << " " <<
         w_e * (double)progs[i]->_error_cost + w_p * (double)progs[i]->_perf_cost << endl;
  }
  fout.close();
  fout.open(file_raw_data_2, ios::out | ios::trunc);
  for (std::pair<int, vector <prog*> > element : prog_dic) {
    vector<prog*> pl = element.second;
    for (auto p : pl) {
      fout << p->_error_cost << " " << p->_perf_cost << " " << p->freq_count << endl;
    }
  }
  fout.close();

  // file_raw_data_ex += suffix;
  // file_raw_data_prog += suffix;
  // file_raw_data_error_perf += suffix;
  // file_raw_data_error_ex += suffix;
}

int main(int argc, char* argv[]) {
  int nrolls = 10;
  double w_e = 1.0;
  double w_p = 0.0;
  int orig_id = 0;
  string path = "";
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
    file_cost_sample_start = atoi(argv[6]);
  }
  if (argc > 7) {
    file_top_iternum_k = atoi(argv[7]);
  }
  cout << "start measuring" << endl;
  file_rename(path, w_e, w_p, orig_id);
  store_config(w_e, w_p, orig_id, nrolls);
  init_origs();
  inputs.resize(30);
  gen_random_input(inputs, -50, 50);
  running_sampler(origs[orig_id], origs_len, nrolls, w_e, w_p);
  store_raw_data(w_e, w_p, orig_id);
  relationship_cost_and_num_samples(w_e, w_p);
  relationship_top_progs_and_iter_num(nrolls, orig_id);
  cout << "last sample is\n";
  progs[progs.size() - 1]->print();
  return 0;
}
