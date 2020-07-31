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

inst bm3_0[N3] = {inst(191, 1, 6, 0, 0),
                  inst(183, 0, 1, 0, 0),
                  inst(97, 6, 2, 36, 0),
                  inst(86, 0, 2, 4, 6),
                  inst(0, 0, 0, 0, 0), // call 7 modified as nop
                  inst(188, 0, 1, 0, 0),
                  inst(103, 0, 1, 0, 32),
                  inst(119, 0, 1, 0, 32),
                  inst(123, 1, 10, -40, 0),
                  inst(97, 6, 1, 4, 0),
                  inst(99, 1, 10, -32, 0),
                  inst(97, 6, 1, 24, 0),
                  inst(99, 1, 10, -16, 0),
                  inst(180, 0, 8, 0, 0),
                  inst(107, 8, 10, -26, 0),
                  inst(97, 10, 1, -16, 0),
                  inst(107, 1, 10, -28, 0),
                  inst(191, 10, 2, 0, 0),
                  inst(7, 0, 2, 0, -40),
                  inst(LDMAPID, 1, 0),
                  inst(0, 0, 0, 0),
                  inst(133, 0, 0, 0, 1),
                  inst(191, 0, 7, 0, 0),
                  inst(21, 0, 7, 65, 0),
                  inst(97, 7, 1, 0, 0),
                  inst(99, 1, 10, -56, 0),
                  inst(105, 7, 1, 4, 0),
                  inst(107, 8, 10, -48, 0),
                  inst(107, 8, 10, -50, 0),
                  inst(107, 8, 10, -46, 0),
                  inst(107, 1, 10, -52, 0),
                  inst(22, 0, 1, 10, 0),
                  inst(191, 10, 2, 0, 0),
                  inst(7, 0, 2, 0, -56),
                  inst(LDMAPID, 1, 1),
                  inst(0, 0, 0, 0),
                  inst(133, 0, 0, 0, 1),
                  inst(21, 0, 0, 2, 0),
                  inst(105, 0, 1, 4, 0),
                  inst(86, 0, 1, 10, 0),
                  inst(180, 0, 1, 0, 0),
                  inst(107, 1, 10, -52, 0),
                  inst(191, 10, 2, 0, 0),
                  inst(7, 0, 2, 0, -56),
                  inst(LDMAPID, 1, 1),
                  inst(0, 0, 0, 0),
                  inst(133, 0, 0, 0, 1),
                  inst(21, 0, 0, 5, 0),
                  inst(105, 0, 1, 4, 0),
                  inst(22, 0, 1, 3, 0),
                  inst(105, 7, 1, 6, 0),
                  inst(105, 0, 2, 6, 0),
                  inst(30, 1, 2, 20, 0),
                  inst(191, 10, 2, 0, 0),
                  inst(7, 0, 2, 0, -40),
                  inst(LDMAPID, 1, 0),
                  inst(0, 0, 0, 0),
                  inst(133, 0, 0, 0, 3),
                  inst(183, 0, 6, 0, 0),
                  inst(123, 6, 10, -8, 0),
                  inst(123, 6, 10, -16, 0),
                  inst(183, 0, 1, 0, 264),
                  inst(123, 1, 10, -24, 0),
                  inst(191, 10, 2, 0, 0),
                  inst(7, 0, 2, 0, -24),
                  inst(LDMAPID, 1, 2),
                  inst(0, 0, 0, 0),
                  inst(133, 0, 0, 0, 1),
                  inst(21, 0, 0, 9, 0),
                  inst(121, 0, 1, 0, 0),
                  inst(7, 0, 1, 0, 1),
                  inst(123, 1, 0, 0, 0),
                  inst(5, 0, 0, 16, 0),
                  inst(97, 7, 1, 0, 0),
                  inst(99, 1, 6, 4, 0),
                  inst(105, 7, 1, 4, 0),
                  inst(99, 1, 6, 24, 0),
                  inst(5, 0, 0, 11, 0),
                  inst(123, 6, 10, -8, 0),
                  inst(183, 0, 1, 0, 1),
                  inst(123, 1, 10, -16, 0),
                  inst(191, 10, 2, 0, 0),
                  inst(7, 0, 2, 0, -24),
                  inst(191, 10, 3, 0, 0),
                  inst(7, 0, 3, 0, -16),
                  inst(LDMAPID, 1, 2),
                  inst(0, 0, 0, 0),
                  inst(180, 0, 4, 0, 0),
                  inst(133, 0, 0, 0, 2),
                  inst(180, 0, 0, 0, 1),
                  inst(149, 0, 0, 0, 0),
                 };
