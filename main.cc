#include <iostream>
#include <fstream>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unordered_map>
#include <algorithm>
#include <unordered_set>
#include <map>
#include <set>
#include <utility>
#include <iomanip>
#include <limits>
#include <getopt.h>
#include <chrono>
#include <cstdlib>
#include "src/utils.h"
#include "src/inout.h"
#include "src/isa/inst_header.h"
#include "src/isa/prog.h"
#include "src/search/mh_prog.h"
#include "src/search/win_select.h"
#include "measure/benchmark_header.h"
#include "measure/meas_mh_bhv.h"
#include "main.h"
#include "src/verify/z3client.h"
using namespace std;

string FILE_CONFIG = "config";

inst* bm;
vector<inout_t> inputs;

ostream& operator<<(ostream& out, const input_paras& ip) {
  out << "niter:" << ip.niter << endl
      << "k:" << ip.k << endl
      << "meas_mode:" << ip.meas_mode << endl
      << "path_out:" << ip.path_out << endl
      << "bm:" << ip.bm << endl
      << "bm_from_file: " << ip.bm_from_file << ", "
      << "bytecode: " << ip.bytecode << ", "
      << "map: " << ip.map << ", "
      << "desc: " << ip.desc << endl
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
      << "reset_win_niter:" << ip.reset_win_niter << endl;
  out << "win list: ";
  for (int i = 0; i < ip.win_s_list.size(); i++) {
    out << "[" << ip.win_s_list[i] << "," << ip.win_e_list[i] << "] ";
  }
  out << endl;
  out << "p_inst_operand:" << ip.p_inst_operand << endl
      << "p_inst:" << ip.p_inst << endl
      << "p_inst_as_nop:" << ip.p_inst_as_nop << endl;
  out << "server_port:" << ip.server_port << endl
      << "disable_prog_eq_cache:" << ip.disable_prog_eq_cache << endl
      << "enable_prog_uneq_cache:" << ip.enable_prog_uneq_cache << endl
      << "is_win:" << ip.is_win << endl
      << "logger_level: " << ip.logger_level << endl;
  return out;
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
  string str_p_inst_as_nop = rm_useless_zero_digits_from_str(to_string(in_para.p_inst_as_nop));
  string suffix = "_" + to_string(in_para.bm) +
                  "_" + to_string(in_para.niter) +
                  "_" + to_string(in_para.st_ex) +
                  to_string(in_para.st_eq) +
                  to_string(in_para.st_avg) +
                  to_string(in_para.st_perf) +
                  "_" + str_w_e +
                  "_" + str_w_p +
                  "_" + to_string(in_para.st_when_to_restart) +
                  "_" + to_string(in_para.st_when_to_restart_niter) +
                  "_" + to_string(in_para.st_start_prog) +
                  "_" + str_p_inst_operand +
                  "_" + str_p_inst +
                  "_" + str_p_inst_as_nop +
                  ".txt";
  return suffix;
}

