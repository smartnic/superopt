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

class inst_static_state {
 public:
  vector<vector<register_state>> reg_state; // all possible states of registers
  inst_static_state();
  void copy_reg_state(int dst_reg, int src_reg);
  void set_reg_state(int reg, int type, int off = 0);
  void insert_reg_state(inst_static_state& iss);
};

typedef vector<inst_static_state> prog_static_state;
void static_analysis(inst* program, int len);