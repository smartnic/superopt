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

void meas_solve_time_delta(inst* p, inst* p_new, validator& vld) {
  // measure the solving time of equivalence check formula
  auto t1 = NOW;
  for (int i = 0; i < loop_times; i++) {
    vld.is_equal_to(p, inst::max_prog_len, p_new, inst::max_prog_len);
  }
  auto t2 = NOW;
  cout << "is_equal_to: " << (DUR(t1, t2) / loop_times) << " us" << endl;
}

// win prpgram + off-based mutilple memory tables
void measure_win_prog_off_based_multi_table(inst* p, inst* p_new, int win_start, int len) {
  cout << "win prpgram + off-based mutilple memory tables......" << endl;
  validator::enable_z3server = false;
  bool enable_win = true;
  smt_var::enable_addr_off = true;
  int win_end = win_start + len - 1;
  validator vld(p, inst::max_prog_len, enable_win, win_start, win_end);
  vld._enable_prog_eq_cache = false;
  meas_solve_time_delta(p, p_new, vld);
}

// full program + off-based multiple memory tables
void measure_full_prog_off_based_multi_table(inst* p, inst* p_new) {
  cout << "full program + off-based multiple memory tables......" << endl;
  validator::enable_z3server = false;
  smt_var::enable_addr_off = true;
  validator vld(p, inst::max_prog_len);
  vld._enable_prog_eq_cache = false;
  meas_solve_time_delta(p, p_new, vld);
}

// full program + addr-based multiple memory tables
void measure_full_prog_addr_based_multi_table(inst* p, inst* p_new) {
  validator::enable_z3server = false;
  smt_var::enable_addr_off = false;
  validator vld(p, inst::max_prog_len);
  vld._enable_prog_eq_cache = false;
  meas_solve_time_delta(p, p_new, vld);
  smt_var::enable_addr_off = true;
}

void meas_solve_time_delta_n_times(inst* p, inst* delta, int start, int len,
                                   string test_name) {
  cout << "starting " << test_name << endl;

  // Generate a new program according to the program p and delta program
  inst p_new[inst::max_prog_len];
  for (int i = 0; i < inst::max_prog_len; i++) {
    p_new[i] = p[i];
  }
  for (int i = 0; i < len; i++) {
    p_new[i + start] = delta[i];
  }
  measure_win_prog_off_based_multi_table(p, p_new, start, len);
  measure_full_prog_off_based_multi_table(p, p_new);
  measure_full_prog_addr_based_multi_table(p, p_new);
}

void meas_solve_time_of_rcv_sock4() {
  cout << "Original program is rcv-sock4" << endl;
  // Init program and static variables
  inst::max_prog_len = N3;
  inst rcv_sock4[inst::max_prog_len];
  for (int i = 0; i < inst::max_prog_len; i++) {
    rcv_sock4[i] = bm3[i];
  }
  mem_t::_layout.clear();
  mem_t::set_pgm_input_type(PGM_INPUT_pkt);
  mem_t::set_pkt_sz(128);
  mem_t::add_map(map_attr(128, 64, inst::max_prog_len));
  mem_t::add_map(map_attr(96, 96, inst::max_prog_len));
  mem_t::add_map(map_attr(64, 128, inst::max_prog_len));
  unsigned int n_randoms_u32 = 1;
  mem_t::_layout._n_randoms_u32 = n_randoms_u32;
  smt_var::init_static_variables();

  inst p1[] = {inst(LDXW, 1, 6, 24),
               inst(),
               inst(MOV32XC, 8, 0),
               inst(STXH, 10, -26, 8),
               inst(),
              };

  meas_solve_time_delta_n_times(rcv_sock4, p1, 11, 5, "p1");
}

int main(int argc, char* argv[]) {
  if (argc > 1) {
    loop_times = atoi(argv[1]);
  }
  logger.set_least_print_level(LOGGER_DEBUG);
  meas_solve_time_of_rcv_sock4();
  kill_server();
}
