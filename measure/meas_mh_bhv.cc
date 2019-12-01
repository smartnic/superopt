#include <iostream>
#include <fstream>
#include <unordered_map>
#include <algorithm>
#include <unordered_set>
#include <map>
#include <set>
#include <utility>
#include <iomanip>
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
inst* bm;
int bm_len = MAX_PROG_LEN;
vector<prog> bm_optimals;
vector<int> bms_best_perf_cost;
vector<int> inputs;
std::unordered_map<int, vector<prog*> > prog_dic;

struct input_paras {
  int niter;
  double w_e;
  double w_p;
  int bm_id;
  string path;
  int st_ex;
  int st_eq;
  int st_avg;
  int st_when_to_restart;
  int st_when_to_restart_niter;
  int st_start_prog;
  double p_inst_operand;
  double p_inst;
};

void init_benchmarks(vector<inst*> &bm_optis_orig, int bm_id) {
  switch (bm_id) {
    case 0:
      bm = bm0;
      bm_optis_orig.push_back(bm_opti00);
      bm_optis_orig.push_back(bm_opti01);
      bm_optis_orig.push_back(bm_opti02);
      bm_optis_orig.push_back(bm_opti03);
      bm_optis_orig.push_back(bm_opti04);
      bm_optis_orig.push_back(bm_opti05);
      bm_optis_orig.push_back(bm_opti06);
      bm_optis_orig.push_back(bm_opti07);
      bm_optis_orig.push_back(bm_opti08);
      bm_optis_orig.push_back(bm_opti09);
      bm_optis_orig.push_back(bm_opti010);
      bm_optis_orig.push_back(bm_opti011);
      return;
    case 1:
      bm = bm1;
      bm_optis_orig.push_back(bm_opti10);
      bm_optis_orig.push_back(bm_opti11);
      bm_optis_orig.push_back(bm_opti12);
      bm_optis_orig.push_back(bm_opti13);
      bm_optis_orig.push_back(bm_opti14);
      bm_optis_orig.push_back(bm_opti15);
      return;
    case 2:
      bm = bm2;
      bm_optis_orig.push_back(bm_opti20);
      bm_optis_orig.push_back(bm_opti21);
      bm_optis_orig.push_back(bm_opti22);
      bm_optis_orig.push_back(bm_opti23);
      bm_optis_orig.push_back(bm_opti24);
      bm_optis_orig.push_back(bm_opti25);
      bm_optis_orig.push_back(bm_opti26);
      bm_optis_orig.push_back(bm_opti27);
      bm_optis_orig.push_back(bm_opti28);
      return;
    default:
      cout << "bm_id" + to_string(bm_id) + "is out of range {0, 1, 2}" << endl;
      return;
  }
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
// row_s is the starting row in `res` that stores the combinations
// e.g. s=1, e=3, n=2, row_s=0, res=[[1,2], [1,3], [2,3]]
// steps: compute combinations recursively ranging from large to small,
// while the real computation is from small to large,
// that is, compute combinations in range [s+1:e] first, then [s:e]
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

// Premise: should ensure the first real_length instructions in program p
// are not NOP, while the remainings are NOP.
// steps: 1. Set all instructions of this optimal program as NOP;
// 2. Compute combinations for real instruction positions;
// 3. replace NOP instructions with real instructions according to combinations.
// e.g. if optimal program has 2 real instuctions, one combination is [2,3],
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
  mh._when_to_restart.set_st(in_para.st_when_to_restart,
                             in_para.st_when_to_restart_niter);
  mh._start_prog.set_st(in_para.st_start_prog);
  mh._next_proposal.set_probability(in_para.p_inst_operand,
                                    in_para.p_inst);
  mh.turn_on_measure();
  prog orig(bm);
  orig.print();
  mh._cost.init(&orig, bm_len, inputs,
                in_para.w_e, in_para.w_p,
                in_para.st_ex, in_para.st_eq,
                in_para.st_avg);
  mh.mcmc_iter(in_para.niter, orig, prog_dic);
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
  string str_p_inst_operand = rm_useless_zero_digits_from_str(to_string(in_para.p_inst_operand));
  string str_p_inst = rm_useless_zero_digits_from_str(to_string(in_para.p_inst));
  string suffix = "_" + to_string(in_para.bm_id) +
                  "_" + to_string(in_para.niter) +
                  "_" + to_string(in_para.st_ex) +
                  to_string(in_para.st_eq) +
                  to_string(in_para.st_avg) +
                  "_" + str_w_e +
                  "_" + str_w_p +
                  "_" + to_string(in_para.st_when_to_restart) +
                  "_" + to_string(in_para.st_when_to_restart_niter) +
                  "_" + to_string(in_para.st_start_prog) +
                  "_" + str_p_inst_operand +
                  "_" + str_p_inst +
                  ".txt";
  file_raw_data_programs += suffix;
  file_raw_data_proposals += suffix;
  file_raw_data_examples += suffix;
  file_raw_data_optimals += "_" + to_string(in_para.bm_id) + ".txt";
}

