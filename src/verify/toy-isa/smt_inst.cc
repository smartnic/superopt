#include "../../../src/isa/inst_codegen.h"
#include "smt_inst.h"

using namespace z3;

#define CURSRC sv.get_cur_reg_var(TOY_ISA_SRCREG(in))
#define CURDST sv.get_cur_reg_var(TOY_ISA_DSTREG(in))
#define NEWDST sv.update_reg_var(TOY_ISA_DSTREG(in))
#define IMM2 TOY_ISA_IMM2VAL(in)

expr smt_inst(smt_var& sv, const inst& in) {
  expr curDst = string_to_expr("false");
  expr curSrc = string_to_expr("false");
  expr newDst = string_to_expr("false");
  int imm;
  switch (in._opcode) {
    case toy_isa::MAXX:
    case toy_isa::ADDXY:
      curDst = CURDST;
      curSrc = CURSRC;
      newDst = NEWDST;
      break;
    case toy_isa::MOVXC:
      imm = IMM2;
      newDst = NEWDST;
      break;
    case toy_isa::MAXC:
      curDst = CURDST;
      imm = IMM2;
      newDst = NEWDST;
      break;
  }
  switch (in._opcode) {
    case toy_isa::ADDXY: return predicate_add(curDst, curSrc, newDst);
    case toy_isa::MOVXC: return predicate_mov(imm, newDst);
    case toy_isa::MAXC: return predicate_max(curDst, imm, newDst);
    case toy_isa::MAXX: return predicate_max(curDst, curSrc, newDst);
    default: return string_to_expr("false");
  }
}

expr smt_inst_jmp(smt_var& sv, const inst& in) {
  switch (in._opcode) {
    case toy_isa::JMPEQ: return (CURDST == CURSRC);
    case toy_isa::JMPGT: return (CURDST > CURSRC);
    case toy_isa::JMPGE: return (CURDST >= CURSRC);
    case toy_isa::JMPLT: return (CURDST < CURSRC);
    case toy_isa::JMPLE: return (CURDST <= CURSRC);
    default: return string_to_expr("false");
  }
}