void time_z3_solver_register() {
  mem_t::set_pkt_sz(128);
  mem_t::add_map(map_attr(128, 64, N3));
  mem_t::add_map(map_attr(96, 96, N3));
  mem_t::add_map(map_attr(64, 128, N3));
  validator vld(bm3, N3);
  auto t1 = NOW;
  cout << vld.is_equal_to(bm3, N3, bm3, N3) << endl;
  auto t2 = NOW;
  cout << "is_equal_to: " << DUR(t1, t2) << " us" << endl;
  // time_measure(vld.is_equal_to(bm3, N3, bm3, N3), 1, "bm3 ");
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
  if (type == 0) {time_measure(is_smt_valid(smt), 1, to_string(len) + "\t");}
  else {time_measure(is_smt_valid(smt), 1, to_string(len) + "\t");}
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

// stack: st, st, st, st + ld
inst pgm1[5] = {inst(STB, 10, -1, 0),
                inst(STB, 10, -2, 0),
                inst(STB, 10, -3, 0),
                inst(STB, 10, -4, 0),
                inst(LDXB, 0, 10, -1),
               };
inst pgm2[5] = {inst(STB, 10, -1, 11),
                inst(STB, 10, -2, 22),
                inst(STB, 10, -3, 33),
                inst(STB, 10, -4, 44),
                inst(LDXB, 0, 10, -1),
               };
inst pgm3[5] = {inst(STXB, 10, -1, 0),
                inst(STXB, 10, -2, 0),
                inst(STXB, 10, -3, 0),
                inst(STXB, 10, -4, 0),
                inst(LDXB, 0, 10, -1),
               };
inst pgm4[5] = {inst(STXB, 10, -1, 1),
                inst(STXB, 10, -2, 1),
                inst(STXB, 10, -3, 1),
                inst(STXB, 10, -4, 1),
                inst(LDXB, 0, 10, -1),
               };
// pkt: st, st, st, st
inst pgm5[5] = {inst(STB, 1, 1, 0),
                inst(STB, 1, 2, 0),
                inst(STB, 1, 3, 0),
                inst(STB, 1, 4, 0),
                inst(),
               };
inst pgm6[5] = {inst(STB, 1, 1, 11),
                inst(STB, 1, 2, 22),
                inst(STB, 1, 3, 33),
                inst(STB, 1, 4, 44),
                inst(),
               };
inst pgm7[5] = {inst(STXB, 1, 1, 0),
                inst(STXB, 1, 2, 0),
                inst(STXB, 1, 3, 0),
                inst(STXB, 1, 4, 0),
                inst(),
               };
inst pgm8[5] = {inst(STXB, 1, 1, 1),
                inst(STXB, 1, 2, 2),
                inst(STXB, 1, 3, 3),
                inst(STXB, 1, 4, 4),
                inst(),
               };
// stack + pkt
inst pgm9[5] = {inst(STXB, 1, 1, 1),
                inst(STXB, 1, 2, 1),
                inst(STXB, 10, -1, 1),
                inst(STXB, 10, -2, 1),
                inst(LDXB, 0, 10, -1),
               };
inst pgm10[5] = {inst(STB, 1, 1, 0),
                 inst(STB, 1, 2, 0),
                 inst(STB, 1, 3, 0),
                 inst(STB, 1, 4, 0),
                 inst(STDW, 10, -16, 0),
                };
inst pgm11[5] = {inst(STB, 1, 1, 0),
                 inst(STB, 1, 2, 0),
                 inst(STB, 1, 3, 0),
                 inst(STB, 1, 4, 0),
                 inst(STXDW, 10, -16, 0),
                };
// pkt ld
inst pgm12[5] = {inst(LDXB, 0, 1, 1),
                 inst(LDXB, 0, 1, 2),
                 inst(LDXB, 0, 1, 3),
                 inst(LDXB, 0, 1, 4),
                 inst(),
                };
inst pgm13[5] = {inst(LDXDW, 0, 1, 1),
                 inst(LDXDW, 0, 1, 2),
                 inst(LDXDW, 0, 1, 3),
                 inst(LDXDW, 0, 1, 4),
                 inst(),
                };
inst pgm14[5] = {inst(LDXDW, 0, 1, 0),
                 inst(LDXDW, 0, 1, 4),
                 inst(LDXDW, 0, 1, 8),
                 inst(LDXDW, 0, 1, 12),
                 inst(),
                };

void set_as_pgm_diff_offsets_1(inst* pgm, int len) {
  for (int i = 0; i < len; i++) {
    int off = i;
    int src_reg = i % NUM_REGS;
    pgm[i] = inst(STXB, 1, off, src_reg); // *(u8 *)(r1 + off) = src_reg
  }
}

void set_as_pgm_diff_offsets_2(inst* pgm, int len) {
  for (int i = 0; i < len - 1; i++) {
    int off = i;
    int src_reg = i % NUM_REGS;
    pgm[i] = inst(STXB, 1, off, src_reg); // *(u8 *)(r1 + off) = src_reg
  }
  int dst_reg = 0, src_reg = 1, off = 0;
  pgm[len - 1] = inst(LDXB, dst_reg, src_reg, off);
}

void set_as_pgm_diff_offsets_3(inst* pgm, int len) {
  for (int i = 0; i < len; i++) {
    int off = i;
    pgm[i] = inst(LDXB, 0, 1, off); // r0 = *(u8 *)(r1 + off)
  }
}

// st + ld for pkt+stack, ld from pkt, st to stack, with diff offsets
void set_as_pgm_diff_offsets_4(inst* pgm, int len) {
  for (int i = 0; i < len / 2; i++) {
    int off = i;
    pgm[i * 2] = inst(LDXB, 0, 1, off);     // r0 = *(u8 *)(r1 + off)
    pgm[i * 2 + 1] = inst(STXB, 10, -off, 0); // *(u8 *)(r10 - off) = r0;
  }
}

void time_is_equal_to_pgm_diff_len_type(int n_off, int type) {
  int len;
  if (type == 1) len = n_off; // n pkt st
  else if (type == 2) len = n_off + 1; // n pkt st + 1 pkt ld
  else if (type == 3) len = n_off; // n pkt ld
  else if (type == 4) len = n_off;
  inst* pgm = (inst*)malloc(len * sizeof(inst));
  if (type == 1) set_as_pgm_diff_offsets_1(pgm, len);
  else if (type == 2) set_as_pgm_diff_offsets_2(pgm, len);
  else if (type == 3) set_as_pgm_diff_offsets_3(pgm, len);
  else if (type == 4) set_as_pgm_diff_offsets_4(pgm, len);

  mem_t::_layout.clear();
  mem_t::set_pkt_sz(len + 1);
  inst::max_prog_len = len;
  validator vld;
  vld.set_orig(pgm, len);
  time_measure(vld.is_equal_to(pgm, len, pgm, len), 10,
               "validator::is_equal_to: ");
  free(pgm);
  pgm = nullptr;
}

void time_is_equal_to_pgm_diff_len() {
  vector<int> n_off = {1, 2, 4, 8, 16, 32, 64, 128};
  cout << "pkt: n st" << endl;
  for (int i = 0; i < n_off.size(); i++) {
    cout << "n_offsets: " << n_off[i] << "\t";
    time_is_equal_to_pgm_diff_len_type(n_off[i], 1);
  }
  cout << "pkt: n st + 1 ld" << endl;
  for (int i = 0; i < n_off.size(); i++) {
    cout << "n_offsets: " << n_off[i] << "\t";
    time_is_equal_to_pgm_diff_len_type(n_off[i], 2);
  }
  cout << "pkt: n ld" << endl;
  for (int i = 0; i < n_off.size(); i++) {
    cout << "n_offsets: " << n_off[i] << "\t";
    time_is_equal_to_pgm_diff_len_type(n_off[i], 3);
  }
  cout << "n/2 (pkt ld + stack st)" << endl;
  for (int i = 1; i < n_off.size(); i++) {
    cout << "n_offsets: " << n_off[i] << "\t";
    time_is_equal_to_pgm_diff_len_type(n_off[i], 4);
  }
}

// func_id: 1 lookup; 2 update; 3 delete
#define N_map_op 9
void set_as_map_pgm(inst* pgm, int func_id, bool lookup_ld = false) {
  int n_map = mem_t::maps_number();
  inst p[N_map_op] = {inst(STXB, 10, -2, 1),  // *addr_v = r1
                      inst(MOV64XC, 1, (int)gen_random(0, 20)), // *addr_k = k1
                      inst(STXB, 10, -1, 1),
                      inst(LDMAPID, 1, (int)gen_random(0, n_map - 1)), // r1 = map_id
                      inst(MOV64XY, 2, 10),   // r2(addr_k) = r10 - 1
                      inst(ADD64XC, 2, -1),
                      inst(MOV64XY, 3, 10),   // r3(addr_v) = r10 - 2
                      inst(ADD64XC, 3, -2),
                      inst(CALL, func_id),
                     };
  for (int i = 0; i < N_map_op; i++) pgm[i] = p[i];
}

void time_is_equal_to_pgm(inst* pgm, int len) {
  inst::max_prog_len = len;
  validator vld;
  vld.set_orig(pgm, len);
  time_measure(vld.is_equal_to(pgm, len, pgm, len), 10,
               "validator::is_equal_to: ");
}

void time_is_equal_to_pgms(inst* pgm1, int len1, inst* pgm2, int len2) {
  validator vld;
  vld.set_orig(pgm1, len1);
  time_measure(vld.is_equal_to(pgm1, len1, pgm2, len2), 10,
               "validator::is_equal_to: ");
}

void time_z3_solver_map_pgm_one_case(int n_op, int n_maps, int type) {
  // # map operations # maps
  mem_t::_layout.clear();
  for (int i = 0; i < n_maps; i++) mem_t::add_map(map_attr(8, 8, 32));
  inst::max_prog_len = n_op * N_map_op + 1;
  int len = inst::max_prog_len;
  inst* pgm = (inst*)malloc(len * sizeof(inst));
  int func_id = 0;
  if (type < 4) func_id = type;
  else if (type == 4) func_id = BPF_FUNC_map_lookup;
  bool lookup_ld = false;
  if (type == 4) lookup_ld = true;
  for (int i = 0; i < n_op; i++) {
    int cur_id = i * N_map_op;
    set_as_map_pgm(&pgm[cur_id], func_id, lookup_ld);
  }
  pgm[len - 1] = inst(EXIT);
  // cout << "print program" << endl;
  // for (int i = 0; i < len; i++) {
  //   pgm[i].print();
  // }
  time_is_equal_to_pgm(pgm, len);
  free(pgm);
  pgm = nullptr;
}

void time_z3_solver_map_pgm(int type) {
  cout << "different # map operation" << endl;
  time_z3_solver_map_pgm_one_case(1, 1, type);
  time_z3_solver_map_pgm_one_case(2, 1, type);
  time_z3_solver_map_pgm_one_case(4, 1, type);
  time_z3_solver_map_pgm_one_case(8, 1, type);
  time_z3_solver_map_pgm_one_case(16, 1, type);
  cout << "different # maps" << endl;
  time_z3_solver_map_pgm_one_case(4, 1, type);
  time_z3_solver_map_pgm_one_case(4, 2, type);
  time_z3_solver_map_pgm_one_case(4, 4, type);
  time_z3_solver_map_pgm_one_case(4, 8, type);
  time_z3_solver_map_pgm_one_case(4, 16, type);
}

void time_error_cost_without_solver() {
  double w_e = 1.0;
  double w_p = 0.0;
  vector<inout_t> inputs(5);
  for (int i = 0; i < inputs.size(); i++) {
    inputs[i].init();
  }
  gen_random_input(inputs, -50, 50);
  cost c;
  prog orig(bm0);
  c.init(&orig, N0, inputs, w_e, w_p);
  time_measure(c.error_cost(&orig, inst::max_prog_len, &orig, inst::max_prog_len);
               orig._error_cost = -1;
               orig._perf_cost = -1,
               200,
               "cost::error_cost: "
              );
}

int main(int argc, char* argv[]) {
  time_z3_solver_register();
  return 0;
  time_z3_solver_map_pgm(2);
  time_z3_solver_map_pgm(3);
  return 0;
  int loop_times = 1;
  if (argc > 1) {
    loop_times = atoi(argv[1]);
  }
  for (int i = 0; i < loop_times; i++) {
    cout << "loop_time: " << i << endl;
    time_is_equal_to_pgm_diff_len();
  }
  return 0;
  cout << "stack" << endl;
  time_is_equal_to_pgm(pgm1, 5);
  time_is_equal_to_pgm(pgm2, 5);
  time_is_equal_to_pgm(pgm3, 5);
  time_is_equal_to_pgm(pgm4, 5);

  cout << "packet" << endl;
  mem_t::_layout.clear();
  mem_t::set_pkt_sz(20);
  time_is_equal_to_pgm(pgm5, 5);
  time_is_equal_to_pgm(pgm6, 5);
  time_is_equal_to_pgm(pgm7, 5);
  time_is_equal_to_pgm(pgm8, 5);

  cout << "stack + packet" << endl;
  time_is_equal_to_pgm(pgm9, 5);
  time_is_equal_to_pgm(pgm10, 5);
  time_is_equal_to_pgm(pgm11, 5);
  time_is_equal_to_pgms(pgm5, 5, pgm11, 5);
  time_is_equal_to_pgms(pgm11, 5, pgm5, 5);

  cout << "packet ld" << endl;
  time_is_equal_to_pgm(pgm12, 5);
  time_is_equal_to_pgm(pgm13, 5);
  time_is_equal_to_pgm(pgm14, 5);

  return 0;

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

  return 0;
}
