#include <iostream>
#include <fstream>
#include <unordered_map>
#include <algorithm>
#include <unordered_set>
#include <map>
#include <set>
#include <utility>
#include <getopt.h>
#include "common.h"
#include "meas_mh_data.h"
#include "../prog.h"
#include "../inout.h"
#include "../mh_prog.h"
#include "../inst.h"
#include "../utils.h"

using namespace std;

string file_raw_data_programs = "raw_data_programs";
string file_raw_data_proposals = "raw_data_proposals";
string file_raw_data_examples = "raw_data_examples";
string file_raw_data_optimals = "raw_data_optimals";
vector<inst*> bms;
int bm_len = MAX_PROG_LEN;
vector<prog> bm_optimals;
vector<int> bms_best_perf_cost;
vector<int> inputs;
std::unordered_map<int, vector<prog*> > prog_dic;

struct input_paras {
  int nrolls;
  double w_e;
  double w_p;
  int bm_id;
  string path;
  int st_ex;
  int st_eq;
  int st_avg;
};

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

// Generate all combinations that picks n unrepeated numbers from s to e
// row_s is the staring row in `res` that store the combinations
// e.g. s=1, e=3, n=2, row_s=0, res=[[1,2], [1,3], [2,3]]
// steps: compute it recursively ranging from large to small,
// while the real computation is from small to large,
// that is, compute combinations in range [s:e] first, then [s-1:e]
void gen_n_combinations(int n, int s, int e,
                        int row_s, vector<vector<int> >& res) {
  if (n == 0) return;
  for (int i = s; i <= e - n + 1; i++) {
    int num_comb = combination(e - i, n - 1);
    for (int j = row_s; j < row_s + num_comb; j++)
      res[j].push_back(i);
    gen_n_combinations(n - 1, i + 1, e, row_s, res);
    row_s += num_comb;
  }
}

// Premise: should ensure the first real_length instructions in program p are not NOP,
// the remainings are NOP.
// steps: 1. Set all instructions of this optimal program as NOP;
// 2. Compute combinations for real instruction positions;
// 3. replace NOP instructions with real instructions according to combinations.
// e.g. if optimal program has 5 NOPs, one combination is [2,3],
// then the second and third instructions are replaced with real instructions
void gen_optis_for_prog(const prog& p, const int& len,
                        vector<prog>& opti_set) {
  int n = num_real_instructions((inst*)p.inst_list, len);
  // C_len^n
  int num_opti = combination(len, n);
  vector<vector<int> > comb_set(num_opti);
  gen_n_combinations(n, 0, len - 1, 0, comb_set);
  opti_set.resize(num_opti);
  for (size_t i = 0; i < comb_set.size(); i++) {
    // set all instructions of this optimal program as NOP
    for (size_t j = 0; j < len; j++)
      opti_set[i].inst_list[j] = inst(NOP);
    // replace some NOP instructions with real instructions
    // according to the combination value
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
  store_optimals_to_file(file_raw_data_optimals, bm_optimals, d._mode);
}

void run_mh_sampler_and_store_data(const input_paras &in_para) {
  mh_sampler mh;
  mh.turn_on_measure();
  prog orig(bms[in_para.bm_id]);
  mh._cost.init(&orig, bm_len, inputs,
                in_para.w_e, in_para.w_p,
                in_para.st_ex, in_para.st_eq,
                in_para.st_avg);
  mh.mcmc_iter(in_para.nrolls, orig, prog_dic);
  store_raw_data(mh._meas_data);
  mh.turn_off_measure();
}

// eg. "1.1100" -> "1.11"; "1.000" -> "1"
string rm_useless_zero_digits_from_str(string s) {
  // rm useless zero digits
  s.erase(s.find_last_not_of('0') + 1, string::npos);
  // rm useless decimal point
  s.erase(s.find_last_not_of('.') + 1, string::npos);
  return s;
}

void gen_file_name_from_input(const input_paras &in_para) {
  file_raw_data_programs = in_para.path + file_raw_data_programs;
  file_raw_data_proposals = in_para.path + file_raw_data_proposals;
  file_raw_data_examples = in_para.path + file_raw_data_examples;
  file_raw_data_optimals = in_para.path + file_raw_data_optimals;
  string str_w_e = rm_useless_zero_digits_from_str(to_string(in_para.w_e));
  string str_w_p = rm_useless_zero_digits_from_str(to_string(in_para.w_p));
  string suffix = "_" + to_string(in_para.bm_id) +
                  "_" + to_string(in_para.nrolls) +
                  "_" + to_string(in_para.st_ex) +
                  to_string(in_para.st_eq) +
                  to_string(in_para.st_avg) +
                  "_" + str_w_e +
                  "_" + str_w_p +
                  ".txt";
  file_raw_data_programs += suffix;
  file_raw_data_proposals += suffix;
  file_raw_data_examples += suffix;
  file_raw_data_optimals += "_" + to_string(in_para.bm_id) + ".txt";
}

void parse_input(int argc, char* argv[], input_paras &in_para) {
  const char* const short_opts = "n:";
  static struct option long_opts[] = {
    {"path_out", required_argument, nullptr, 0},
    {"bm", required_argument, nullptr, 1},
    {"we", required_argument, nullptr, 2},
    {"wp", required_argument, nullptr, 3},
    {"st_ex", required_argument, nullptr, 4},
    {"st_eq", required_argument, nullptr, 5},
    {"st_avg", required_argument, nullptr, 6},
    {nullptr, no_argument, nullptr, 0}
  };
  int opt;
  while ((opt = getopt_long(argc, argv, short_opts,
                            long_opts, nullptr)) != -1) {
    switch (opt) {
      case 'n': in_para.nrolls = stoi(optarg); break;
      case 0: in_para.path = optarg; break;
      case 1: in_para.bm_id = stoi(optarg); break;
      case 2: in_para.w_e = stod(optarg); break;
      case 3: in_para.w_p = stod(optarg); break;
      case 4: in_para.st_ex = stoi(optarg); break;
      case 5: in_para.st_eq = stoi(optarg); break;
      case 6: in_para.st_avg = stoi(optarg); break;
    }
  }
}

void set_default_para_vals(input_paras &in_para) {
  in_para.nrolls = 10;
  in_para.w_e = 1.0;
  in_para.w_p = 0.0;
  in_para.bm_id = 0;
  in_para.path = "measure/";
  in_para.st_ex = ERROR_COST_STRATEGY_ABS;
  in_para.st_eq = ERROR_COST_STRATEGY_EQ1;
  in_para.st_avg = ERROR_COST_STRATEGY_NAVG;
}

int main(int argc, char* argv[]) {
  input_paras in_para;
  set_default_para_vals(in_para);
  parse_input(argc, argv, in_para);
  gen_file_name_from_input(in_para);
  vector<vector<inst*> > bm_optis_orig;
  init_benchmarks(bm_optis_orig);
  // get all optimal programs from the original ones
  gen_optis_for_progs(bm_optis_orig[in_para.bm_id]);
  inputs.resize(30);
  gen_random_input(inputs, -50, 50);
  run_mh_sampler_and_store_data(in_para);
  return 0;
}
