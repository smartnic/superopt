#include <iostream>
#include "inst.h"

using namespace std;

#define DSTREG(inst_var) (inst_var)._args[0]
#define SRCREG(inst_var) (inst_var)._args[1]
#define SRC2REG(inst_var) (inst_var)._args[3]
#define IMM1VAL(inst_var) (inst_var)._args[0]
#define IMM2VAL(inst_var) (inst_var)._args[1]

string inst::opcode_to_str(int opcode) const {
  switch (opcode) {
    case NOP: return "nop";
    // case IMMED: return "immed";
    default: return "unknown opcode";
  }
}

void inst::print() const {
  cout << opcode_to_str(_opcode);
  if (get_num_operands() > 0) {
    cout << "[";
    for (int i = 0; i < get_num_operands(); i++) {
      cout << ", " << _args[i];
    }
    cout << "]";
  }
  cout << endl;
}

inst& inst::operator=(const inst &rhs) {
  _opcode = rhs._opcode;
  _args[0] = rhs._args[0];
  _args[1] = rhs._args[1];
  _args[2] = rhs._args[2];
  return *this;
}

bool inst::operator==(const inst &x) const {
  return ((_opcode == x._opcode) &&
          (_args[0] == x._args[0]) &&
          (_args[1] == x._args[1]) &&
          (_args[2] == x._args[2]));
}

// For jmp opcode, it can only jump forward
int inst::get_max_operand_val(int op_index, int inst_index) const {
  // max value for each operand type
  int max_val[4] = {
    [OP_UNUSED] = 0,
    [OP_REG] = NUM_REGS - 1,
    [OP_IMM] = MAX_CONST,
    [OP_OFF] = MAX_PROG_LEN - inst_index - 2,
  };
  return max_val[OPTYPE(_opcode, op_index)];
}

// For jmp opcode, it can only jump forward
int inst::get_min_operand_val(int op_index, int inst_index) const {
  return 0;
}

vector<int> inst::get_canonical_reg_list() const {
  vector<int> reg_list;
  for (int i = 0; i < get_insn_num_regs(); i++) {
    if (_args[i] != 0)
      reg_list.push_back(_args[i]);
  }
  return reg_list;
}

vector<int> inst::get_isa_canonical_reg_list() {
  return vector<int> {1, 2, 3};
}

int inst::get_jmp_dis() const {
  switch (get_opcode_type()) {
    case (OP_UNCOND_JMP): return _args[0];
    case (OP_COND_JMP): return _args[2];
    default: cout << "Error: opcode is not jmp" << endl; return 0;
  }
}

// void inst::insert_jmp_opcodes(unordered_set<int>& jmp_set) const {
//   jmp_set.insert(JMP);
//   jmp_set.insert(JMPEQ);
//   jmp_set.insert(JMPGT);
//   jmp_set.insert(JMPGE);
//   jmp_set.insert(JMPLT);
//   jmp_set.insert(JMPLE);
// }

int inst::inst_output_opcode_type() const {
  // for now, just return a register value
  return RET_X;
}

int inst::inst_output() const {
  // for now, return value of register a0 (index 0)
  return 0;
}

bool inst::is_real_inst() const {
  if (_opcode == NOP) return false;
  return true;
}

bool inst::is_reg(int op_index) const {
  if (OPTYPE(_opcode, op_index) == OP_REG) return true;
  return false;
}

int inst::implicit_ret_reg() const {
  return 0;
}

void inst::set_as_nop_inst() {
  _opcode = NOP;
  _args[0] = 0;
  _args[1] = 0;
  _args[2] = 0;
}

#undef IMM2
#define CURSRC sv.get_cur_reg_var(SRCREG(*this))
#define CURSRC2 sv.get_cur_reg_var(SRC2REG(*this))
#define CURDST sv.get_cur_reg_var(DSTREG(*this))
#define NEWDST sv.update_reg_var(DSTREG(*this))
#define IMM2 to_expr(IMM2VAL(*this))