string para_st_ex_desc() {
  string s = "strategy of error cost computation from example. " \
             "`arg`: 0(abs), 1(pop)";
  return s;
}

string para_st_eq_desc() {
  string s = "strategy of error cost computation equations. " \
             "`arg`: 0(eq1), 1(eq2)";
  return s;
}

string para_st_avg_desc() {
  string s = "strategy of whether average total error cost from examples. " \
             "`arg`: 0(navg), 1(avg)";
  return s;
}

string para_st_when_to_restart_desc() {
  string s = "strategy of when to restart during sampling. " \
             "`arg`: 0(no restart), 1(restart every `st_when_to_restart_niter`)";
  return s;
}

string para_st_when_to_restart_niter_desc() {
  string s = "when `st_when_to_restart` is set as 1, should set this parameter.";
  return s;
}

string para_st_start_prog_desc() {
  string s = "strategy of a new start program for restart" \
             "`arg`: 0(no change) 1(change all instructions) 2(change k continuous instructions)";
  return s;
}

string para_next_proposal_desc() {
  string s = "The next two parameters are about new proposal generation.\n" \
             "A new proposal has three modification typies: modify a random instrution operand, \n" \
             "instruction and two continuous instructions. Sum of their probabilities is 1. \n" \
             "The three probabilities are set by `p_inst_operand` and `p_inst` " \
             "(`p_two_cont_insts` can be computed)";
  return s;
}

string para_p_inst_operand_desc() {
  string s = "probability of modifying a random operand in a random instruction for a new proposal";
  return s;
}

string para_p_inst_desc() {
  string s = "probability of modifying a random instruction (both opcode and operands) for a new proposal";
  return s;
}

void usage() {
  // setw(.): Sets the field width to be used on output operations.
  // reference: http://www.cplusplus.com/reference/iomanip/setw/
  const int W = 31; // field width
  cout << "usage: " << endl
       << "options and descriptions" << endl
       << left // set setw(.) as left-aligned
       << setw(W) << "-h" << ": display usage" << endl
       << setw(W) << "-n arg" << ": number of iterations" << endl
       << setw(W) << "--path_out arg" << ": output file path" << endl
       << setw(W) << "--bm arg" << ": benchmark ID" << endl
       << endl
       << setw(W) << "--we arg" << ": weight of error cost in cost function" << endl
       << setw(W) << "--wp arg" << ": weight of performance cost in cost function" << endl
       << endl
       << setw(W) << "--st_ex arg" << ": " +  para_st_ex_desc() << endl
       << setw(W) << "--st_eq arg" << ": " +  para_st_eq_desc() << endl
       << setw(W) << "--st_avg arg" << ": " + para_st_avg_desc() << endl
       << endl
       << setw(W) << "--st_when_to_restart arg" << ": " + para_st_when_to_restart_desc() << endl
       << setw(W) << "--st_when_to_restart_niter arg" << ": "  + para_st_when_to_restart_niter_desc() << endl
       << setw(W) << "--st_start_prog arg" << ": " << para_st_start_prog_desc() << endl
       << endl << para_next_proposal_desc() << endl
       << setw(W) << "--p_inst_operand arg:" << ": " << para_p_inst_operand_desc() << endl
       << setw(W) << "--p_inst arg" << ": " << para_p_inst_desc() << endl;
}

