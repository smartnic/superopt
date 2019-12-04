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
#include "measure/common.h"
#include "measure/meas_mh_bhv.h"
#include "prog.h"
#include "inout.h"
#include "mh_prog.h"
#include "inst.h"
#include "utils.h"

using namespace std;

inst* bm;
int bm_len = MAX_PROG_LEN;
vector<int> inputs;
std::unordered_map<int, vector<prog*> > prog_dic;

struct input_paras {
  int niter;
  int bm;
  double w_e;
  double w_p;
  bool meas_mode;
  string meas_path_out;
  int st_ex;
  int st_eq;
  int st_avg;
  int st_when_to_restart;
  int st_when_to_restart_niter;
  int st_start_prog;
  double p_inst_operand;
  double p_inst;
};

ostream& operator<<(ostream& out, const input_paras& ip) {
  out << "meas_mode:" << ip.meas_mode << endl
      << "meas_path_out:" << ip.meas_path_out << endl
      << "bm:" << ip.bm << endl
      << "niter:" << ip.niter << endl
      << "w_e:" << ip.w_e << endl
      << "w_p:" << ip.w_p << endl
      << "st_ex:" << ip.st_ex << endl
      << "st_eq:" << ip.st_eq << endl
      << "st_avg:" << ip.st_avg << endl
      << "st_when_to_restart:" << ip.st_when_to_restart << endl
      << "st_when_to_restart_niter:" << ip.st_when_to_restart_niter << endl
      << "st_start_prog:" << ip.st_start_prog << endl
      << "p_inst_operand:" << ip.p_inst_operand << endl
      << "p_inst:" << ip.p_inst << endl;
  return out;
}

void init_benchmarks(vector<inst*> &bm_optis_orig, int bm_id) {
  switch (bm_id) {
    case 0:
      bm = bm0;
      bm_optis_orig.push_back(bm_opti00);
      bm_optis_orig.push_back(bm_opti01);
      bm_optis_orig.push_back(bm_opti02);
      bm_optis_orig.push_back(bm_opti03);
      return;
    case 1:
      bm = bm1;
      bm_optis_orig.push_back(bm_opti10);
      bm_optis_orig.push_back(bm_opti11);
      return;
    case 2:
      bm = bm2;
      bm_optis_orig.push_back(bm_opti20);
      bm_optis_orig.push_back(bm_opti21);
      bm_optis_orig.push_back(bm_opti22);
      return;
    default:
      cout << "bm_id" + to_string(bm_id) + "is out of range {0, 1, 2}" << endl;
      return;
  }
}

// eg. "1.1100" -> "1.11"; "1.000" -> "1"
string rm_useless_zero_digits_from_str(string s) {
  // rm useless zero digits
  s.erase(s.find_last_not_of('0') + 1, string::npos);
  // rm useless decimal point
  s.erase(s.find_last_not_of('.') + 1, string::npos);
  return s;
}

string gen_file_name_suffix_from_input(const input_paras &in_para) {
  string str_w_e = rm_useless_zero_digits_from_str(to_string(in_para.w_e));
  string str_w_p = rm_useless_zero_digits_from_str(to_string(in_para.w_p));
  string str_p_inst_operand = rm_useless_zero_digits_from_str(to_string(in_para.p_inst_operand));
  string str_p_inst = rm_useless_zero_digits_from_str(to_string(in_para.p_inst));
  string suffix = "_" + to_string(in_para.bm) +
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
  return suffix;
}

