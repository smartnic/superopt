#pragma once

#include <unordered_set>
#include "inst.h"

using namespace std;

void canonicalize(inst* program, int len);

void remove_nops(inst* program, int len);

// for static analysis
// TODO: deal with map later
struct register_state {
  int type;
  int off;
};

struct live_variables {
  unordered_set<int> regs;
  // stack, pkt
  unordered_map<int, unordered_set<int>> mem; // offsets in different memory ranges, todo: deal with map later
};

class inst_static_state {
 public:
  vector<vector<register_state>> reg_state; // all possible states of registers
  live_variables live_var;

  inst_static_state();
  void copy_reg_state(int dst_reg, int src_reg);
  void set_reg_state(int reg, int type, int off = 0);
  void insert_reg_state(inst_static_state& iss);
  void insert_live_reg(int reg);
  void insert_live_off(int type, int off);
  void insert_live_var(inst_static_state& iss);
  static void intersection_live_var(inst_static_state& iss, inst_static_state& iss1, inst_static_state& iss2);
};

typedef vector<inst_static_state> prog_static_state;
void static_analysis(prog_static_state& pss, inst* program, int len);