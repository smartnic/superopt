#include <unordered_set>
#include "win_select.h"

int num_unimplemented = 0;
unordered_set<int> unimplemented_opcodes = {};
int num_call = 0;
int num_multi_values = 0;
int num_symbolic = 0;

/* If the insn does not satisfy ISA window constraints, return false and
   this insn won't be selected in the windows.
*/
bool insn_satisfy_isa_win_constraints(const inst& insn, const inst_static_state& iss) {
  /* opcode is implemented */
  if (! inst::is_valid_opcode(insn._opcode)) {
    num_unimplemented++;
    unimplemented_opcodes.insert(insn._opcode);
    return false;
  }

  /* not a function call
    (1. some helpers are not supported;
     2. can hardly improve function call;
     3. map update flag limitation:
        https://github.com/smartnic/superopt/commit/d1c11c5ebfbeda68fd4e9cbc14066bca8c0316f1)
  */
  if (insn._opcode == CALL) {
    num_call++;
    return false;
  }

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
      if (reg_state.size() != 1) {  // constant offset check
        num_multi_values++;
        return false;
      }
      int reg_type = reg_state[0].type;
      if (! is_ptr(reg_type)) { // addr reg is a ptr check
        num_symbolic++;
        return false;
      }
    }
  }

  return true;
}

void reset_isa_win_constraints_statistics() {
  num_unimplemented = 0;
  unimplemented_opcodes = {};
  num_call = 0;
  num_multi_values = 0;
  num_symbolic = 0;
}

void print_isa_win_constraints_statistics() {
  cout << "# unimplemented opcodes: " << num_unimplemented << "  ";
  if (num_unimplemented != 0) {
    cout << "# opcodes:" << unimplemented_opcodes.size() << "  ";
    cout << "opcodes:";
    for (auto op : unimplemented_opcodes) cout << hex << "0x" << op << dec << " ";
  }
  cout << endl;
  cout << "# helper calls: " << num_call << endl;
  cout << "# mem_acc multi_values: " << num_multi_values << endl;
  cout << "# mem_acc num_symbolic: " << num_symbolic << endl;
}