void run_mh_sampler(const input_paras &in_para, vector<inst*> &bm_optis_orig) {
  mh_sampler mh;
  mh._restart.set_st_when_to_restart(in_para.st_when_to_restart,
                                     in_para.st_when_to_restart_niter);
  mh._restart.set_st_next_start_prog(in_para.st_start_prog);
  mh._next_proposal.set_probability(in_para.p_inst_operand,
                                    in_para.p_inst);
  if (in_para.meas_mode) mh.turn_on_measure();
  prog orig(bm);
  orig.print();
  mh._cost.init(&orig, bm_len, inputs,
                in_para.w_e, in_para.w_p,
                in_para.st_ex, in_para.st_eq,
                in_para.st_avg);
  mh.mcmc_iter(in_para.niter, orig, prog_dic);
  if (in_para.meas_mode) {
    string suffix = gen_file_name_suffix_from_input(in_para);
    vector<prog> bm_optimals;
    // get all optimal programs from the original ones
    gen_optis_for_progs(bm_optis_orig, bm_optimals);
    meas_store_raw_data(mh._meas_data, in_para.meas_path_out,
                        suffix, in_para.bm, bm_optimals);
    mh.turn_off_measure();
  }
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
       << setw(W) << "-m" << ": turn on measurement" << endl
       << setw(W) << "--meas_path_out arg" << ": measurement output file path" << endl
       << setw(W) << "--bm arg" << ": benchmark ID" << endl
       << setw(W) << "--we arg" << ": weight of error cost in cost function" << endl
       << setw(W) << "--wp arg" << ": weight of performance cost in cost function" << endl
       << setw(W) << "--st_ex arg" << ": " +  para_st_ex_desc() << endl
       << setw(W) << "--st_eq arg" << ": " +  para_st_eq_desc() << endl
       << setw(W) << "--st_avg arg" << ": " + para_st_avg_desc() << endl
       << setw(W) << "--st_when_to_restart arg" << ": " + para_st_when_to_restart_desc() << endl
       << setw(W) << "--st_when_to_restart_niter arg" << ": "  + para_st_when_to_restart_niter_desc() << endl
       << setw(W) << "--st_start_prog arg" << ": " << para_st_start_prog_desc() << endl
       << endl << para_next_proposal_desc() << endl
       << setw(W) << "--p_inst_operand arg:" << ": " << para_p_inst_operand_desc() << endl
       << setw(W) << "--p_inst arg" << ": " << para_p_inst_desc() << endl;
}

bool parse_input_and_return_whether_to_sample(int argc, char* argv[], input_paras &in_para) {
  const char* const short_opts = "hmn:";
  static struct option long_opts[] = {
    {"meas_path_out", required_argument, nullptr, 0},
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
      case 'm': in_para.meas_mode = true; break;
      case 'n': in_para.niter = stoi(optarg); break;
      case 0: in_para.meas_path_out = optarg; break;
      case 1: in_para.bm = stoi(optarg); break;
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
  in_para.meas_mode = false;
  in_para.meas_path_out = "measure/";
  in_para.bm = 0;
  in_para.niter = 10;
  in_para.w_e = 1.0;
  in_para.w_p = 0.0;
  in_para.st_ex = ERROR_COST_STRATEGY_ABS;
  in_para.st_eq = ERROR_COST_STRATEGY_EQ1;
  in_para.st_avg = ERROR_COST_STRATEGY_NAVG;
  in_para.st_when_to_restart = MH_SAMPLER_ST_WHEN_TO_RESTART_NO_RESTART;
  in_para.st_when_to_restart_niter = 0;
  in_para.st_start_prog = MH_SAMPLER_ST_NEXT_START_PROG_ORIG;
  in_para.p_inst_operand = 1.0 / 3.0;
  in_para.p_inst = 1.0 / 3.0;
}

int main(int argc, char* argv[]) {
  input_paras in_para;
  set_default_para_vals(in_para);
  if (! parse_input_and_return_whether_to_sample(argc, argv, in_para)) return 0;
  cout << in_para;
  vector<inst*> bm_optis_orig;
  init_benchmarks(bm_optis_orig, in_para.bm);
  inputs.resize(30);
  gen_random_input(inputs, -50, 50);
  run_mh_sampler(in_para, bm_optis_orig);
  return 0;
}
