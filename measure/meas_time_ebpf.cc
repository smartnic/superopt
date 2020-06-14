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

void time_vld_mem_input_output() {
  cout << "SMT of memory input set and memory output equivalence check" << endl;
  smt_prog sp1, sp2;
  sp1.gen_smt(0, bm3, inst::max_prog_len);
  sp2.gen_smt(1, bm3, inst::max_prog_len);
  {
    time_measure(smt_map_set_same_input(sp1.sv, sp2.sv), 10,
                 "set same map input: ");
  }
  {
    time_measure(smt_pkt_set_same_input(sp1.sv, sp2.sv), 10,
                 "set same pkt input: ");
  }
  {
    time_measure(smt_map_eq_chk(sp1.sv, sp2.sv), 10,
                 "map equivalence check: ");
  }
  {
    time_measure(smt_pkt_eq_chk(sp1.sv, sp2.sv), 10,
                 "pkt equivalence check: ");
  }
}

// no mem same input set and output check
z3::expr smt_pgm_register_eq_check(inst* orig, int len1, inst* synth, int len2) {
  z3::expr smt = Z3_true;
  try {
    z3::expr pre_orig = Z3_true, pre_synth = Z3_true;
    validator vld;
    vld.smt_pre(pre_orig, 0, NUM_REGS, orig->get_input_reg());
    vld.smt_pre(pre_synth, 1, NUM_REGS, synth->get_input_reg());

    smt_prog sp1, sp2;
    z3::expr pl_orig = sp1.gen_smt(0, orig, len1);
    z3::expr pl_synth = sp2.gen_smt(1, synth, len2);
    z3::expr post = (string_to_expr("output0") == string_to_expr("output1"));
    smt = z3::implies(pre_orig && pre_synth && pl_orig && pl_synth, post);
  } catch (const string err_msg) {
    cout << err_msg << endl;
  }

  return smt;
}

void time_z3_solver_register() {
  z3::expr pre_orig = Z3_true, pre_synth = Z3_true;
  validator vld;
  vld.smt_pre(pre_orig, 0, NUM_REGS, bm3->get_input_reg());
  vld.smt_pre(pre_synth, 1, NUM_REGS, bm3->get_input_reg());

  smt_prog sp1, sp2;
  z3::expr pl_orig = sp1.gen_smt(0, bm3, inst::max_prog_len);
  z3::expr pl_synth = sp2.gen_smt(1, bm3, inst::max_prog_len);
  z3::expr post = (string_to_expr("output0") == string_to_expr("output1"));
  z3::expr f1 = (pre_orig && pl_orig).simplify() && (pre_synth && pl_synth).simplify();
  z3::expr smt = z3::implies(f1, post);
  time_measure(is_smt_valid(smt), 1, "z3 solver program register equivalence check: ");
}

void set_as_register_pgm(inst* pgm, int len, int type) {
  for (int i = 0; i < len; i++) {
    int opcode = pgm->get_opcode_by_idx(gen_random(IDX_ADD64XC, IDX_ARSH32XY));
    int imm = gen_random(-10, 10);
    int dst_reg = gen_random(0, 2);
    int src_reg = gen_random(0, 2);
    int off = 0;
    pgm[i] = inst(opcode, src_reg, dst_reg, off, imm);
  }
}

void set_as_st_ld_pgm(inst* pgm, int len, int type) {
  if (type <= 2) {  // st, st, ....., st, ld
    inst st_insn, ld_insn;
    if (type == 1) {
      st_insn = inst(STXB, 10, -4, 1);
      ld_insn = inst(LDXB, 0, 10, -4);
    } else if (type == 2) {
      st_insn = inst(STXW, 10, -4, 1);
      ld_insn = inst(LDXW, 0, 10, -4);
    }
    for (int i = 0; i < len - 1; i++) {
      pgm[i] = st_insn;
    }
    pgm[len - 1] = ld_insn;
  } else if (type <= 4) { // st, ld, st, ld, ....
    inst insns[2];
    if (type == 3) {
      insns[0] = inst(STXB, 10, -4, 1);
      insns[1] = inst(LDXB, 0, 10, -4);
    } else if (type == 4) {
      insns[0] = inst(STXW, 10, -4, 1);
      insns[1] = inst(LDXW, 0, 10, -4);
    }

    for (int i = 0; i < len / 2; i++) {
      pgm[2 * i] = insns[0];
      pgm[2 * i + 1] = insns[1];
    }
  } else if (type <= 6) { // st, ld, ld, ld, ....
    inst st_insn, ld_insn;
    if (type == 1) {
      st_insn = inst(STXB, 10, -4, 1);
      ld_insn = inst(LDXB, 0, 10, -4);
    } else if (type == 2) {
      st_insn = inst(STXW, 10, -4, 1);
      ld_insn = inst(LDXW, 0, 10, -4);
    }
    pgm[0] = st_insn;
    for (int i = 1; i < len; i++) {
      pgm[i] = ld_insn;
    }
  }
}

void time_z3_solver_st_ld(int len, int type) {
  inst::max_prog_len = len;
  inst* pgm = (inst*)malloc(len * sizeof(inst));
  if (type == 0) set_as_register_pgm(pgm, len, type);
  else set_as_st_ld_pgm(pgm, len, type);
  z3::expr smt = smt_pgm_register_eq_check(pgm, len, pgm, len);
  if (type == 0) {time_measure(is_smt_valid(smt), 1000, to_string(len) + "\t");}
  else {time_measure(is_smt_valid(smt), 10, to_string(len) + "\t");}
  free(pgm);
  pgm = nullptr;
}

void time_z3_solver_pgm(int type) {
  cout << type << endl;
  time_z3_solver_st_ld(2, type);
  time_z3_solver_st_ld(10, type);
  time_z3_solver_st_ld(20, type);
  time_z3_solver_st_ld(40, type);
  time_z3_solver_st_ld(80, type);
  time_z3_solver_st_ld(100, type);
  time_z3_solver_st_ld(200, type);
  if (type == 0) {
    time_z3_solver_st_ld(400, type);
    time_z3_solver_st_ld(600, type);
    time_z3_solver_st_ld(800, type);
    time_z3_solver_st_ld(1000, type);
  }
}

int main() {
  inst::max_prog_len = N3;
  inst::add_sample_imm(vector<int32_t> {264});
  mem_t::set_pkt_sz(128);
  mem_t::add_map(map_attr(128, 64, N3));
  mem_t::add_map(map_attr(96, 96, N3));
  mem_t::add_map(map_attr(64, 128, N3));
  time_z3_solver_pgm(0);
  time_z3_solver_pgm(1);
  time_z3_solver_pgm(2);
  time_z3_solver_pgm(3);
  time_z3_solver_pgm(4);
  time_z3_solver_pgm(5);
  time_z3_solver_pgm(6);

  return 0;

  time_interpret();
  time_smt_prog();
  time_vld_mem_input_output();
  // time_z3_solver_register();

  return 0;
}
