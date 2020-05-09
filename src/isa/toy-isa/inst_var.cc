#include "inst_var.h"

using namespace std;

void update_ps_by_input(prog_state& ps, const inout_t& input) {
  ps._regs[0] = input.reg;
}

void update_output_by_ps(inout_t& output, const prog_state& ps) {
  output.reg =  ps._regs[0];
}

void get_cmp_lists(vector<int>& val_list1, vector<int>& val_list2,
                   inout_t& output1, inout_t& output2) {
  val_list1.resize(1);
  val_list2.resize(1);
  val_list1[0] = output1.reg;
  val_list2[0] = output2.reg;
}
