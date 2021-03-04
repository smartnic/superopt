#include "win_select.h"


/* If the insn does not satisfy ISA window constraints, return bool and
   this insn won't be selected in the windows.
*/
bool insn_satisfy_isa_win_constraints(const inst& insn) {
  return true;
}
