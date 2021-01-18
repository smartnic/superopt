#include <iostream>
#include "../../src/utils.h"
#include "../../measure/benchmark_ebpf.h"
#include "cost.h"

using namespace std;

double get_error_cost(inst* p1, inst* p2, int win_start, int win_end) {
  cost c;
  c._vld._is_win = true;
  smt_var::is_win = true;
  prog prog1(p1), prog2(p2);

  inout_t::start_insn = win_start;
  inout_t::end_insn = win_end;
  static_safety_check_pgm(prog1.inst_list, inst::max_prog_len);
  c.set_orig(&prog1, inst::max_prog_len, win_start, win_end);
  prog_static_state pss;
  static_analysis(pss, p1, inst::max_prog_len);
  int num_examples = 30;
  vector<inout_t> examples;
  gen_random_input_for_win(examples, num_examples, pss.static_state[win_start], win_start, win_end);
  c.set_examples(examples, &prog1);
  return c.error_cost(&prog1, inst::max_prog_len, &prog2, inst::max_prog_len);
}

void test1() {
  cout << "Test1: test error cost" << endl;
  inst p1[N3], p2[N3];
  for (int i = 0; i < N3; i++) p1[i] = bm3[i];
  for (int i = 0; i < N3; i++) p2[i] = bm3[i];
  p2[5] = inst(MOV32XY, 1, 0);
  p2[6] = inst();
  p2[7] = inst();
  int win_start = 5, win_end = 7;
  mem_t::_layout.clear();
  inst::max_prog_len = N3;
  mem_t::set_pgm_input_type(PGM_INPUT_pkt);
  mem_t::set_pkt_sz(128);
  mem_t::add_map(map_attr(128, 64, 91));
  mem_t::add_map(map_attr(96, 96, 91));
  mem_t::add_map(map_attr(64, 128, 91));
  mem_t::_layout._n_randoms_u32 = 1;
  smt_var::init_static_variables();

  print_test_res(get_error_cost(p1, p2, win_start, win_end) == 0, "rcv_sock4 1");
  mem_t::_layout.clear();

  // xdp_exception
  const int xdp_exp_len = N16;
  inst::max_prog_len = xdp_exp_len;
  mem_t::set_pgm_input_type(PGM_INPUT_pkt);
  mem_t::set_pkt_sz(32);
  mem_t::add_map(map_attr(32, 64, N16));
  inst xdp_exp[xdp_exp_len];
  inst xdp_exp_1[xdp_exp_len];
  for (int i = 0; i < xdp_exp_len; i++) xdp_exp[i] = bm16[i];
  for (int i = 0; i < xdp_exp_len; i++) xdp_exp_1[i] = xdp_exp[i];
  win_start = 12;
  win_end = 14;
  xdp_exp_1[12] = inst();
  xdp_exp_1[13] = inst();
  xdp_exp_1[14] = inst(XADD64, 0, 0, 1);
  print_test_res(get_error_cost(xdp_exp, xdp_exp_1, win_start, win_end) == 0, "xdp_exception 1");
  mem_t::_layout.clear();

  // xdp_pktcntr, bm24
  const int xdp_pkt_len = N24;
  inst::max_prog_len = xdp_pkt_len;
  mem_t::set_pgm_input_type(PGM_INPUT_pkt);
  mem_t::set_pkt_sz(68);
  mem_t::add_map(map_attr(32, 32, N24));
  mem_t::add_map(map_attr(32, 64, N24));
  inst xdp_pkt[xdp_pkt_len];
  inst xdp_pkt_1[xdp_pkt_len];
  for (int i = 0; i < xdp_pkt_len; i++) xdp_pkt[i] = bm24[i];
  for (int i = 0; i < xdp_pkt_len; i++) xdp_pkt_1[i] = xdp_pkt[i];
  win_start = 17;
  win_end = 19;
  xdp_pkt_1[17] = inst();
  xdp_pkt_1[18] = inst(MOV32XC, 1, 1);
  xdp_pkt_1[19] = inst(XADD64, 0, 0, 1);
  print_test_res(get_error_cost(xdp_pkt, xdp_pkt_1, win_start, win_end) == 0, "xdp_pktcntr 1");
  mem_t::_layout.clear();

  // test PGM_INPUT_pkt_ptrs
  const int p3_len = 8;
  inst::max_prog_len = p3_len;
  mem_t::set_pgm_input_type(PGM_INPUT_pkt_ptrs);
  mem_t::set_pkt_sz(32);
  mem_t::add_map(map_attr(32, 64, p3_len));
  inst p3[] = {inst(LDXW, 3, 1, 4),
               inst(LDXW, 2, 1, 0),
               inst(LDXB, 4, 2, 12), // insn 2
               inst(LDXB, 5, 2, 13),
               inst(LSH64XC, 5, 8),
               inst(OR64XY, 5, 4),   // insn 5
               inst(MOV64XY, 0, 5),
               inst(EXIT),
              };
  inst p3_1[] = {inst(LDXW, 3, 1, 4),
                 inst(LDXW, 2, 1, 0),
                 inst(LDXB, 4, 2, 12),
                 inst(LDXB, 5, 2, 13),
                 inst(LSH64XC, 5, 8),
                 inst(LDXH, 5, 2, 12),
                 inst(MOV64XY, 0, 5),
                 inst(EXIT),
                };
  win_start = 2;
  win_end = 5;
  print_test_res(get_error_cost(p3, p3_1, win_start, win_end) == 0, "PGM_INPUT_pkt_ptrs");
  mem_t::_layout.clear();
}

int main() {
  test1();
  kill_server();
  return 0;
}
