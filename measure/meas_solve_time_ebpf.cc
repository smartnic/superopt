#include <iostream>
#include <unordered_map>
#include <unordered_set>
#include <chrono>
#include "../src/utils.h"
#include "../src/inout.h"
#include "../src/isa/inst_header.h"
#include "../src/isa/prog.h"
#include "../src/verify/smt_prog.h"
#include "../src/verify/validator.h"
#include "../src/search/mh_prog.h"
#include "benchmark_ebpf.h"
#include "z3++.h"

using namespace std;

int loop_times = 1;
string path_prefix = "../superopt-input-bm/input_bm_0108_ab7e41e/";

void set_up_enviornment() {
  mem_t::_layout.clear();
  smt_var::is_win = true; // aviod print in init_benchmarks()
}

void meas_solve_time_delta(inst* p, inst* p_new, validator& vld) {
  // measure the solving time of equivalence check formula
  auto t1 = NOW;
  for (int i = 0; i < loop_times; i++) {
    vld.is_equal_to(p, inst::max_prog_len, p_new, inst::max_prog_len);
  }
  auto t2 = NOW;
  cout << "is_equal_to: " << (DUR(t1, t2) / loop_times) << " us" << endl;
}

// win program + off-based mutilple memory tables + multiple map tables
void measure_win_prog_off_based_multi_table(inst* p, inst* p_new, int win_start, int win_end) {
  cout << "win program + off-based mutilple memory tables + multiple map tables......" << endl;
  validator::enable_z3server = false;
  smt_var::enable_multi_map_tables = true;
  smt_var::enable_multi_mem_tables = true;
  bool enable_win = true;
  smt_var::enable_addr_off = true;
  validator vld(p, inst::max_prog_len, enable_win, win_start, win_end);
  vld._enable_prog_eq_cache = false;
  meas_solve_time_delta(p, p_new, vld);
}

// full program + off-based multiple memory tables + multiple map tables
void measure_full_prog_off_based_multi_table(inst* p, inst* p_new) {
  cout << "full program + off-based multiple memory tables + multiple map tables......" << endl;
  validator::enable_z3server = false;
  smt_var::enable_multi_map_tables = true;
  smt_var::enable_multi_mem_tables = true;
  smt_var::enable_addr_off = true;
  smt_var::is_win = false;
  validator vld(p, inst::max_prog_len);
  vld._enable_prog_eq_cache = false;
  meas_solve_time_delta(p, p_new, vld);
}

// full program + addr-based multiple memory tables + multiple map tables
void measure_full_prog_addr_based_multi_table(inst* p, inst* p_new) {
  cout << "full program + addr-based multiple memory tables + multiple map tables......" << endl;
  validator::enable_z3server = false;
  smt_var::enable_multi_map_tables = true;
  smt_var::enable_multi_mem_tables = true;
  smt_var::enable_addr_off = false;
  smt_var::is_win = false;
  validator vld(p, inst::max_prog_len);
  vld._enable_prog_eq_cache = false;
  meas_solve_time_delta(p, p_new, vld);
  smt_var::enable_addr_off = true;
}

// full program + addr-based multiple memory tables + single map table
void measure_full_prog_addr_based_single_map_table(inst* p, inst* p_new) {
  cout << "full program + addr-based multiple memory tables + single map table......" << endl;
  validator::enable_z3server = false;
  smt_var::enable_multi_map_tables = false;
  smt_var::enable_multi_mem_tables = true;
  smt_var::enable_addr_off = false;
  smt_var::is_win = false;
  validator vld(p, inst::max_prog_len);
  vld._enable_prog_eq_cache = false;
  meas_solve_time_delta(p, p_new, vld);
  smt_var::enable_addr_off = true;
}

void measure_full_prog_addr_based_single_mem_table(inst* p, inst* p_new) {
  cout << "full program + addr-based single memory table + multiple map table......" << endl;
  validator::enable_z3server = false;
  smt_var::enable_multi_map_tables = true;
  smt_var::enable_multi_mem_tables = false;
  smt_var::enable_addr_off = false;
  smt_var::is_win = false;
  validator vld(p, inst::max_prog_len);
  vld._enable_prog_eq_cache = false;
  meas_solve_time_delta(p, p_new, vld);
  smt_var::enable_addr_off = true;
}

