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
  // This is to infer program state while entering and leaving the window.
  static_analysis(pss, p1, inst::max_prog_len);
  int num_examples = 30;
  // storing all the initial test cases.
  vector<inout_t> examples;
  gen_random_input_for_win(examples, num_examples,
                           pss.static_state[win_start], p1[win_start],
                           win_start, win_end);
  c.set_examples(examples, &prog1);
  return c.error_cost(&prog1, inst::max_prog_len, &prog2, inst::max_prog_len);
}

double get_error_cost_repair(inst* p1, inst* p2, int win_start, int win_end) {
  cost c;
  c._vld._is_win = true;
  smt_var::is_win = true;
  prog prog1(p1), prog2(p2);

  inout_t::start_insn = win_start;
  inout_t::end_insn = win_end;

  static_safety_check_pgm(prog1.inst_list, inst::max_prog_len);

  c.set_orig(&prog1, inst::max_prog_len, win_start, win_end);
  prog_static_state pss;
  // This is to infer program state while entering and leaving the window.
  static_analysis(pss, p1, inst::max_prog_len);

  int num_examples = 30;
  // storing all the initial test cases.
  vector<inout_t> examples;
  gen_random_input_for_win(examples, num_examples,
                           pss.static_state[win_start], p1[win_start],
                           win_start, win_end);
  c.set_examples(examples, &prog1);

  return c.error_cost_repair(&prog1, inst::max_prog_len, &prog2, inst::max_prog_len);

}

double get_safety_cost_repair(inst* p1, inst* p2, int win_start, int win_end) {
  cost c;
  c._vld._is_win = true;
  smt_var::is_win = true;
  prog prog1(p1), prog2(p2);

  inout_t::start_insn = win_start;
  inout_t::end_insn = win_end;

  static_safety_check_pgm(prog1.inst_list, inst::max_prog_len);

  c.set_orig(&prog1, inst::max_prog_len, win_start, win_end);
  prog_static_state pss;
  // This is to infer program state while entering and leaving the window.
  static_analysis(pss, p1, inst::max_prog_len);

  int num_examples = 30;
  // storing all the initial test cases.
  vector<inout_t> examples;
  gen_random_input_for_win(examples, num_examples,
                           pss.static_state[win_start], p1[win_start],
                           win_start, win_end);
  c.set_examples(examples, &prog1);

  return c.safety_cost_repair(&prog1, inst::max_prog_len, &prog2, inst::max_prog_len);

}

