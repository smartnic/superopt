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
#include <chrono>
#include "measure/benchmark_toy_isa.h"
#include "measure/benchmark_ebpf.h"
#include "measure/meas_mh_bhv.h"
#include "src/inout.h"
#include "src/utils.h"
#include "src/search/mh_prog.h"
#include "src/isa/prog.h"
#include "main.h"

using namespace std;

string FILE_CONFIG = "config";

inst* bm;
vector<reg_t> inputs;
std::unordered_map<int, vector<prog*> > prog_dic;

ostream& operator<<(ostream& out, const input_paras& ip) {
  out << "isa:" << ip.isa << endl
      << "meas_mode:" << ip.meas_mode << endl
      << "path_out:" << ip.path_out << endl
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
      << "restart_w_e_list:" << ip.restart_w_e_list << endl
      << "restart_w_p_list:" << ip.restart_w_p_list << endl
      << "p_inst_operand:" << ip.p_inst_operand << endl
      << "p_inst:" << ip.p_inst << endl;
  return out;
}

void init_benchmarks_toy_isa(vector<inst*> &bm_optis_orig, int bm_id) {
  switch (bm_id) {
    case 0:
      bm = bm0;
      bm_optis_orig.push_back(bm_opti00);
      bm_optis_orig.push_back(bm_opti01);
      bm_optis_orig.push_back(bm_opti02);
      bm_optis_orig.push_back(bm_opti03);
      bm_optis_orig.push_back(bm_opti04);
      bm_optis_orig.push_back(bm_opti05);
      return;
    case 1:
      bm = bm1;
      bm_optis_orig.push_back(bm_opti10);
      bm_optis_orig.push_back(bm_opti11);
      bm_optis_orig.push_back(bm_opti12);
      bm_optis_orig.push_back(bm_opti13);
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
      return;
    default:
      cout << "ERROR: toy-isa bm_id " + to_string(bm_id) + " is out of range {0, 1, 2}" << endl;
      return;
  }
}

void init_benchmarks_ebpf(vector<inst*> &bm_optis_orig, int bm_id) {
  switch (bm_id) {
    case 0:
      bm = ebpf_bm0;
      bm_optis_orig.push_back(ebpf_bm_opti00);
      return;
    default:
      cout << "ERROR: ebpf bm_id " + to_string(bm_id) + " is out of range {0}" << endl;
      return;
  }
}

