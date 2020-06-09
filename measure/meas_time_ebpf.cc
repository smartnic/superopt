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

default_random_engine gen_mt;
uniform_real_distribution<double> unidist_mt(0.0, 1.0);

double gen_random(double start, double end) {
  return unidist_mt(gen_mt) * (double)(end - start);
}

#define measure_print(print, loop_times, t1, t2) \
cout << print << DUR(t1, t2) / loop_times << " us" << endl;

#define time_measure(func_called, times, print) \
int loop_times = times;                         \
auto start = NOW;                               \
for (int i = 0; i < loop_times; i++) {          \
  func_called;                                  \
}                                               \
auto end = NOW;                                 \
measure_print(print, times, start, end);

void time_interpret() {
  prog_state ps;
  ps.init();
  inout_t in, out;
  in.init();
  out.init();
  in.reg = gen_random((uint64_t)0xffffffffffffffff, 0);
  prog p(bm0);
  time_measure(p.interpret(out, ps, in), 10000,
               "interpret: ");
}

void time_smt_prog() {
  smt_prog sp;
  time_measure(sp.gen_smt(0, bm3, inst::max_prog_len), 10,
               "smt prog::gen_smt: ");
}

void time_get_output_pc_mem() {
  smt_prog sp;
  sp.gen_smt(0, bm3, inst::max_prog_len);
  vector<z3::expr> pc_list;
  vector<smt_var> sv_list;
  time_measure(sp.get_output_pc_mem(pc_list, sv_list), 10,
               "smt prog::get_output_pc_mem: ");
}

void time_set_the_same_input() {
  smt_prog sp1, sp2;
  vector<z3::expr> pc_list1, pc_list2;
  vector<smt_var> sv_list1, sv_list2;
  sp1.gen_smt(0, bm3, inst::max_prog_len);
  sp1.get_output_pc_mem(pc_list1, sv_list1);
  sp2.gen_smt(0, bm3, inst::max_prog_len);
  sp2.get_output_pc_mem(pc_list2, sv_list2);
  cout << "time for min table size, idx: 49" << endl;
  {
    time_measure(smt_map_set_same_input(sv_list1[49], sv_list2[49]), 10,
                 "set same input of map of one path condition pair: ");
  }
  {
    time_measure(smt_pkt_set_same_input(sv_list1[49], sv_list2[49]), 10,
                 "set same input of pkt of one path condition pair: ");
  }
  {
    time_measure(smt_map_eq_chk(sv_list1[49], sv_list2[49]), 10,
                 "equivalence check of map of one path condition pair: ");
  }
  {
    time_measure(smt_pkt_eq_chk(sv_list1[49], sv_list2[49]), 10,
                 "equivalence check of pkt of one path condition pair: ");
  }

  cout << "time for max table size, idx: 20" << endl;
  {
    time_measure(smt_map_set_same_input(sv_list1[20], sv_list2[20]), 10,
                 "set same input of map of one path condition pair: ");
  }
  {
    time_measure(smt_pkt_set_same_input(sv_list1[20], sv_list2[20]), 10,
                 "set same input of pkt of one path condition pair: ");
  }
  {
    time_measure(smt_map_eq_chk(sv_list1[20], sv_list2[20]), 10,
                 "equivalence check of map of one path condition pair: ");
  }
  {
    time_measure(smt_pkt_eq_chk(sv_list1[20], sv_list2[20]), 10,
                 "equivalence check of pkt of one path condition pair: ");
  }
  cout << "table size info for path conditions" << endl;
}

int main() {
  inst::max_prog_len = N3;
  inst::add_sample_imm(vector<int32_t> {264});
  mem_t::set_pkt_sz(128);
  mem_t::add_map(map_attr(128, 64, N3));
  mem_t::add_map(map_attr(96, 96, N3));
  mem_t::add_map(map_attr(64, 128, N3));

  time_interpret();
  time_smt_prog();
  time_get_output_pc_mem();
  time_set_the_same_input();
  return 0;
}