void test1() {
  cout << "Test1: test error cost" << endl;
  inst p1[N3], p2[N3];
  // The bm3 program is there in benchmark_ebf.cc under the measure folder
  for (int i = 0; i < N3; i++) p1[i] = bm3[i];
  for (int i = 0; i < N3; i++) p2[i] = bm3[i];
  p2[5] = inst(MOV32XY, 1, 0); // changing the 6th instruction
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
  // The expected error cost is 0 because in this window interval we expect the
  // 2 programs to be semantically equivalent.
  // Making sure expected error cost is same as the result
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
  const int p3_len = 11;
  inst::max_prog_len = p3_len;
  mem_t::set_pgm_input_type(PGM_INPUT_pkt_ptrs);
  mem_t::set_pkt_sz(32);
  mem_t::add_map(map_attr(32, 64, p3_len));
  inst p3[] = {inst(LDXW, 3, 1, 4),
               inst(LDXW, 2, 1, 0),
               inst(MOV64XY, 6, 2),
               inst(ADD64XC, 6, 14),
               inst(JGTXY, 6, 3, 5),
               inst(LDXB, 4, 2, 12), // insn 5
               inst(LDXB, 5, 2, 13),
               inst(LSH64XC, 5, 8),
               inst(OR64XY, 5, 4),   // insn 8
               inst(MOV64XY, 0, 5),
               inst(EXIT),
              };
  inst p3_1[] = {inst(LDXW, 3, 1, 4),
                 inst(LDXW, 2, 1, 0),
                 inst(MOV64XY, 6, 2),
                 inst(ADD64XC, 6, 14),
                 inst(JGTXY, 6, 3, 5),
                 inst(LDXB, 4, 2, 12),
                 inst(LDXB, 5, 2, 13),
                 inst(LSH64XC, 5, 8),
                 inst(LDXH, 5, 2, 12),
                 inst(MOV64XY, 0, 5),
                 inst(EXIT),
                };
  win_start = 5;
  win_end = 8;
  try {
    print_test_res(get_error_cost(p3, p3_1, win_start, win_end) == 0, "PGM_INPUT_pkt_ptrs 1");
  } catch (string err_string) {
    cout << "ERROR: " << err_string << endl;
  }
  mem_t::_layout.clear();


  inst p4[] = {inst(LDXW, 2, 1, 4),
               inst(LDXW, 8, 1, 0),
               inst(MOV64XC, 1, 0),
               inst(STXW, 10, -4, 1),
               inst(STXW, 10, -8, 1),
               inst(MOV64XC, 7, 1),
               inst(MOV64XY, 1, 8),
               inst(ADD64XC, 1, 14),
               inst(JGTXY, 1, 2, 32),
               inst(MOV64XY, 2, 10),
               inst(ADD64XC, 2, -4),
               INSN_LDMAPID(1, 0),
               inst(NOP),
               inst(CALL, 1),
               inst(MOV64XY, 6, 0),
               inst(JEQXC, 6, 0, 25),
               inst(MOV64XY, 2, 10),
               inst(ADD64XC, 2, -8),
               INSN_LDMAPID(1, 1),
               inst(NOP),
               inst(CALL, 1),
               inst(JEQXC, 0, 0, 3),
               inst(LDXDW, 1, 0, 0),
               inst(ADD64XC, 1, 1),
               inst(STXDW, 0, 0, 1),
               inst(LDXH, 1, 8, 0),
               inst(LDXH, 2, 8, 6),
               inst(STXH, 8, 0, 2),
               inst(LDXH, 2, 8, 8),
               inst(LDXH, 3, 8, 2),
               inst(STXH, 8, 8, 3),
               inst(STXH, 8, 2, 2),
               inst(LDXH, 2, 8, 10),
               inst(LDXH, 3, 8, 4),
               inst(STXH, 8, 10, 3),
               inst(STXH, 8, 6, 1),
               inst(STXH, 8, 4, 2),
               inst(LDXW, 1, 6, 0),
               inst(MOV64XC, 2, 0),
               inst(CALL, 23),
               inst(MOV64XY, 7, 0),
               inst(MOV64XY, 0, 7),
               inst(EXIT),
              };

  const int p4_len = sizeof(p4) / sizeof(inst);
  inst::max_prog_len = p4_len;
  mem_t::set_pgm_input_type(PGM_INPUT_pkt_ptrs);
  mem_t::set_pkt_sz(64);
  mem_t::add_map(map_attr(32, 32, p4_len));
  mem_t::add_map(map_attr(32, 64, p4_len));
  inst p4_1[p4_len];
  for (int i = 0; i < p4_len; i++) p4_1[i] = p4[i];
  win_start = 2;
  win_end = 4;
  p4_1[2] = inst();
  p4_1[3] = inst();
  p4_1[4] = inst(STDW, 10, -8, 0);
  print_test_res(get_error_cost(p4, p4_1, win_start, win_end) == 0, "PGM_INPUT_pkt_ptrs 2");
  mem_t::_layout.clear();

  inst p5[] = {inst(MOV64XY, 6, 1),
               inst(MOV64XC, 1, 0),
               inst(CALL, BPF_FUNC_get_prandom_u32),
               inst(MOV64XC, 2, 0),
               inst(STXDW, 10, -8, 2),
               inst(LDXDW, 0, 10, -8),
               inst(EXIT),
              };
  inst p5_1[] = {inst(MOV64XY, 6, 1),
                 inst(MOV64XC, 1, 0),
                 inst(CALL, BPF_FUNC_get_prandom_u32),
                 inst(MOV64XC, 2, 0),
                 inst(STXDW, 10, -8, 1), // src_reg from 2 to 1
                 inst(LDXDW, 0, 10, -8),
                 inst(EXIT),
                };
  const int p5_len = sizeof(p5) / sizeof(inst);
  inst::max_prog_len = p5_len;
  mem_t::set_pgm_input_type(PGM_INPUT_pkt);
  mem_t::set_pkt_sz(32);
  win_start = 3;
  win_end = 5;
  double err =  get_error_cost(p5, p5_1, win_start, win_end);
  print_test_res(get_error_cost(p5, p5_1, win_start, win_end) == ERROR_COST_MAX, "p5");

  mem_t::_layout.clear();
}

void test2() {

  // Two safe programs that are semantically equivalent output a 0 error cost and safety cost

  // *(u32 *)pkt = 0
  // r1 is pkt start address
  inst p1[] = {inst(MOV64XC, 0, 0),
               inst(STXB, 1, 0, 0), // *(u8 *)(pkt + 0) = r0
               inst(STXB, 1, 1, 0), // *(u8 *)(pkt + 1) = r0
               inst(STXB, 1, 2, 0), // *(u8 *)(pkt + 2) = r0
               inst(STXB, 1, 3, 0), // *(u8 *)(pkt + 3) = r0
               inst(),
               inst(),
              };
  // optimization: changing 4 bytes at once
  inst p2[] = {inst(MOV64XC, 0, 0),
               inst(STXW, 1, 0, 0), // *(u32 *)(pkt + 0) = r0
               inst(),
               inst(),
               inst(),
               inst(),
               inst(),
              };

  int win_start = 0, win_end = 4;

  mem_t::_layout.clear();
  // use sizeof(program)/sizeof(ins) for below
  inst::max_prog_len = sizeof(p1) / sizeof(inst);

  // inst_var.h (line 46): PGM_INPUT types for different kinds of program inputs
  mem_t::set_pgm_input_type(PGM_INPUT_pkt);
  // we don't use packet in the above programs
  mem_t::set_pkt_sz(5);
  mem_t::_layout._n_randoms_u32 = 1;
  smt_var::init_static_variables();


  print_test_res(get_error_cost_repair(p1, p2, win_start, win_end) == 0, "error_cost_repair: 2 safe and equivalent programs ");
  print_test_res(get_safety_cost_repair(p1, p2, win_start, win_end) == 0, "safety_cost_repair: 2 safe and equivalent programs ");
  mem_t::_layout.clear();


}

