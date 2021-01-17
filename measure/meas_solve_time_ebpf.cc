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

void meas_solve_time_delta(inst* p, inst* delta, int start, int len, validator& vld) {
  // Generate a new program according to the program p and delta program
  inst p_new[inst::max_prog_len];
  for (int i = 0; i < inst::max_prog_len; i++) {
    p_new[i] = p[i];
  }
  for (int i = 0; i < len; i++) {
    p_new[i + start] = delta[i];
  }

  // measure the solving time of equivalence check formula
  vld.is_equal_to(p, inst::max_prog_len, p_new, inst::max_prog_len);
}

void meas_solve_time_delta_n_times(inst* p, inst* delta, int start, int len,
                                   string test_name, validator& vld) {
  cout << "starting " << test_name << endl;
  for (int i = 0; i < loop_times; i++) {
    meas_solve_time_delta(p, delta, start, len, vld);
  }
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
  // Set rcv_sock4 as the original program in the validator
  validator vld(rcv_sock4, inst::max_prog_len);
  inst p1[] = {inst(MOV32XC, 9, 32),
               inst(MOV32XC, 8, -4),
               inst(STW, 10, -28, 4),
               inst(ARSH64XC, 8, 58),
              };
  meas_solve_time_delta_n_times(rcv_sock4, p1, 12, 4, "p1", vld);

  inst p2[] = {inst(NOP),
               INSN_LDMAPID(8, 0),
               inst(STXH, 10, -26, 8),
               inst(JGTXC, 2, -5, 7),
              };
  meas_solve_time_delta_n_times(rcv_sock4, p2, 12, 4, "p2", vld);
}

int main(int argc, char* argv[]) {
  if (argc > 1) {
    loop_times = atoi(argv[1]);
  }
  meas_solve_time_of_rcv_sock4();
}