z3::expr inst::smt_inst(smt_var& sv) const {
  switch (_opcode) {
    case IMMED: return predicate_mov(IMM2, NEWDST);
    case ALU:
      switch (_args[2]) {
        case ALU_PLUS: return predicate_add(CURSRC, CURSRC2, NEWDST);
        case ALU_PLUS_16: return predicate_add16(CURSRC, CURSRC2, NEWDST);
        case ALU_PLUS_8: return predicate_add8(CURSRC, CURSRC2, NEWDST);
        // case ALU_PLUS_CARRY: 
        // case ALU_MINUS_CARRY:
        case ALU_MINUS: return predicate_subtract(CURSRC, CURSRC2, NEWDST);
        case ALU_B_MINUS_A: return predicate_subtract(CURSRC2, CURSRC, NEWDST); // note reversed order
        case ALU_B: return predicate_mov(CURSRC2, NEWDST);
        case ALU_INV_B: return predicate_inv(CURSRC2, NEWDST);
        case ALU_AND: return predicate_and(CURSRC, CURSRC2, NEWDST);
        case ALU_INV_AND: return predicate_inv_and(CURSRC, CURSRC2, NEWDST);
        case ALU_AND_INV: return predicate_inv_and(CURSRC2, CURSRC, NEWDST); // note reversed order
        case ALU_OR: return predicate_or(CURSRC, CURSRC2, NEWDST);
        case ALU_XOR: return predicate_xor(CURSRC, CURSRC2, NEWDST);
        default: return Z3_false;
      }
      break;
    default: return Z3_false;
  }
}



z3::expr inst::smt_inst_jmp(smt_var& sv) const {
  // not yet implemented
  return string_to_expr("false");
}

z3::expr inst::smt_set_pre(z3::expr input, smt_var& sv) {
  z3::expr f = Z3_true;
  f = (sv.get_cur_reg_var(0) == input);
  for (size_t i = 1; i < NUM_REGS; i++) {
    f = f && (sv.get_cur_reg_var(i) == 0);
  }
  return f;
}


void interpret(inout_t& output, inst* program, int length, prog_state &ps, const inout_t& input) {
  /* Input currently is just one integer which will be written into R0. Will
  need to generalize this later. */
  inst *insn = program;
  ps.clear();
  ps._regs[0] = input.reg;

  static void *jumptable[NUM_INSTR] = {
    [NOP]   = && INSN_NOP,
    [IMMED] = && INSN_IMMED,
    [ALU] = && INSN_ALU,
  };

  static void *alu_jumptable[NUM_ALU_INSTR] = {
    [ALU_PLUS] = && INSN_ALU_PLUS,
    [ALU_PLUS_16] = && INSN_ALU_PLUS_16,
    [ALU_PLUS_8] = && INSN_ALU_PLUS_8,
    [ALU_PLUS_CARRY] = && INSN_ALU_PLUS_CARRY,
    // [ALU_MINUS_CARRY] = && INSN_ALU_MINUS_CARRY,
    [ALU_MINUS] = && INSN_ALU_MINUS,
    [ALU_B_MINUS_A] = && INSN_ALU_B_MIUS_A,
    [ALU_B] = && INSN_ALU_B,
    [ALU_INV_B] = && INSN_ALU_INV_B,
    [ALU_AND] = && INSN_ALU_AND,
    [ALU_INV_AND] = && INSN_ALU_INV_AND,
    [ALU_AND_INV] = && INSN_ALU_AND_INV,
    [ALU_OR] = && INSN_ALU_OR,
    [ALU_XOR] = && INSN_ALU_XOR,
  };

#undef CONT
#define CONT { \
      insn++;                                                           \
      if (insn < program + length) {                                    \
        goto select_insn;                                               \
      } else goto out;                                                  \
  }