void test3() {

  // 2 safe programs that are not equivalent should have error cost > 0 and safety cost = 0

  // *(u32 *)pkt = 0
  // r1 is pkt start address
  inst p1[] = { inst(MOV64XC, 0, 0),
                inst(STXB, 1, 0, 0), // *(u8 *)(pkt + 0) = r0
                inst(STXB, 1, 1, 0), // *(u8 *)(pkt + 1) = r0
                inst(STXB, 1, 2, 0), // *(u8 *)(pkt + 2) = r0
                inst(STXB, 1, 3, 0), // *(u8 *)(pkt + 3) = r0
                inst(),
                inst(),
              };


  inst p2[] = { inst(MOV64XC, 0, 1), //Moving 1 into register r0
                inst(STXB, 1, 0, 0), // *(u8 *)(pkt + 0) = r0
                inst(STXB, 1, 1, 0), // *(u8 *)(pkt + 1) = r0
                inst(STXB, 1, 2, 0), // *(u8 *)(pkt + 2) = r0
                inst(STXB, 1, 3, 0), // *(u8 *)(pkt + 3) = r0
                inst(),
                inst(),
              };



  int win_start = 0, win_end = 0;

  mem_t::_layout.clear();
  // use sizeof(program)/sizeof(ins) for below
  inst::max_prog_len = sizeof(p1) / sizeof(inst);

  // inst_var.h (line 46): PGM_INPUT types for different kinds of program inputs
  mem_t::set_pgm_input_type(PGM_INPUT_pkt);
  // we don't use packet in the above programs
  mem_t::set_pkt_sz(5);
  mem_t::_layout._n_randoms_u32 = 1;
  smt_var::init_static_variables();

  print_test_res(get_error_cost_repair(p1, p2, win_start, win_end) > 0, "error_cost_repair: 2 safe and non-equivalent programs ");
  print_test_res(get_safety_cost_repair(p1, p2, win_start, win_end) == 0, "safety_cost_repair: 2 safe and non-equivalent programs ");
  mem_t::_layout.clear();

}

void test4() {

  // orig: safe program
  // synth: unsafe program
  // safety_cost_repair should be greater than 0.

  // *(u32 *)pkt = 0
  // r1 is pkt start address
  inst p1[] = { inst(MOV64XC, 0, 0),
                inst(STXB, 1, 0, 0), // *(u8 *)(pkt + 0) = r0
                inst(STXB, 1, 1, 0), // *(u8 *)(pkt + 1) = r0
                inst(STXB, 1, 2, 0), // *(u8 *)(pkt + 2) = r0
                inst(STXB, 1, 3, 0), // *(u8 *)(pkt + 3) = r0
                inst(),
                inst(),
              };

  inst p2[] = {inst(STXB, 1, 0, 0), // *(u8 *)(pkt + 0) = r0
               inst(STXB, 1, 1, 0), // *(u8 *)(pkt + 1) = r0
               inst(STXB, 1, 2, 0), // *(u8 *)(pkt + 2) = r0
               inst(STXB, 1, 3, 0), // *(u8 *)(pkt + 3) = r0
               inst(MOV64XC, 0, 0),
               inst(),
               inst(),
              };

  int win_start = 0, win_end = 4;

  mem_t::_layout.clear();
  // use sizeof(program)/sizeof(ins) for below
  inst::max_prog_len = sizeof(p1) / sizeof(inst);

  // inst_var.h (line 46): PGM_INPUT types for different kinds of program inputs
  mem_t::set_pgm_input_type(PGM_INPUT_pkt);
  // we don't use packet in the above programs
  mem_t::set_pkt_sz(5);
  mem_t::_layout._n_randoms_u32 = 1;
  smt_var::init_static_variables();

  print_test_res(get_safety_cost_repair(p1, p2, win_start, win_end) > 0, "safety_cost_repair: 2nd program unsafe ");

  mem_t::_layout.clear();


}

int main() {
  test1();
  test2();
  test3();
  test4();
  kill_server();
  return 0;
}