void run_mh_sampler(top_k_progs& topk_progs, input_paras &in_para, vector<inst*> &bm_optis_orig) {
  cout << "run_mh_sampler..." << endl;
  mh_sampler mh;
  mh._restart.set_st_when_to_restart(in_para.st_when_to_restart,
                                     in_para.st_when_to_restart_niter);
  mh._restart.set_st_next_start_prog(in_para.st_start_prog);
  mh._restart.set_we_wp_list(in_para.restart_w_e_list, in_para.restart_w_p_list);
  mh._next_proposal.set_probability(in_para.p_inst_operand,
                                    in_para.p_inst,
                                    in_para.p_inst_as_nop);
  mh._next_win.set_win_lists(in_para.win_s_list, in_para.win_e_list);
  mh._next_win.set_max_num_iter(in_para.reset_win_niter);
  if (in_para.meas_mode) mh.turn_on_measure();
  prog orig(bm);
  orig.print();
  mh._cost.init(&orig, inst::max_prog_len, inputs,
                in_para.w_e, in_para.w_p,
                in_para.st_ex, in_para.st_eq,
                in_para.st_avg, in_para.st_perf,
                (! in_para.disable_prog_eq_cache),
                in_para.enable_prog_uneq_cache,
                in_para.is_win);
  try {
    mh.mcmc_iter(topk_progs, in_para.niter, &orig, in_para.is_win);
  } catch (string err_msg) {
    cout << err_msg << endl;
  }
  mh._cost._vld.print_counters();
  if (in_para.meas_mode) {
    string suffix = gen_file_name_suffix_from_input(in_para);
    vector<prog> bm_optimals;
    // get all optimal programs from the original ones
    gen_optis_for_progs(bm_optis_orig, bm_optimals);
    meas_store_raw_data(mh._meas_data, in_para.path_out,
                        suffix, in_para.bm, bm_optimals);
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

string para_bm_desc() {
  string s = "There are two ways for the compiler to get benchmarks: \n"\
             "1. read benchmark which is in the code: use `--bm`;\n" \
             "2. (only support ebpf)read benchmark from files: use `--bytecode`, `--map`, `--desc` to " \
             "set the input file path. There is an example in input/";
  return s;
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

string para_st_perf_desc() {
  string s = "strategy of whether performance cost for examples. " \
             "`arg`: 0(program length), 1(program runtime)";
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

string para_win_desc() {
  string s = "configurations of window decomposition method. window range: [0, max_pgm_len-1]";
  return s;
}

string para_reset_win_niter_desc() {
  string s = "iterations of setting next window during sampling, should > 0";
  return s;
}

string para_win_s_list_desc() {
  string s = "window start list. `arg` eg: 0,4";
  return s;
}

string para_win_e_list_desc() {
  string s = "window end list. `arg` eg: 3,6";
  return s;
}

string para_next_proposal_desc() {
  string s = "The next two parameters are about new proposal generation.\n" \
             "A new proposal has three modification typies: modify a random instruction operand, \n" \
             "instruction, two continuous instructions, and modify a random instruction as NOP. \n" \
             "Sum of their probabilities is 1. \n" \
             "The four probabilities are set by `p_inst_operand`, `p_inst` and `p_inst_as_nop` " \
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

string para_p_inst_as_nop_desc() {
  string s = "probability of modifying a random instruction as NOP for a new proposal";
  return s;
}

string para_port_desc() {
  string s = "port number the z3 server and client communicate on";
  return s;
}

string para_logger_level_desc() {
  string s = "least logger level that allows to print: 0(ERROR), 1(DEBUG)";
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
       << setw(W) << "-k arg" << ": number of top programs" << endl
       << setw(W) << "-m" << ": turn on measurement" << endl
       << setw(W) << "--path_out arg" << ": output file path" << endl
       << endl << para_bm_desc() << endl
       << setw(W) << "--bm arg" << ": benchmark ID. toy_isa: 0 - 2; ebpf: 0 - 2" << endl
       << setw(W) << "--bm_from_file" << ": benchmark from file flag. the default is false" << endl
       << setw(W) << "--bytecode arg" << ": bpf bytecode file" << endl
       << setw(W) << "--map arg" << ": bpf map attributes file" << endl
       << setw(W) << "--desc arg" << ": bpf bytecode description file" << endl
       << setw(W) << "--w_e arg" << ": weight of error cost in cost function" << endl
       << setw(W) << "--w_p arg" << ": weight of performance cost in cost function" << endl
       << setw(W) << "--st_ex arg" << ": " +  para_st_ex_desc() << endl
       << setw(W) << "--st_eq arg" << ": " +  para_st_eq_desc() << endl
       << setw(W) << "--st_avg arg" << ": " + para_st_avg_desc() << endl
       << setw(W) << "--st_perf arg" << ": " + para_st_perf_desc() << endl
       << endl << para_restart_desc() << endl
       << setw(W) << "--st_when_to_restart arg" << ": " + para_st_when_to_restart_desc() << endl
       << setw(W) << "--st_when_to_restart_niter arg" << ": "  + para_st_when_to_restart_niter_desc() << endl
       << setw(W) << "--st_start_prog arg" << ": " << para_st_start_prog_desc() << endl
       << setw(W) << "--restart_w_e_list arg" << ": " << para_restart_w_e_list_desc() << endl
       << setw(W) << "--restart_w_p_list arg" << ": " << para_restart_w_p_list_desc() << endl
       << endl << para_win_desc() << endl
       << setw(W) << "--reset_win_niter arg" << ": " + para_reset_win_niter_desc() << endl
       << setw(W) << "--win_s_list arg" << ": " + para_win_s_list_desc() << endl
       << setw(W) << "--win_e_list arg" << ": " + para_win_e_list_desc() << endl
       << endl << para_next_proposal_desc() << endl
       << setw(W) << "--p_inst_operand arg:" << ": " << para_p_inst_operand_desc() << endl
       << setw(W) << "--p_inst arg" << ": " << para_p_inst_desc() << endl
       << setw(W) << "--p_inst_as_nop arg" << ": " << para_p_inst_as_nop_desc() << endl
       << setw(W) << "--port arg" << ": " << para_port_desc() << endl
       << endl << "validator related arguments" << endl
       << setw(W) << "--disable_prog_eq_cache: disable the usage of prog_eq_cache" << endl
       << setw(W) << "--enable_prog_uneq_cache: enable the usage of prog_uneq_cache" << endl
       << setw(W) << "--is_win: enable window program equivalence check" << endl
       << endl << setw(W) << "--logger_level" << ": " << para_logger_level_desc() << endl;
}

void set_w_list(vector<double> &list, string s) {
  vector<string> str_v;
  split_string(s, str_v, ",");
  list.clear();
  for (size_t i = 0; i < str_v.size(); i++)
    list.push_back(stod(str_v[i]));
}

void set_win_list(vector<int> &list, string s) {
  vector<string> str_v;
  split_string(s, str_v, ",");
  list.clear();
  for (size_t i = 0; i < str_v.size(); i++)
    list.push_back(stod(str_v[i]));
}

bool parse_input(int argc, char* argv[], input_paras &in_para) {
  const char* const short_opts = "hmn:k:";
  static struct option long_opts[] = {
    {"path_res", required_argument, nullptr, 0},
    {"bm", required_argument, nullptr, 1},
    {"bm_from_file", no_argument, nullptr, 2},
    {"bytecode", required_argument, nullptr, 3},
    {"map", required_argument, nullptr, 4},
    {"desc", required_argument, nullptr, 5},
    {"w_e", required_argument, nullptr, 6},
    {"w_p", required_argument, nullptr, 7},
    {"st_ex", required_argument, nullptr, 8},
    {"st_eq", required_argument, nullptr, 9},
    {"st_avg", required_argument, nullptr, 10},
    {"st_perf", required_argument, nullptr, 11},
    {"st_when_to_restart", required_argument, nullptr, 12},
    {"st_when_to_restart_niter", required_argument, nullptr, 13},
    {"st_start_prog", required_argument, nullptr, 14},
    {"restart_w_e_list", required_argument, nullptr, 15},
    {"restart_w_p_list", required_argument, nullptr, 16},
    {"reset_win_niter", required_argument, nullptr, 17},
    {"win_s_list", required_argument, nullptr, 18},
    {"win_e_list", required_argument, nullptr, 19},
    {"p_inst_operand", required_argument, nullptr, 20},
    {"p_inst", required_argument, nullptr, 21},
    {"p_inst_as_nop", required_argument, nullptr, 22},
    {"port", required_argument, nullptr, 23},
    {"disable_prog_eq_cache", no_argument, nullptr, 24},
    {"enable_prog_uneq_cache", no_argument, nullptr, 25},
    {"is_win", no_argument, nullptr, 26},
    {"logger_level", required_argument, nullptr, 27},
    {nullptr, no_argument, nullptr, 0}
  };
  int opt;
  int opt_idx = 0;
  while ((opt = getopt_long(argc, argv, short_opts,
                            long_opts, &opt_idx)) != -1) {
    switch (opt) {
      case 'h': usage(); return false;
      case 'm': in_para.meas_mode = true; break;
      case 'n': in_para.niter = stoi(optarg); break;
      case 'k': in_para.k = stoi(optarg); break;
      case 0: in_para.path_out = optarg; break;
      case 1: in_para.bm = stoi(optarg); break;
      case 2: in_para.bm_from_file = true; break;
      case 3: in_para.bytecode = optarg; break;
      case 4: in_para.map = optarg; break;
      case 5: in_para.desc = optarg; break;
      case 6: in_para.w_e = stod(optarg); break;
      case 7: in_para.w_p = stod(optarg); break;
      case 8: in_para.st_ex = stoi(optarg); break;
      case 9: in_para.st_eq = stoi(optarg); break;
      case 10: in_para.st_avg = stoi(optarg); break;
      case 11: in_para.st_perf = stoi(optarg); break;
      case 12: in_para.st_when_to_restart = stoi(optarg); break;
      case 13: in_para.st_when_to_restart_niter = stoi(optarg); break;
      case 14: in_para.st_start_prog = stoi(optarg); break;
      case 15: set_w_list(in_para.restart_w_e_list, optarg); break;
      case 16: set_w_list(in_para.restart_w_p_list, optarg); break;
      case 17: in_para.reset_win_niter = stoi(optarg); break;
      case 18: set_win_list(in_para.win_s_list, optarg); break;
      case 19: set_win_list(in_para.win_e_list, optarg); break;
      case 20: in_para.p_inst_operand = stod(optarg); break;
      case 21: in_para.p_inst = stod(optarg); break;
      case 22: in_para.p_inst_as_nop = stod(optarg); break;
      case 23: in_para.server_port = stoi(optarg); break;
      case 24: in_para.disable_prog_eq_cache = true; break;
      case 25: in_para.enable_prog_uneq_cache = true; break;
      case 26: in_para.is_win = true; break;
      case 27: in_para.logger_level = stoi(optarg); break;
      case '?': usage(); return false;
    }
  }
  return true;
}

// best programs are programs with the smallest performance cost among zero-error-cost programs
void get_best_pgms_from_candidates(vector<prog*>& best_pgms, unordered_map<int, vector<prog*> >& prog_dic) {
  double min_perf_cost = numeric_limits<double>::max();
  // get the minimum performance cost with zero error cost
  for (auto& element : prog_dic) {
    vector<prog*>& pl = element.second; // list of progs with the same hash
    for (auto p : pl) {
      if (p->_error_cost == 0) {
        min_perf_cost = min(min_perf_cost, p->_perf_cost);
      }
    }
  }

  best_pgms.clear();
  for (auto& element : prog_dic) {
    vector<prog*>& pl = element.second; // list of progs with the same hash
    for (auto p : pl) {
      if ((p->_error_cost == 0) && (p->_perf_cost == min_perf_cost)) {
        prog* p_copy = new prog(*p);
        best_pgms.push_back(p_copy);
      }
    }
  }
}

void set_default_para_vals(input_paras & in_para) {
  in_para.meas_mode = false;
  in_para.path_out = "output/";
  in_para.bm = 0;
  in_para.bm_from_file = false;
  in_para.bytecode = "";
  in_para.desc = "";
  in_para.niter = 10;
  in_para.k = 1;
  in_para.w_e = 1.0;
  in_para.w_p = 0.0;
  in_para.st_ex = ERROR_COST_STRATEGY_ABS;
  in_para.st_eq = ERROR_COST_STRATEGY_EQ1;
  in_para.st_avg = ERROR_COST_STRATEGY_NAVG;
  in_para.st_perf = PERF_COST_STRATEGY_LEN;
  in_para.st_when_to_restart = MH_SAMPLER_ST_WHEN_TO_RESTART_NO_RESTART;
  in_para.st_when_to_restart_niter = 0;
  in_para.st_start_prog = MH_SAMPLER_ST_NEXT_START_PROG_ORIG;
  in_para.restart_w_e_list = {1};
  in_para.restart_w_p_list = {0};
  in_para.reset_win_niter = in_para.niter;
  in_para.win_s_list = {};
  in_para.win_e_list = {};
  in_para.p_inst_operand = 1.0 / 4.0;
  in_para.p_inst = 1.0 / 4.0;
  in_para.p_inst_as_nop = 1.0 / 4.0;
  in_para.server_port = 8002;
  in_para.disable_prog_eq_cache = false;
  in_para.enable_prog_uneq_cache = false;
  in_para.is_win = false;
  in_para.logger_level = LOGGER_ERROR;
}

void write_insns_to_file(prog* current_program, string prefix_name) {
  string output_file = prefix_name + ".insns";
  FILE* output_file_fp = fopen(output_file.c_str(), "w");
  for (int i = 0; i < inst::max_prog_len; i++) {
    inst t_insn = current_program->inst_list[i];
    struct bpf_insn insn = {(uint8_t)t_insn._opcode,
             (uint8_t)t_insn._dst_reg,
             (uint8_t)t_insn._src_reg,
             t_insn._off,
             t_insn._imm
    };
    fwrite(&insn, sizeof(bpf_insn), 1, output_file_fp);
  }

  fclose(output_file_fp);
}

// readable code + perf_cost
void write_desc_to_file(prog* current_program, string prefix_name) {
  string output_file = prefix_name + ".desc";
  ofstream fout;
  fout.open(output_file, ios::out | ios::trunc);
  fout << "perf_cost: " << current_program->_perf_cost << endl;
  fout << "readable program: " << endl;
  for (int i = 0; i < inst::max_prog_len; i++) {
    fout << i << ": " << current_program->inst_list[i];
  }
  fout.close();
}

void write_bpf_insns_to_file(prog* current_program, string prefix_name) {
  string output_file = prefix_name + ".bpf_insns";
  ofstream fout;
  fout.open(output_file, ios::out | ios::trunc);
  int real_len = num_real_instructions(current_program->inst_list, inst::max_prog_len);
  for (int i = 0; i < real_len; i++) {
    fout << current_program->inst_list[i].get_bytecode_str() << "," << endl;
  }
  fout.close();
}

void write_optimized_prog_to_file(prog* current_program, int id, string path_out) {
  // create path_out if not exist
  if (access(path_out.c_str(), 0) != 0) {
    if (mkdir(path_out.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) != 0) {
      cout << "ERROR: mkdir path_out " << path_out << " failed" << endl;
      return;
    }
  }
  convert_superopt_pgm_to_bpf_pgm(current_program->inst_list, inst::max_prog_len);
  prog* p_bpf_insns = new prog(*current_program);

  string prefix_name = path_out + "output" + to_string(id);
  set_nops_as_JA0(current_program->inst_list, inst::max_prog_len);
  write_desc_to_file(current_program, prefix_name);
  write_insns_to_file(current_program, prefix_name);

  remove_nops(p_bpf_insns->inst_list, inst::max_prog_len);
  write_bpf_insns_to_file(p_bpf_insns, prefix_name);
}

void generate_wins(vector<int>& win_s_list, vector<int>& win_e_list) {
  prog_static_state pss;
  static_analysis(pss, bm, inst::max_prog_len);
  vector<pair<int, int>> wins;
  gen_wins(wins, bm, inst::max_prog_len, pss);
  optimize_wins(wins);
  int seed = 1;
  std::srand(seed);
  std::random_shuffle(wins.begin(), wins.end());
  for (int i = 0; i < wins.size(); i++) {
    win_s_list.push_back(wins[i].first);
    win_e_list.push_back(wins[i].second);
  }
}

int main(int argc, char* argv[]) {
  dur_sum = 0;
  dur_sum_long = 0;
  n_sum_long = 0;
  input_paras in_para;
  set_default_para_vals(in_para);
  if (! parse_input(argc, argv, in_para)) return 0;

  string output_file = in_para.path_out + "log.txt";
  // create output_file if not exist
  if (access(in_para.path_out.c_str(), 0) != 0) {
    if (mkdir(in_para.path_out.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) != 0) {
      cout << "ERROR: mkdir output_file " << in_para.path_out << " failed" << endl;
      return 0;
    }
  }
  std::ofstream out(output_file);
  std::streambuf *coutbuf = std::cout.rdbuf(); //save old buf
  std::cout.rdbuf(out.rdbuf()); //redirect std::cout to output_file
  cout << in_para;
  // store_config_to_file(in_para);
  vector<inst*> bm_optis_orig;
  // todo: a temporary way to set up win eq check flag
  smt_var::is_win = in_para.is_win;
  auto start = NOW;
  if (in_para.bm_from_file) {
    init_benchmark_from_file(&bm, in_para.bytecode.c_str(),
                             in_para.map.c_str(), in_para.desc.c_str());
  } else {
    init_benchmarks(&bm, bm_optis_orig, in_para.bm);
  }
  convert_bpf_pgm_to_superopt_pgm(bm, inst::max_prog_len);
  if (! in_para.is_win) {
    int num_examples = 30;
    gen_random_input(inputs, num_examples, -50, 50);
  }
  SERVER_PORT = in_para.server_port;
  logger.set_least_print_level(in_para.logger_level);
  if (in_para.win_s_list.size() == 0) {
    generate_wins(in_para.win_s_list, in_para.win_e_list);
  }

  top_k_progs topk_progs(in_para.k);
  run_mh_sampler(topk_progs, in_para, bm_optis_orig);
  if (in_para.bm_from_file) {
    delete[] bm;
  }
  auto end = NOW;

  cout << "Best program(s): " << endl;
  topk_progs.sort();
  // rbegin() returns to the last value of map
  for (int i = 0; i < topk_progs.progs.size(); i++) {
    prog* p = topk_progs.progs[i];
    cout << "program " << i  << " cost: " << p->_error_cost << " " << p->_perf_cost << endl;
    write_optimized_prog_to_file(p, i, in_para.path_out);
  }

  cout << "validator time: " << dur_sum << endl;
  if (n_sum_long > 0)
    cout << "validator long time: " << dur_sum_long << " " << n_sum_long << " " << dur_sum_long / n_sum_long << endl;
  else
    cout << "validator long time: " << dur_sum_long << endl;
  cout << "compiling time: " << DUR(start, end) << " us" << endl;

  // kill z3 solver server after compiling
  kill_server();
  std::cout.rdbuf(coutbuf); //reset to standard output again
  return 0;
}


