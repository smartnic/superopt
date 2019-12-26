#include "smt_inst.h"

using namespace z3;

#define CURSRC sv.get_cur_reg_var(SRCREG(in))
#define CURDST sv.get_cur_reg_var(DSTREG(in))
#define NEWDST sv.update_reg_var(DSTREG(in))
#define IMM2 IMM2VAL(in)

expr smt_inst(smt_var& sv, const inst& in) {
  switch (in._opcode) {
    case ADDXY: return (CURDST + CURSRC == NEWDST);
    case MOVXC: return (IMM2 == NEWDST);
    case MAXC: {
      expr curDst = CURDST;
      expr newDst = NEWDST;
      expr cond1 = (curDst > IMM2) and (newDst == curDst);
      expr cond2 = (curDst <= IMM2) and (newDst == IMM2);
      return (cond1 or cond2);
    }
    case MAXX: {
      expr curDst = CURDST;
      expr curSrc = CURSRC;
      expr newDst = NEWDST;
      expr cond1 = (curDst > curSrc) and (newDst == curDst);
      expr cond2 = (curDst <= curSrc) and (newDst == curSrc);
      return (cond1 or cond2);
    }
    default: return string_to_expr("false");
  }
}

expr smt_inst_jmp(smt_var& sv, const inst& in) {
  // e is formula for Jmp
  expr e = string_to_expr("true");
  switch (in._opcode) {
    case JMPEQ: e = (CURDST == CURSRC); return e;
    case JMPGT: e = (CURDST > CURSRC); return e;
    case JMPGE: e = (CURDST >= CURSRC); return e;
    case JMPLT: e = (CURDST < CURSRC); return e;
    case JMPLE: e = (CURDST <= CURSRC); return e;
  }
  return e;
}