void init_benchmarks(vector<inst*> &bm_optis_orig, int bm_id, int isa) {
  if (isa == 0) {
    init_benchmarks_toy_isa(bm_optis_orig, bm_id);
  } else if (isa == 1) {
    init_benchmarks_ebpf(bm_optis_orig, bm_id);
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
  mh._restart.set_we_wp_list(in_para.restart_w_e_list, in_para.restart_w_p_list);
  mh._next_proposal.set_probability(in_para.p_inst_operand,
                                    in_para.p_inst);
  if (in_para.meas_mode) mh.turn_on_measure();
  prog orig(bm);
  orig.print();
  mh._cost.init(in_para.isa, &orig, orig.get_max_prog_len(), inputs,
                in_para.w_e, in_para.w_p,
                in_para.st_ex, in_para.st_eq,
                in_para.st_avg);
  mh.mcmc_iter(in_para.niter, orig, prog_dic);
  if (in_para.meas_mode) {
    string suffix = gen_file_name_suffix_from_input(in_para);
    vector<prog> bm_optimals;
    // get all optimal programs from the original ones
    gen_optis_for_progs(bm_optis_orig, bm_optimals);
    meas_store_raw_data(mh._meas_data, in_para.path_out,
                        suffix, in_para.bm, bm_optimals, in_para.isa);
    mh.turn_off_measure();
  }
}

void store_config_to_file(const input_paras &in_para) {
  string suffix = gen_file_name_suffix_from_input(in_para);
  string file_name = in_para.path_out + FILE_CONFIG + suffix;
  fstream fout;
  fout.open(file_name, ios::out | ios::trunc);
  fout << in_para;
  fout.close();
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

string para_restart_desc() {
  string s = "The next five parameters are about restart. The last four " \
             "parameters work ONLY when `st_when_to_restart` is not set as 0";
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
  string s = "strategy of a new start program for restart." \
             "`arg`: 0(no change) 1(change all instructions) 2(change k continuous instructions)";
  return s;
}

string para_restart_w_e_list_desc() {
  string s = "weights of error cost in cost function for restart. " \
             "`arg` eg: 1.5,0.5";
  return s;
}

string para_restart_w_p_list_desc() {
  string s = "weights of performance cost in cost function for restart. " \
             "`arg` eg: 1.5,0.5";
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
       << setw(W) << "--isa arg" << ": ISA type, `arg`: 0(toy_isa), 1(ebpf)" << endl
       << setw(W) << "-n arg" << ": number of iterations" << endl
       << setw(W) << "-m" << ": turn on measurement" << endl
       << setw(W) << "--path_out arg" << ": output file path" << endl
       << setw(W) << "--bm arg" << ": benchmark ID. toy_isa: 0 - 2; ebpf: 0" << endl
       << setw(W) << "--w_e arg" << ": weight of error cost in cost function" << endl
       << setw(W) << "--w_p arg" << ": weight of performance cost in cost function" << endl
       << setw(W) << "--st_ex arg" << ": " +  para_st_ex_desc() << endl
       << setw(W) << "--st_eq arg" << ": " +  para_st_eq_desc() << endl
       << setw(W) << "--st_avg arg" << ": " + para_st_avg_desc() << endl
       << endl << para_restart_desc() << endl
       << setw(W) << "--st_when_to_restart arg" << ": " + para_st_when_to_restart_desc() << endl
       << setw(W) << "--st_when_to_restart_niter arg" << ": "  + para_st_when_to_restart_niter_desc() << endl
       << setw(W) << "--st_start_prog arg" << ": " << para_st_start_prog_desc() << endl
       << setw(W) << "--restart_w_e_list arg" << ": " << para_restart_w_e_list_desc() << endl
       << setw(W) << "--restart_w_p_list arg" << ": " << para_restart_w_p_list_desc() << endl
       << endl << para_next_proposal_desc() << endl
       << setw(W) << "--p_inst_operand arg:" << ": " << para_p_inst_operand_desc() << endl
       << setw(W) << "--p_inst arg" << ": " << para_p_inst_desc() << endl;
}

void set_w_list(vector<double> &list, string s) {
  vector<string> str_v;
  split_string(s, str_v, ",");
  list.clear();
  for (size_t i = 0; i < str_v.size(); i++)
    list.push_back(stod(str_v[i]));
}

bool parse_input(int argc, char* argv[], input_paras &in_para) {
  const char* const short_opts = "hmn:";
  static struct option long_opts[] = {
    {"path_out", required_argument, nullptr, 0},
    {"bm", required_argument, nullptr, 1},
    {"w_e", required_argument, nullptr, 2},
    {"w_p", required_argument, nullptr, 3},
    {"st_ex", required_argument, nullptr, 4},
    {"st_eq", required_argument, nullptr, 5},
    {"st_avg", required_argument, nullptr, 6},
    {"st_when_to_restart", required_argument, nullptr, 7},
    {"st_when_to_restart_niter", required_argument, nullptr, 8},
    {"st_start_prog", required_argument, nullptr, 9},
    {"restart_w_e_list", required_argument, nullptr, 10},
    {"restart_w_p_list", required_argument, nullptr, 11},
    {"p_inst_operand", required_argument, nullptr, 12},
    {"p_inst", required_argument, nullptr, 13},
    {"isa", required_argument, nullptr, 14},
    {nullptr, no_argument, nullptr, 0}
  };
  int opt;
  while ((opt = getopt_long(argc, argv, short_opts,
                            long_opts, nullptr)) != -1) {
    switch (opt) {
      case 'h': usage(); return false;
      case 'm': in_para.meas_mode = true; break;
      case 'n': in_para.niter = stoi(optarg); break;
      case 0: in_para.path_out = optarg; break;
      case 1: in_para.bm = stoi(optarg); break;
      case 2: in_para.w_e = stod(optarg); break;
      case 3: in_para.w_p = stod(optarg); break;
      case 4: in_para.st_ex = stoi(optarg); break;
      case 5: in_para.st_eq = stoi(optarg); break;
      case 6: in_para.st_avg = stoi(optarg); break;
      case 7: in_para.st_when_to_restart = stoi(optarg); break;
      case 8: in_para.st_when_to_restart_niter = stoi(optarg); break;
      case 9: in_para.st_start_prog = stoi(optarg); break;
      case 10: set_w_list(in_para.restart_w_e_list, optarg); break;
      case 11: set_w_list(in_para.restart_w_p_list, optarg); break;
      case 12: in_para.p_inst_operand = stod(optarg); break;
      case 13: in_para.p_inst = stod(optarg); break;
      case 14: in_para.isa = stoi(optarg); break;
      case '?': usage(); return false;
    }
  }
  return true;
}

void set_default_para_vals(input_paras &in_para) {
  in_para.meas_mode = false;
  in_para.path_out = "measure/";
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
  in_para.restart_w_e_list = {1};
  in_para.restart_w_p_list = {0};
  in_para.p_inst_operand = 1.0 / 3.0;
  in_para.p_inst = 1.0 / 3.0;
  in_para.isa = 0;
}

int main(int argc, char* argv[]) {
  input_paras in_para;
  set_default_para_vals(in_para);
  if (! parse_input(argc, argv, in_para)) return 0;
  cout << in_para;
  store_config_to_file(in_para);
  vector<inst*> bm_optis_orig;
  auto start = NOW;
  init_benchmarks(bm_optis_orig, in_para.bm, in_para.isa);
  inputs.resize(30);
  gen_random_input(inputs, -50, 50);
  run_mh_sampler(in_para, bm_optis_orig);
  auto end = NOW;
  cout << "compiling time: " << DUR(start, end) << " us" << endl;
  return 0;
}
