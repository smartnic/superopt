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

string file_raw_data_programs = "raw_data_programs";
string file_raw_data_proposals = "raw_data_proposals";
string file_raw_data_examples = "raw_data_examples";
string file_raw_data_optimals = "raw_data_optimals";
vector<inst*> bms;
int bms_len = MAX_PROG_LEN;
vector<prog> bm_optimals;
vector<int> bms_best_perf_cost;
vector<int> inputs;
std::unordered_map<int, vector<prog*> > prog_dic;

void init_benchmarks(vector<vector<inst*> > &bm_optis_orig) {
  bms.push_back(bm0);
  bms.push_back(bm1);
  bms.push_back(bm2);
  for (int i = 0; i < NUM_ORIG; i++)
    bm_optis_orig.push_back(vector<inst*> {});
  bm_optis_orig[0].push_back(bm_opti00);
  bm_optis_orig[0].push_back(bm_opti01);
  bm_optis_orig[0].push_back(bm_opti02);
  bm_optis_orig[0].push_back(bm_opti03);
  bm_optis_orig[0].push_back(bm_opti04);
  bm_optis_orig[0].push_back(bm_opti05);
  bm_optis_orig[1].push_back(bm_opti10);
  bm_optis_orig[1].push_back(bm_opti11);
  bm_optis_orig[1].push_back(bm_opti12);
  bm_optis_orig[2].push_back(bm_opti20);
  bm_optis_orig[2].push_back(bm_opti21);
  bm_optis_orig[2].push_back(bm_opti22);
  bm_optis_orig[2].push_back(bm_opti23);
  bm_optis_orig[2].push_back(bm_opti24);
  bm_optis_orig[2].push_back(bm_opti25);
  bm_optis_orig[2].push_back(bm_opti26);
  bm_optis_orig[2].push_back(bm_opti27);
  bm_optis_orig[2].push_back(bm_opti28);
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
void gen_optis_for_prog(const prog& p, const int& len,
                        vector<prog>& opti_set) {
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

void gen_optis_for_progs(vector<inst*> &bm_optis_orig) {
  for (size_t i = 0; i < bm_optis_orig.size(); i++) {
    prog bm_opti(bm_optis_orig[i]);
    // op_set: temporarily store optimals for one bm optimal program
    vector<prog> op_set;
    gen_optis_for_prog(bm_opti, MAX_PROG_LEN, op_set);
    for (size_t j = 0; j < op_set.size(); j++)
      bm_optimals.push_back(op_set[j]);
  }
}

void store_raw_data(meas_mh_data &d) {
  store_proposals_to_file(file_raw_data_proposals, d, bm_optimals);
  store_programs_to_file(file_raw_data_programs, d, bm_optimals);
  store_examples_to_file(file_raw_data_examples, d);
  store_optimals_to_file(file_raw_data_optimals, bm_optimals);
}

void run_mh_sampler_and_store_data(int bm_id, int len,
                                   int nrolls, double w_e, double w_p,
                                   int strategy_ex, int strategy_eq,
                                   int strategy_avg) {
  mh_sampler mh;
  mh.turn_on_measure();
  prog orig(bms[bm_id]);
  mh._cost.init(&orig, len, inputs, w_e, w_p,
                strategy_ex, strategy_eq, strategy_avg);
  mh.mcmc_iter(nrolls, orig, prog_dic);
  mh.turn_off_measure();
  store_raw_data(mh._meas_data);
}

// eg. "1.1100" -> "1.11"; "1.000" -> "1"
string rm_useless_zero_digits_from_str(string s) {
  // rm useless zero digits
  s.erase(s.find_last_not_of('0') + 1, string::npos);
  // rm useless `.`
  s.erase(s.find_last_not_of('.') + 1, string::npos);
  return s;
}

void gen_file_name_from_input(string path, int bm_id,
                              double w_e, double w_p,
                              int strategy_ex, int strategy_eq,
                              int strategy_avg) {
  file_raw_data_programs = path + file_raw_data_programs;
  file_raw_data_proposals = path + file_raw_data_proposals;
  file_raw_data_examples = path + file_raw_data_examples;
  file_raw_data_optimals = path + file_raw_data_optimals;
  string str_w_e = rm_useless_zero_digits_from_str(to_string(w_e));
  string str_w_p = rm_useless_zero_digits_from_str(to_string(w_p));
  string suffix = "_" + to_string(bm_id) +
                  "_" + to_string(strategy_ex) +
                  to_string(strategy_eq) +
                  to_string(strategy_avg) +
                  "_" + str_w_e +
                  "_" + str_w_p +
                  ".txt";
  file_raw_data_programs += suffix;
  file_raw_data_proposals += suffix;
  file_raw_data_examples += suffix;
  file_raw_data_optimals += "_" + to_string(bm_id) + ".txt";
}

int main(int argc, char* argv[]) {
  int nrolls = 10;
  double w_e = 1.0;
  double w_p = 0.0;
  int bm_id = 0;
  string path = "measure/";
  int strategy_ex = ERROR_COST_STRATEGY_ABS;
  int strategy_eq = ERROR_COST_STRATEGY_EQ1;
  int strategy_avg = ERROR_COST_STRATEGY_NAVG;
  if (argc > 1)
    bm_id = atoi(argv[1]);
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
  gen_file_name_from_input(path, bm_id, w_e, w_p,
                           strategy_ex, strategy_eq, strategy_avg);
  vector<vector<inst*> > bm_optis_orig;
  init_benchmarks(bm_optis_orig);
  // get all optimal programs from the original ones
  gen_optis_for_progs(bm_optis_orig[bm_id]);
  inputs.resize(30);
  gen_random_input(inputs, -50, 50);
  run_mh_sampler_and_store_data(bm_id, bms_len, nrolls, w_e, w_p,
                                strategy_ex, strategy_eq, strategy_avg);
  return 0;
}