#undef DST
#undef SRC
#undef IMM1
#undef IMM2
#define DST ps._regs[DSTREG(*insn)]
#define SRC ps._regs[SRCREG(*insn)]
#define SRC2 ps._regs[SRC2REG(*insn)]
#define IMM1 IMM1VAL(*insn)
#define IMM2 IMM2VAL(*insn)
#define OP_SUBTYPE insn->_args[2]

// what's the better way to test for this?
// ps._unsigned_carry = (SRC > UINT32_MAX - SRC2) : 1 ? 0;
#define SET_CARRY {                                                   \
  ps._unsigned_carry = (((uint64_t)SRC + (uint64_t)SRC2) >> 32) & 1;  \
}

#define CLEAR_CARRY {     \
  ps._unsigned_carry = 0; \
}

select_insn:
  goto *jumptable[insn->_opcode];

INSN_NOP:
  CONT;

INSN_IMMED:
  DST = compute_mov(IMM2, DST);
  CONT;

INSN_ALU:
  goto *alu_jumptable[OP_SUBTYPE];

INSN_ALU_PLUS:
  DST = compute_add(SRC, SRC2);
  SET_CARRY;
  CONT;

INSN_ALU_PLUS_16:
  DST = compute_add16(SRC, SRC2, DST);
  SET_CARRY;
  CONT;

INSN_ALU_PLUS_8:
  DST = compute_add8(SRC, SRC2, DST);
  SET_CARRY;
  CONT;

INSN_ALU_PLUS_CARRY:
  DST = compute_add_carry(SRC, SRC2, (ps._unsigned_carry), DST);
  cout << "here INSN_ALU_PLUS_CARRY" << endl;
  cout << SRC << endl;
  cout << SRC2 << endl;
  cout << ps._unsigned_carry << endl;
  cout << ((uint64_t)SRC + (uint64_t)SRC2 + (uint64_t)ps._unsigned_carry) << endl;
  ps._unsigned_carry = (((uint64_t)SRC + (uint64_t)SRC2 + (uint64_t)ps._unsigned_carry) >> 32) & 1;
  CONT;

INSN_ALU_MINUS:
  cout << "here INSN_ALU_MINUS" << endl;
  cout << SRC << endl;
  cout << SRC2 << endl;
  DST = compute_subtract(SRC, SRC2);
  cout << DST << endl;
  SET_CARRY;
  CONT;

INSN_ALU_B_MIUS_A:
  DST = compute_subtract(SRC2, SRC);
  SET_CARRY;
  CONT;

INSN_ALU_B:
  DST = compute_mov(SRC2);
  CLEAR_CARRY;
  CONT;

INSN_ALU_INV_B:
  DST = compute_inv(SRC2, DST);
  CLEAR_CARRY;
  CONT;

INSN_ALU_AND:
  DST = compute_and(SRC, SRC2, DST);
  CLEAR_CARRY;
  CONT;

INSN_ALU_INV_AND:
  DST = compute_inv_and(SRC, SRC2, DST);
  CLEAR_CARRY;
  CONT;

INSN_ALU_AND_INV:
  DST = compute_inv_and(SRC2, SRC, DST); // note backwards
  CLEAR_CARRY;
  CONT;

INSN_ALU_OR:
  DST = compute_or(SRC, SRC2, DST);
  CLEAR_CARRY;
  CONT;

INSN_ALU_XOR:
  DST = compute_xor(SRC, SRC2, DST);
  CLEAR_CARRY;
  CONT;

// #undef COND_JMP
// #define COND_JMP(SUFFIX, OP)                    \
//   INSN_JMP##SUFFIX:                             \
//       if (DST OP SRC)                           \
//         insn += insn->_args[2];                 \
//   CONT;

//   COND_JMP(EQ, == )
//   COND_JMP(GT, > )
//   COND_JMP(GE, >= )
//   COND_JMP(LT, < )
//   COND_JMP(LE, <= )

error_label:
  cout << "Error in processing instruction; unknown opcode" << endl;
  return; /* return the default output */

out:
  //cout << "Error: program terminated without RET; returning R0" << endl;
  output.reg = ps._regs[0];
  return;
}
