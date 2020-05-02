#include "inst_var.h"

using namespace std;

void update_ps_by_input(prog_state& ps, const inout_t& input) {
  ps._regs[0] = input.reg;
}

void update_output_by_ps(inout_t& output, const prog_state& ps) {
  output.reg =  ps._regs[0];
}
