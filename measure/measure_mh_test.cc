#include <iostream>
#include <fstream>
#include <unordered_map>
#include <algorithm>
#include <unordered_set>
#include <map>
#include <set>
#include <utility>
#include "../prog.h"
#include "../inout.h"
#include "../mh_prog.h"
#include "../inst.h"
#include "../utils.h"
#include "common.h"
#include "meas_mh_data.h"

using namespace std;

string file_config_store = "config";
string file_raw_data_prog = "raw_data_prog";
string file_raw_data_prog_dic = "raw_data_prog_dic";
string file_raw_data_proposal = "raw_data_proposal";
string file_raw_data_proposal_prog = "raw_data_proposal_prog";
string file_raw_data_ex = "raw_data_ex";
string file_raw_data_optimal = "raw_data_optimal";
vector<inst*> origs;
vector<vector<inst*> > optis;
int origs_len = MAX_PROG_LEN;
vector<int> origs_best_perf_cost;
vector<int> inputs;
std::unordered_map<int, vector<prog*> > prog_dic;

void init_origs() {
  for (int i = 0; i < 4; i++)
    optis.push_back(vector<inst*> {});
  origs.push_back(orig0);
  optis[0].push_back(opti00);
  optis[0].push_back(opti01);
  optis[0].push_back(opti02);
  optis[0].push_back(opti03);
  optis[0].push_back(opti04);
  optis[0].push_back(opti05);
  origs_best_perf_cost.push_back(4);
  // optis.push_back(opti1);
  origs_best_perf_cost.push_back(2);
  origs.push_back(orig2);
  optis[2].push_back(opti20);
  optis[2].push_back(opti21);
  optis[2].push_back(opti22);
  origs_best_perf_cost.push_back(4);
  origs.push_back(orig3);
  optis[3].push_back(opti30);
  optis[3].push_back(opti31);
  optis[3].push_back(opti32);
  optis[3].push_back(opti33);
  optis[3].push_back(opti34);
  optis[3].push_back(opti35);
  optis[3].push_back(opti36);
  optis[3].push_back(opti37);
  optis[3].push_back(opti38);
  origs_best_perf_cost.push_back(4);
}

// return C_n^m
unsigned int combination(unsigned int n, unsigned m) {
  unsigned int a = 1;
  for (unsigned int i = n; i > (n - m); i--) {
    a *= i;
  }
  unsigned int b = 1;
  for (unsigned int i = 1; i <= m; i++) {
    b *= i;
  }
  return (a / b);
}

// generate all combinations that picks n unrepeated numbers from s to e
// row_s is the staring row in `res` that store the combinations
// e.g. s=0, e=3, n=2, row_s=0, res=[[1,2], [1,3], [2,3]]
void gen_n_numbers(int n, int s, int e,
                   int row_s, vector<vector<int> >& res) {
  if (n == 0) return;
  for (int i = s; i <= e - n + 1; i++) {
    int num_comb = combination(e - i, n - 1);
    for (int j = row_s; j < row_s + num_comb; j++)
      res[j].push_back(i);
    gen_n_numbers(n - 1, i + 1, e, row_s, res);
    row_s += num_comb;
  }
}

// should ensure the first real_length instructions in program p are not NOP,
// the remainings are NOP.
void gen_opti_set(const prog& p, const int& len, vector<prog>& opti_set) {
  int n = num_real_instructions((inst*)p.inst_list, len);
  // C_len^n
  int num_opti = combination(len, n);
  vector<vector<int> > comb_set(num_opti);
  gen_n_numbers(n, 0, len - 1, 0, comb_set);
  opti_set.resize(num_opti);
  for (size_t i = 0; i < comb_set.size(); i++) {
    for (size_t j = 0; j < len; j++)
      opti_set[i].inst_list[j] = inst(NOP);
    for (size_t j = 0; j < comb_set[i].size(); j++) {
      size_t pos = comb_set[i][j];
      opti_set[i].inst_list[pos] = p.inst_list[j];
    }
  }
}

void gen_opti_progs(vector<prog> &opti_set, int orig_id) {
  for (size_t i = 0; i < optis[orig_id].size(); i++) {
    vector<prog> op_set;
    prog op(optis[orig_id][i]);
    gen_opti_set(op, MAX_PROG_LEN, op_set);
    for (size_t j = 0; j < op_set.size(); j++)
      opti_set.push_back(op_set[j]);
  }
}

void store_raw_data(meas_mh_data &d, vector<prog> &optimals) {
  store_proposals_to_file(file_raw_data_proposal, d, optimals);
  store_programs_to_file(file_raw_data_prog, d, optimals);
  store_examples_to_file(file_raw_data_ex, d);
  store_optimals_to_file(file_raw_data_optimal, optimals);
}

void running_sampler(int orig_id, int len,
                     int nrolls, double w_e, double w_p,
                     int strategy_ex, int strategy_eq, int strategy_avg,
                     vector<prog> &opti_set) {
  mh_sampler mh;
  mh.turn_on_measure();
  prog orig(origs[orig_id]);
  mh._cost.init(&orig, len, inputs, w_e, w_p,
                strategy_ex, strategy_eq, strategy_avg);
  mh.mcmc_iter(nrolls, orig, prog_dic);
  mh.turn_off_measure();
  store_raw_data(mh._meas_data, opti_set);
}

void file_rename(string path, double w_e, double w_p, int orig_id) {
  file_raw_data_prog = path + file_raw_data_prog;
  file_raw_data_prog_dic = path + file_raw_data_prog_dic;
  file_raw_data_proposal_prog = path + file_raw_data_proposal_prog;
  file_raw_data_proposal = path + file_raw_data_proposal;
  file_raw_data_ex = path + file_raw_data_ex;
  file_raw_data_optimal = path + file_raw_data_optimal;
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
  file_raw_data_optimal += "_" + to_string(orig_id) + ".txt";
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
  // cout << "start measuring" << endl;
  file_rename(path, w_e, w_p, orig_id);
  init_origs();
  inputs.resize(30);
  gen_random_input(inputs, -50, 50);
  vector<prog> opti_set;
  gen_opti_progs(opti_set, orig_id);
  running_sampler(orig_id, origs_len, nrolls, w_e, w_p,
                  strategy_ex, strategy_eq, strategy_avg,
                  opti_set);
  return 0;
}
