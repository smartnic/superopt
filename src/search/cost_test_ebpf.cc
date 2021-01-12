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
  int num_examples = 1;
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

  print_test_res(get_error_cost(p1, p2, win_start, win_end) == 0, "1");
  mem_t::_layout.clear();
}

int main() {
  test1();
  kill_server();
  return 0;
}
