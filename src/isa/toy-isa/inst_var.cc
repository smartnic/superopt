#include <random>
#include <unordered_set>
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

void gen_random_input(vector<inout_t>& inputs, int reg_min, int reg_max) {
  unordered_set<reg_t> input_set;
  for (size_t i = 0; i < inputs.size();) {
    reg_t input = random_uint64(reg_min, reg_max);
    if (input_set.find(input) == input_set.end()) {
      input_set.insert(input);
      inputs[i].reg = input;
      i++;
    }
  }
}
