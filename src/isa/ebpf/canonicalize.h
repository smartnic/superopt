#pragma once

#include <unordered_set>
#include "inst.h"

using namespace std;

void canonicalize(inst* program, int len);

void remove_nops(inst* program, int len);

class inst_static_state {
 public:
  vector<vector<register_state>> reg_state; // all possible states of registers
  live_variables live_var;

  inst_static_state();
  void copy_reg_state(int dst_reg, int src_reg);
  void set_reg_state(int reg, int type, int off = 0);
  void set_reg_state(int reg, register_state rs);
  void insert_reg_state(inst_static_state& iss);
  void insert_live_reg(int reg);
  void insert_live_off(int type, int off);
  void insert_live_var(inst_static_state& iss);
  static void intersection_live_var(inst_static_state& iss, inst_static_state& iss1, inst_static_state& iss2);
  inst_static_state& operator=(const inst_static_state &rhs);
};

typedef vector<inst_static_state> prog_static_state;
void static_analysis(prog_static_state& pss, inst* program, int len);
void set_up_smt_inout_orig(prog_static_state& pss, inst* program, int len, int win_start, int win_end);
void set_up_smt_inout_win(smt_input& sin, smt_output& sout, prog_static_state& pss_orig, inst* program, int win_start, int win_end);
