#include "win_select.h"

/* If the insn does not satisfy ISA window constraints, return false and
   this insn won't be selected in the windows.
*/
bool insn_satisfy_isa_win_constraints(const inst& insn, const inst_static_state& iss) {
  /* opcode is implemented */
  if (! inst::is_valid_opcode(insn._opcode)) return false;

  /* not a function call (some helpers are not supported; can hardly improve function call) */
  if (insn._opcode == CALL) return false;

  /* memory access:
     1. addr register is a pointer (pointer from addxy is not tracked in static analysis)
     2. memory access offset is a constant (safety: not able to check out of bound)
  */
  vector<int> mem_acc_regs;
  insn.mem_access_regs(mem_acc_regs);
  if (mem_acc_regs.size() > 0) {
    for (int i = 0; i < mem_acc_regs.size(); i++) {
      int reg = mem_acc_regs[i];
      const vector<register_state>& reg_state = iss.reg_state[reg];
      if (reg_state.size() != 1) return false; // constant offset check
      int reg_type = reg_state[0].type;
      if (! is_ptr(reg_type)) return false; // addr reg is a ptr check
    }
  }

  return true;
}