void measure_full_prog_addr_based_single_mem_single_map_table(inst* p, inst* p_new) {
  cout << "full program + addr-based single memory table + single map table......" << endl;
  validator::enable_z3server = false;
  smt_var::enable_multi_map_tables = false;
  smt_var::enable_multi_mem_tables = false;
  smt_var::enable_addr_off = false;
  smt_var::is_win = false;
  validator vld(p, inst::max_prog_len);
  vld._enable_prog_eq_cache = false;
  meas_solve_time_delta(p, p_new, vld);
  smt_var::enable_addr_off = true;
}

void meas_solve_time_delta_n_times(inst* p, inst* delta, int start, int end,
                                   string test_name,
                                   bool test1 = true,
                                   bool test2 = true,
                                   bool test3 = true,
                                   bool test4 = true,
                                   bool test5 = true,
                                   bool test6 = true) {
  cout << "starting " << test_name << " " << start << "," << end << " " << (end - start + 1) << endl;

  // Generate a new program according to the program p and delta program
  inst p_new[inst::max_prog_len];
  for (int i = 0; i < inst::max_prog_len; i++) {
    p_new[i] = p[i];
  }
  for (int i = 0; i < end - start + 1; i++) {
    p_new[i + start] = delta[i];
  }
  if (test1) measure_win_prog_off_based_multi_table(p, p_new, start, end);
  if (test2) measure_full_prog_off_based_multi_table(p, p_new);
  if (test3) measure_full_prog_addr_based_multi_table(p, p_new);
  if (test4) measure_full_prog_addr_based_single_map_table(p, p_new);
  if (test5) measure_full_prog_addr_based_single_mem_table(p, p_new);
  if (test6) measure_full_prog_addr_based_single_mem_single_map_table(p, p_new);
}

void meas_solve_time_of_cilium_recvmsg4() {
  // Init program and static variables
  set_up_enviornment();
  inst* bm;
  vector<inst*> optis_progs;
  string bm_name = path_prefix + "cilium/bpf_sock_recvmsg4";
  init_benchmark_from_file(&bm, (bm_name + ".insns").c_str(),
                           (bm_name + ".maps").c_str(), (bm_name + ".desc").c_str());

  cout << "benchmark: cilium, recvmsg4" << endl;
  inst p1[] = {inst(LDXW, 1, 6, 24),
               inst(),
               inst(STXH, 10, -26, 7),
               inst(),
               inst(STXH, 10, -28, 1),
              };
  meas_solve_time_delta_n_times(bm, p1, 11, 15, "p1");
}

void meas_solve_time_of_cilium_from_network() {
  set_up_enviornment();
  inst* bm;
  vector<inst*> optis_progs;
  string bm_name = path_prefix + "cilium/bpf_network_from-network";
  init_benchmark_from_file(&bm, (bm_name + ".insns").c_str(),
                           (bm_name + ".maps").c_str(), (bm_name + ".desc").c_str());

  cout << "benchmark: cilium, from_network" << endl;
  inst p1[] = {inst(STDW, 10, -24, 259),
               inst(),
               inst(),
               inst(),
               inst(),
              };
  meas_solve_time_delta_n_times(bm, p1, 2, 6, "p1");

  inst p2[] = {inst(MOV64XC, 1, 0),
               inst(STXDW, 6, 48, 1),
               inst(STXDW, 6, 56, 1),
               inst(STXW, 6, 64, 1),
               inst(),
               inst(),
              };
  meas_solve_time_delta_n_times(bm, p2, 31, 36, "p2");
}