bool parse_input_and_return_whether_to_measure(int argc, char* argv[], input_paras &in_para) {
  const char* const short_opts = "hn:";
  static struct option long_opts[] = {
    {"path_out", required_argument, nullptr, 0},
    {"bm", required_argument, nullptr, 1},
    {"we", required_argument, nullptr, 2},
    {"wp", required_argument, nullptr, 3},
    {"st_ex", required_argument, nullptr, 4},
    {"st_eq", required_argument, nullptr, 5},
    {"st_avg", required_argument, nullptr, 6},
    {"st_when_to_restart", required_argument, nullptr, 7},
    {"st_when_to_restart_niter", required_argument, nullptr, 8},
    {"st_start_prog", required_argument, nullptr, 9},
    {"p_inst_operand", required_argument, nullptr, 10},
    {"p_inst", required_argument, nullptr, 11},
    {nullptr, no_argument, nullptr, 0}
  };
  int opt;
  while ((opt = getopt_long(argc, argv, short_opts,
                            long_opts, nullptr)) != -1) {
    switch (opt) {
      case 'h': usage(); return false;
      case 'n': in_para.niter = stoi(optarg); break;
      case 0: in_para.path = optarg; break;
      case 1: in_para.bm_id = stoi(optarg); break;
      case 2: in_para.w_e = stod(optarg); break;
      case 3: in_para.w_p = stod(optarg); break;
      case 4: in_para.st_ex = stoi(optarg); break;
      case 5: in_para.st_eq = stoi(optarg); break;
      case 6: in_para.st_avg = stoi(optarg); break;
      case 7: in_para.st_when_to_restart = stoi(optarg); break;
      case 8: in_para.st_when_to_restart_niter = stoi(optarg); break;
      case 9: in_para.st_start_prog = stoi(optarg); break;
      case 10: in_para.p_inst_operand = stod(optarg); break;
      case 11: in_para.p_inst = stod(optarg); break;
      case '?': usage(); return false;
    }
  }
  return true;
}

void set_default_para_vals(input_paras &in_para) {
  in_para.niter = 10;
  in_para.w_e = 1.0;
  in_para.w_p = 0.0;
  in_para.bm_id = 0;
  in_para.path = "measure/";
  in_para.st_ex = ERROR_COST_STRATEGY_ABS;
  in_para.st_eq = ERROR_COST_STRATEGY_EQ1;
  in_para.st_avg = ERROR_COST_STRATEGY_NAVG;
  in_para.st_when_to_restart = MH_SAMPLER_ST_WHEN_TO_RESTART_NO_RESTART;
  in_para.st_when_to_restart_niter = 1000;
  in_para.st_start_prog = MH_SAMPLER_ST_NEXT_START_PROG_ORIG;
  in_para.p_inst_operand = 1.0 / 3.0;
  in_para.p_inst = 2.0 / 3.0;
}

int main(int argc, char* argv[]) {
  input_paras in_para;
  set_default_para_vals(in_para);
  if (! parse_input_and_return_whether_to_measure(argc, argv, in_para)) return 0;
  gen_file_name_from_input(in_para);
  vector<inst*> bm_optis_orig;
  init_benchmarks(bm_optis_orig, in_para.bm_id);
  // get all optimal programs from the original ones
  gen_optis_for_progs(bm_optis_orig);
  inputs.resize(30);
  gen_random_input(inputs, -50, 50);
  run_mh_sampler_and_store_data(in_para);
  return 0;
}