void meas_solve_time_of_katran_xdp_balancer() {
  set_up_enviornment();
  inst* bm;
  vector<inst*> optis_progs;
  string bm_name = path_prefix + "katran/balancer_kern_xdp-balancer";
  init_benchmark_from_file(&bm, (bm_name + ".insns").c_str(),
                           (bm_name + ".maps").c_str(), (bm_name + ".desc").c_str());

  cout << "benchmark: katran, xdp-balancer" << endl;
  inst p1[] = {inst(),
               inst(STB, 9, 22, 64),
              };
  meas_solve_time_delta_n_times(bm, p1, 47, 48, "p1", true, false, false, false, false, false);

  inst p2[] = {inst(),
               inst(STB, 9, 54, 129),
               inst(),
               inst(STB, 9, 21, 64),
              };
  meas_solve_time_delta_n_times(bm, p2, 136, 139, "p2", true, false, false, false, false, false);

  inst p3[] = {inst(LDXW, 1, 9, 0),
               inst(STXW, 9, 6, 1),
               inst(LDXH, 1, 9, 4),
               inst(STXH, 9, 10, 1),
               inst(),
               inst(),
              };
  meas_solve_time_delta_n_times(bm, p3, 185, 190, "p3", true, false, false, false, false, false);
}

void meas_solve_time_of_xdp_exception() {
  set_up_enviornment();
  inst* bm;
  vector<inst*> optis_progs;
  init_benchmarks(&bm, optis_progs, 16);

  cout << "benchmark: xdp_monitor_kern, xdp_exception" << endl;
  inst p1[] = {inst(MOV64XC, 1, 1),
               inst(XADD64, 0, 0, 1),
               inst(),
              };
  meas_solve_time_delta_n_times(bm, p1, 12, 14, "p1");
}

void meas_solve_time_of_xdp_redirect_err() {
  set_up_enviornment();
  inst* bm;
  vector<inst*> optis_progs;
  init_benchmarks(&bm, optis_progs, 15);

  cout << "benchmark: xdp_monitor_kern, xdp_redirect_err" << endl;
  inst p1[] = {inst(MOV64XC, 1, 1),
               inst(XADD64, 0, 0, 1),
               inst(),
              };
  meas_solve_time_delta_n_times(bm, p1, 12, 14, "p1");
}

void meas_solve_time_of_xdp_devmap_xmit() {
  set_up_enviornment();
  inst* bm;
  vector<inst*> optis_progs;
  init_benchmarks(&bm, optis_progs, 13);

  cout << "benchmark: xdp_monitor_kern, xdp_devmap_xmit" << endl;
  inst p1[] = {inst(LDXW, 1, 6, 20),
               inst(MOV64XC, 2, 1),
               inst(XADD64, 0, 16, 2),
               inst(),
              };
  meas_solve_time_delta_n_times(bm, p1, 15, 18, "p1");
}

void meas_solve_time_of_xdp_cpumap_kthread() {
  set_up_enviornment();
  inst* bm;
  vector<inst*> optis_progs;
  init_benchmarks(&bm, optis_progs, 14);

  cout << "benchmark: xdp_monitor_kern, xdp_cpumap_kthread" << endl;
  inst p1[] = {inst(STW, 10, -4, 0),
               inst(),
              };
  meas_solve_time_delta_n_times(bm, p1, 1, 2, "p1");

  inst p2[] = {inst(LDXW, 1, 6, 24),
               inst(),
               inst(XADD64, 0, 0, 1),
               inst(),
              };
  meas_solve_time_delta_n_times(bm, p2, 9, 12, "p2");
}

void meas_solve_time_of_xdp_cpumap_enqueue() {
  set_up_enviornment();
  inst* bm;
  vector<inst*> optis_progs;
  init_benchmarks(&bm, optis_progs, 17);

  cout << "benchmark: xdp_monitor_kern, xdp_cpumap_enqueue" << endl;
  inst p1[] = {inst(XADD64, 1, 0, 2),
               inst(),
               inst(),
              };
  meas_solve_time_delta_n_times(bm, p1, 14, 16, "p1");
}

int main(int argc, char* argv[]) {
  if (argc > 1) {
    loop_times = atoi(argv[1]);
  }
  logger.set_least_print_level(LOGGER_DEBUG);
  meas_solve_time_of_xdp_exception();
  meas_solve_time_of_xdp_redirect_err();
  meas_solve_time_of_xdp_devmap_xmit();
  meas_solve_time_of_xdp_cpumap_kthread();
  meas_solve_time_of_xdp_cpumap_enqueue();
  meas_solve_time_of_cilium_from_network();
  meas_solve_time_of_cilium_recvmsg4();
  meas_solve_time_of_katran_xdp_balancer();
  return 0;
}
