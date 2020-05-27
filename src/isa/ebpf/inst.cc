#include <iostream>
#include <cassert>
#include <string.h>
#include "inst.h"

using namespace std;

vector<int32_t> inst::_sample_neg_imms;
vector<int32_t> inst::_sample_pos_imms;

inst::inst(int opcode, int32_t arg1, int32_t arg2, int32_t arg3) {
  int32_t arg[3] = {arg1, arg2, arg3};
  _opcode = opcode;
  _dst_reg = 0;
  _src_reg = 0;
  _imm = 0;
  _off = 0;
  for (int i = 0; i < MAX_OP_LEN; i++) {
    switch (OPTYPE(opcode, i)) {
      case OP_DST_REG: _dst_reg = arg[i]; break;
      case OP_SRC_REG: _src_reg = arg[i]; break;
      case OP_OFF: _off = arg[i]; break;
      case OP_IMM: _imm = arg[i]; break;
      default: break;
    }
  }
}

void inst::to_abs_bv(vector<op_t>& abs_vec) const {
  abs_vec.push_back(_opcode);
  abs_vec.push_back(_dst_reg);
  abs_vec.push_back(_src_reg);
  abs_vec.push_back(_off);
  abs_vec.push_back(_imm);
}

int inst::get_opcode_by_idx(int idx) const {
  return idx_2_opcode[idx];
}

int inst::get_operand(int op_index) const {
  assert(op_index < MAX_OP_LEN);
  int operand_type = OPTYPE(_opcode, op_index);
  switch (operand_type) {
    case OP_DST_REG: return _dst_reg;
    case OP_SRC_REG: return _src_reg;
    case OP_OFF: return _off;
    case OP_IMM: return _imm;
    default: cout << "Error: get_operand cannot find operand_type for instruction: ";
      print();
      return 0;
  }
}

void inst::set_imm(int op_value) {
  // if it is the second operand of LE or BE, the op_value is the type index
  if ((_opcode == LE) || (_opcode == BE)) {
    int value_map[3] = {16, 32, 64};
    _imm = value_map[op_value];
  } else {
    unordered_set<int32_t> opcodes_set = {ADD64XC, MOV64XC, ADD32XC, OR32XC,
                                          AND32XC, MOV32XC, JEQXC, JGTXC, JSGTXC,
                                         };
    auto found = opcodes_set.find(_opcode);
    if (found == opcodes_set.end()) {
      _imm = op_value;
      return;
    }
    if (op_value < MIN_IMM) {
      int idx = op_value - MIN_IMM + _sample_neg_imms.size();
      assert(idx < _sample_neg_imms.size());
      _imm = _sample_neg_imms[idx];
    } else if (op_value > MAX_IMM) {
      int idx = op_value - MAX_IMM - 1;
      assert(idx < _sample_pos_imms.size());
      _imm = _sample_pos_imms[idx];
    } else {
      _imm = op_value;
    }
  }
}

int32_t inst::get_max_imm() const {
  switch (_opcode) {
    case ADD64XC:
    case MOV64XC:
    case ADD32XC:
    case OR32XC:
    case AND32XC:
    case MOV32XC:
    case JEQXC:
    case JGTXC:
    case JSGTXC: return MAX_IMM + _sample_pos_imms.size();
    case LSH64XC:
    case RSH64XC:
    case ARSH64XC: return MAX_IMM_SH64;
    case LSH32XC:
    case RSH32XC:
    case ARSH32XC: return MAX_IMM_SH32;
    case LE:
    case BE: return MAX_TYPES_IMM_ENDIAN;
    case CALL: return MAX_CALL_IMM;
    default: cout << "Error: no imm in instruction: ";
      print();
      return 0;
  }
}

int16_t inst::get_max_off(int inst_index) const {
  int op_type = get_opcode_type();
  switch (op_type) {
    case OP_LD:
    case OP_ST: return -1; // assume only stack
    case OP_UNCOND_JMP:
    case OP_COND_JMP: return min(MAX_OFF, inst::max_prog_len - inst_index - 2);
    default: cout << "Error: no off in instruction: ";
      print();
      return 0;
  }
}

int32_t inst::get_min_imm() const {
  switch (_opcode) {
    case ADD64XC:
    case MOV64XC:
    case ADD32XC:
    case OR32XC:
    case AND32XC:
    case MOV32XC:
    case JEQXC:
    case JGTXC:
    case JSGTXC: return MIN_IMM - _sample_neg_imms.size();
    case LSH64XC:
    case RSH64XC:
    case ARSH64XC:
    case LSH32XC:
    case RSH32XC:
    case ARSH32XC:
    case LE:
    case BE:
    case CALL: return 0;
    default: cout << "Error: no imm in instruction: ";
      print();
      return 0;
  }
}

int16_t inst::get_min_off() const {
  int op_type = get_opcode_type();
  switch (op_type) {
    case OP_LD:
    case OP_ST: return -STACK_SIZE; // assume only stack
    case OP_UNCOND_JMP:
    case OP_COND_JMP: return 0; // assume only jump forward
    default: cout << "Error: no off in instruction: ";
      print();
      return 0;
  }
}

void inst::set_operand(int op_index, op_t op_value) {
  assert(op_index < MAX_OP_LEN);
  int operand_type = OPTYPE(_opcode, op_index);
  switch (operand_type) {
    case OP_DST_REG: _dst_reg = op_value; return;
    case OP_SRC_REG: _src_reg = op_value; return;
    case OP_OFF: _off = op_value; return;
    case OP_IMM: set_imm(op_value); return;
    default: cout << "Error: set_operand cannot find operand_type for instruction: ";
      print();
      return;
  }
}

string inst::opcode_to_str(int opcode) const {
  switch (opcode) {
    case NOP: return "nop";
    case ADD64XC: return "addxc";
    case ADD64XY: return "addxy";
    case LSH64XC: return "lshxc";
    case LSH64XY: return "lshxy";
    case RSH64XC: return "rshxc";
    case RSH64XY: return "rshxy";
    case MOV64XC: return "movxc";
    case MOV64XY: return "movxy";
    case ARSH64XC: return "arshxc";
    case ARSH64XY: return "arshxy";
    case ADD32XC: return "add32xc";
    case ADD32XY: return "add32xy";
    case OR32XC: return "or32xc";
    case OR32XY: return "or32xy";
    case AND32XC: return "and32xc";
    case AND32XY: return "and32xy";
    case LSH32XC: return "lsh32xc";
    case LSH32XY: return "lsh32xy";
    case RSH32XC: return "rsh32xc";
    case RSH32XY: return "rsh32xy";
    case MOV32XC: return "mov32xc";
    case MOV32XY: return "mov32xy";
    case ARSH32XC: return "arsh32xc";
    case ARSH32XY: return "arsh32xy";
    case LE: return "le";
    case BE: return "be";
    case LDXB: return "ldxb";
    case STXB: return "stxb";
    case LDXH: return "ldxh";
    case STXH: return "stxh";
    case LDXW: return "ldxw";
    case STXW: return "stxw";
    case LDXDW: return "ldxdw";
    case STXDW: return "stxdw";
    case JA: return "ja";
    case JEQXC: return "jeqxc";
    case JEQXY: return "jeqxy";
    case JGTXC: return "jgtxc";
    case JGTXY: return "jgtxy";
    case JSGTXC: return "jsgtxc";
    case JSGTXY: return "jsgtxy";
    case CALL: return "call";
    case EXIT: return "exit";
    default: return "unknown opcode";
  }
}

void inst::print() const {
  cout << opcode_to_str(_opcode);
  for (int i = 0; i < get_num_operands(); i++) {
    cout << " " << get_operand(i);
  }
  cout << endl;
}

// get_canonical_reg_list return the registers that can be modified
vector<int> inst::get_canonical_reg_list() const {
  vector<int> reg_list;
  for (int i = 0; i < MAX_OP_LEN; i++) {
    // r10 cannot be modified, since r10 is the read-only frame pointer to access stack
    // reference: https://www.kernel.org/doc/Documentation/networking/filter.txt
    if (is_reg(i)) {
      int reg = get_operand(i);
      if ((reg != 10) && (reg != 1) && (reg != 0)) {
        reg_list.push_back(reg);
      }
    }
  }
  return reg_list;
}

vector<int> inst::get_isa_canonical_reg_list() {
  return vector<int> {2, 3, 4, 5, 6, 7, 8, 9};
}

// vector does not have the same number
void inst::sorted_vec_insert(int32_t num, vector<int32_t>& sorted_vec) {
  int size = sorted_vec.size();
  if (size <= 0) {
    sorted_vec.push_back(num);
    return;
  }

  if (sorted_vec[size - 1] < num) {
    sorted_vec.push_back(num);
    return;
  }

  for (int i = 0; i < sorted_vec.size(); i++) {
    if (sorted_vec[i] == num) return;
    if (sorted_vec[i] > num) {
      sorted_vec.push_back(0);
      for (int j = size - 1; j >= i; j--) sorted_vec[j + 1] = sorted_vec[j];
      sorted_vec[i] = num;
      return;
    }
  }
}

void inst::add_sample_imm(const vector<int32_t>& nums) {
  for (int i = 0; i < nums.size(); i++) {
    int32_t num = nums[i];
    if (num < MIN_IMM) {
      sorted_vec_insert(num, _sample_neg_imms);
    } else if (num > MAX_IMM) {
      sorted_vec_insert(num, _sample_pos_imms);
    }
  }
  for (int i = 0; i < _sample_neg_imms.size(); i++) cout << _sample_neg_imms[i] << " ";
  cout << endl;
  for (int i = 0; i < _sample_pos_imms.size(); i++) cout << _sample_pos_imms[i] << " ";
  cout << endl;
}

inst& inst::operator=(const inst &rhs) {
  _opcode = rhs._opcode;
  _dst_reg = rhs._dst_reg;
  _src_reg = rhs._src_reg;
  _off = rhs._off;
  _imm = rhs._imm;
  return *this;
}

bool inst::operator==(const inst &x) const {
  return ((_opcode == x._opcode) &&
          (_dst_reg == x._dst_reg) &&
          (_src_reg == x._src_reg) &&
          (_off == x._off) &&
          (_imm == x._imm));
}

// For jmp opcode, it can only jump forward
// TODO: modify the name
int32_t inst::get_max_operand_val(int op_index, int inst_index) const {
  int operand_type = OPTYPE(_opcode, op_index);
  switch (operand_type) {
    case OP_UNUSED: return 0;
    case OP_DST_REG: return NUM_REGS - 1;
    case OP_SRC_REG: return NUM_REGS - 1;
    case OP_OFF: return get_max_off(inst_index);
    case OP_IMM: return get_max_imm();
    default: cout << "Error: cannot find operand_type for instruction: ";
      // print();
      return 0;
  }
}

// For jmp opcode, it can only jump forward
int32_t inst::get_min_operand_val(int op_index, int inst_index) const {
  int operand_type = OPTYPE(_opcode, op_index);
  switch (operand_type) {
    case OP_UNUSED: return 0;
    case OP_DST_REG: return 0;
    case OP_SRC_REG: return 0;
    case OP_OFF: return get_min_off();
    case OP_IMM: return get_min_imm();
    default: cout << "Error: cannot find operand_type for instruction: ";
      print();
      return 0;
  }
}

int inst::get_jmp_dis() const {
  switch (get_opcode_type()) {
    case (OP_UNCOND_JMP): return _off;
    case (OP_COND_JMP): return _off;
    default: cout << "Error: opcode is not jmp" << endl; return 0;
  }
}

void inst::insert_jmp_opcodes(unordered_set<int>& jmp_set) const {
  jmp_set.insert(JA);
  jmp_set.insert(JEQXC);
  jmp_set.insert(JEQXY);
  jmp_set.insert(JGTXC);
  jmp_set.insert(JGTXY);
  jmp_set.insert(JSGTXC);
  jmp_set.insert(JSGTXY);
}

int inst::inst_output_opcode_type() const {
  switch (_opcode) {
    case EXIT: return RET_X;
    default: /* cout << "Error: opcode is not EXIT" << endl; */ return RET_X;
  }
}

int inst::inst_output() const {
  switch (_opcode) {
    case EXIT: return 0;
    default: /* cout << "Error: opcode is not EXIT" << endl; */ return 0;
  }
}

bool inst::is_real_inst() const {
  if (_opcode == NOP) return false;
  return true;
}

bool inst::is_reg(int op_index) const {
  int op_type = OPTYPE(_opcode, op_index);
  if ((op_type == OP_DST_REG) || (op_type == OP_SRC_REG)) return true;
  return false;
}

int inst::implicit_ret_reg() const {
  return -1;
}

void inst::set_as_nop_inst() {
  _opcode = NOP;
  _dst_reg = 0;
  _src_reg = 0;
  _off = 0;
  _imm = 0;
}

// z3 64-bit bv
#define NEWDST newDst
#define CURDST curDst
#define CURSRC curSrc
#define MEM sv.mem_var
#define IMM to_expr(imm)
#define OFF to_expr(off)
#define CURSRC_L6 (CURSRC & to_expr((int64_t)0x3f))
#define CURSRC_L5 (CURSRC & to_expr((int64_t)0x1f))
#define R0 newDst
#define R1 sv.get_cur_reg_var(1)
#define R2 sv.get_cur_reg_var(2)
#define R3 sv.get_cur_reg_var(3)
#define R4 sv.get_cur_reg_var(4)
#define R5 sv.get_cur_reg_var(5)

z3::expr inst::smt_inst(smt_var& sv) const {
  // check whether opcode is valid. If invalid, curDst cannot be updated to get newDst
  // If opcode is valid, then define curDst, curSrc, imm and newDst
  int op_type = get_opcode_type();
  if ((op_type != OP_OTHERS) && (op_type != OP_LD) && (op_type != OP_ST))
    return string_to_expr("false");
  // Get curDst, curSrc, imm and newDst at the begining to avoid using switch case to
  // get some of these values for different opcodes. Should get curDst and curSrc before
  // updating curDst (curSrc may be the same reg as curDst)
  z3::expr curDst = sv.get_cur_reg_var(_dst_reg);
  z3::expr curSrc = sv.get_cur_reg_var(_src_reg);
  z3::expr newDst = string_to_expr("false");
  // update register according to the opcode type
  if (_opcode == CALL) {
    newDst = sv.update_reg_var(0); // r0 contains return value
  } else if ((op_type == OP_OTHERS) || (op_type == OP_LD)) {
    newDst = sv.update_reg_var(_dst_reg);
  }
  int64_t imm = (int64_t)_imm;
  int64_t off = (int64_t)_off;

  switch (_opcode) {
    case ADD64XC: return predicate_add(CURDST, IMM, NEWDST);
    case ADD64XY: return predicate_add(CURDST, CURSRC, NEWDST);
    case LSH64XC: return predicate_lsh(CURDST, IMM, NEWDST);
    case LSH64XY: return predicate_lsh(CURDST, CURSRC_L6, NEWDST);
    case RSH64XC: return predicate_rsh(CURDST, IMM, NEWDST);
    case RSH64XY: return predicate_rsh(CURDST, CURSRC_L6, NEWDST);
    case MOV64XC: return predicate_mov(IMM, NEWDST);
    case MOV64XY: return predicate_mov(CURSRC, NEWDST);
    case ARSH64XC: return predicate_arsh(CURDST, IMM, NEWDST);
    case ARSH64XY: return predicate_arsh(CURDST, CURSRC_L6, NEWDST);
    case ADD32XC: return predicate_add32(CURDST, IMM, NEWDST);
    case ADD32XY: return predicate_add32(CURDST, CURSRC, NEWDST);
    case OR32XC: return predicate_or32(CURDST, IMM, NEWDST);
    case OR32XY: return predicate_or32(CURDST, CURSRC, NEWDST);
    case AND32XC: return predicate_and32(CURDST, IMM, NEWDST);
    case AND32XY: return predicate_and32(CURDST, CURSRC, NEWDST);
    case LSH32XC: return predicate_lsh32(CURDST, IMM, NEWDST);
    case LSH32XY: return predicate_lsh32(CURDST, CURSRC_L5, NEWDST);
    case RSH32XC: return predicate_rsh32(CURDST, IMM, NEWDST);
    case RSH32XY: return predicate_rsh32(CURDST, CURSRC_L5, NEWDST);
    case MOV32XC: return predicate_mov32(IMM, NEWDST);
    case MOV32XY: return predicate_mov32(CURSRC, NEWDST);
    case ARSH32XC: return predicate_arsh32(CURDST, IMM, NEWDST);
    case ARSH32XY: return predicate_arsh32(CURDST, CURSRC_L5, NEWDST);
    case LE:
      switch (imm) {
        case 16: return predicate_le16(CURDST, NEWDST);
        case 32: return predicate_le32(CURDST, NEWDST);
        case 64: return predicate_le64(CURDST, NEWDST);
        default: cout << "Error: imm " << imm << " is not 16, 32, 64" << endl;
          return string_to_expr("false");
      }
    case BE:
      switch (imm) {
        case 16: return predicate_be16(CURDST, NEWDST);
        case 32: return predicate_be32(CURDST, NEWDST);
        case 64: return predicate_be64(CURDST, NEWDST);
        default: cout << "Error: imm " << imm << " is not 16, 32, 64" << endl;
          return string_to_expr("false");
      }
    case LDXB: return predicate_ld8(CURSRC, OFF, sv, NEWDST);
    case LDXH: return predicate_ld16(CURSRC, OFF, sv, NEWDST);
    case LDXW: return predicate_ld32(CURSRC, OFF, sv, NEWDST);
    case LDXDW: return predicate_ld64(CURSRC, OFF, sv, NEWDST);
    case STXB: predicate_st8(CURSRC, CURDST, OFF, MEM); return string_to_expr("true");
    case STXH: predicate_st16(CURSRC, CURDST, OFF, MEM); return string_to_expr("true");
    case STXW: predicate_st32(CURSRC, CURDST, OFF, MEM); return string_to_expr("true");
    case STXDW: predicate_st64(CURSRC, CURDST, OFF, MEM); return string_to_expr("true");
    case CALL: return predicate_helper_function(imm, R1, R2, R3, R4, R5, R0, sv);
    default: return string_to_expr("false");
  }
}

z3::expr inst::smt_inst_jmp(smt_var& sv) const {
  // If opcode is valid, then define curDst, curSrc, imm
  if (get_opcode_type() != OP_COND_JMP) return string_to_expr("false");
  z3::expr curDst = sv.get_cur_reg_var(_dst_reg);
  z3::expr curSrc = sv.get_cur_reg_var(_src_reg);
  int64_t imm = (int64_t)_imm;

  switch (_opcode) {
    case JEQXC: return (CURDST == IMM);
    case JEQXY: return (CURDST == CURSRC);
    case JGTXC: return (ugt(CURDST, IMM));
    case JGTXY: return (ugt(CURDST, CURSRC));
    case JSGTXC: return (CURDST > IMM);
    case JSGTXY: return (CURDST > CURSRC);
    default: return string_to_expr("false");
  }
}

int opcode_2_idx(int opcode) {
  switch (opcode) {
    case NOP: return IDX_NOP;
    case ADD64XC: return IDX_ADD64XC;
    case ADD64XY: return IDX_ADD64XY;
    case LSH64XC: return IDX_LSH64XC;
    case LSH64XY: return IDX_LSH64XY;
    case RSH64XC: return IDX_RSH64XC;
    case RSH64XY: return IDX_RSH64XY;
    case MOV64XC: return IDX_MOV64XC;
    case MOV64XY: return IDX_MOV64XY;
    case ARSH64XC: return IDX_ARSH64XC;
    case ARSH64XY: return IDX_ARSH64XY;
    case ADD32XC: return IDX_ADD32XC;
    case ADD32XY: return IDX_ADD32XY;
    case OR32XC: return IDX_OR32XC;
    case OR32XY: return IDX_OR32XY;
    case AND32XC: return IDX_AND32XC;
    case AND32XY: return IDX_AND32XY;
    case LSH32XC: return IDX_LSH32XC;
    case LSH32XY: return IDX_LSH32XY;
    case RSH32XC: return IDX_RSH32XC;
    case RSH32XY: return IDX_RSH32XY;
    case MOV32XC: return IDX_MOV32XC;
    case MOV32XY: return IDX_MOV32XY;
    case ARSH32XC: return IDX_ARSH32XC;
    case ARSH32XY: return IDX_ARSH32XY;
    case LE: return IDX_LE;
    case BE: return IDX_BE;
    case LDXB: return IDX_LDXB;
    case STXB: return IDX_STXB;
    case LDXH: return IDX_LDXH;
    case STXH: return IDX_STXH;
    case LDXW: return IDX_LDXW;
    case STXW: return IDX_STXW;
    case LDXDW: return IDX_LDXDW;
    case STXDW: return IDX_STXDW;
    case JA: return IDX_JA;
    case JEQXC: return IDX_JEQXC;
    case JEQXY: return IDX_JEQXY;
    case JGTXC: return IDX_JGTXC;
    case JGTXY: return IDX_JGTXY;
    case JSGTXC: return IDX_JSGTXC;
    case JSGTXY: return IDX_JSGTXY;
    case CALL: return IDX_CALL;
    case EXIT: return IDX_EXIT;
    default: cout << "unknown opcode" << endl; return 0;
  }
}

// TODO: set the stack memory as 0
z3::expr inst::smt_set_pre(z3::expr input, smt_var& sv) {
  z3::expr f = string_to_expr("true");
  f = (sv.get_cur_reg_var(1) == input) &&
      (sv.get_cur_reg_var(10) == sv.get_stack_bottom_addr()) &&
      sv.stack_start_constrain() &&
      (sv.get_cur_reg_var(0) == 0);
  for (size_t i = 2; i < 10; i++) {
    f = f && (sv.get_cur_reg_var(i) == 0);
  }
  return f;
}

string inst::get_bytecode_str() const {
  return ("{"
          + to_string(_opcode) + ", " + to_string(_dst_reg) + ", "
          + to_string(_src_reg) + ", " + to_string(_off) + ", "
          + to_string(_imm)
          + "}");
}

void interpret(inout_t& output, inst* program, int length, prog_state &ps, const inout_t& input) {
#undef IMM
#undef OFF
#undef MEM
#undef R0
#undef R1
#undef R2
#undef R3
#undef R4
#undef R5
// type: int64_t
#define DST ps._regs[insn->_dst_reg]
#define SRC ps._regs[insn->_src_reg]
#define IMM (int64_t)insn->_imm
#define SRC_L6 L6(SRC)
#define SRC_L5 L5(SRC)
#define OFF (int64_t)insn->_off

// type: uint64_t
#define UDST (uint64_t)DST
#define USRC (uint64_t)SRC
#define UIMM (uint64_t)IMM

#define MEM ps._mem
#define R0 ps._regs[0]
#define R1 ps._regs[1]
#define R2 ps._regs[2]
#define R3 ps._regs[3]
#define R4 ps._regs[4]
#define R5 ps._regs[5]
#define SIMU_R10 simu_r10
#define REAL_R10 real_r10

#define ALU_UNARY(OPCODE, OP, OPERAND)                             \
  INSN_##OPCODE:                                                   \
    DST = compute_##OP(OPERAND);                                   \
    CONT;

#define ALU_BINARY(OPCODE, OP, OPERAND1, OPERAND2)                 \
  INSN_##OPCODE:                                                   \
    DST = compute_##OP(OPERAND1, OPERAND2);                        \
    CONT;

#define LDST(SIZEOP, SIZE)                                         \
  INSN_LDX##SIZEOP:                                                \
    real_addr = get_real_addr_by_simu(SRC, SIMU_R10, REAL_R10);    \
    ps._mem.memory_access_check(real_addr + OFF, SIZE/8);          \
    DST = compute_ld##SIZE(real_addr, OFF);                        \
    CONT;                                                          \
  INSN_STX##SIZEOP:                                                \
    real_addr = get_real_addr_by_simu(DST, SIMU_R10, REAL_R10);    \
    ps._mem.memory_access_check(real_addr + OFF, SIZE/8);          \
    compute_st##SIZE(SRC, real_addr, OFF);                         \
    CONT;

#define BYTESWAP(OPCODE, OP)                                       \
  INSN_##OPCODE:                                                   \
    switch (IMM) {                                                 \
      case 16: DST = compute_##OP##16(DST);break;                  \
      case 32: DST = compute_##OP##32(DST);break;                  \
      case 64: DST = compute_##OP##64(DST);break;                  \
      default: cout << "[Error] imm " << IMM                       \
                    << " is not 16, 32, 64" << endl;               \
               break;                                              \
    }                                                              \
    CONT;

#define COND_JMP(OPCODE, OP, OPERAND1, OPERAND2)                   \
  INSN_##OPCODE:                                                   \
    if (OPERAND1 OP OPERAND2)                                      \
      insn += OFF;                                                 \
  CONT;

  inst* insn = program;
  ps.clear();
  // set real_r10 as frame pointer, the bottom of the stack
  uint64_t real_r10 = (uint64_t)ps._mem.get_stack_bottom_addr();
  // register r10 is set by update_ps_by_input
  update_ps_by_input(ps, input);
  uint64_t simu_r10 = (uint64_t)ps._regs[10];
  uint64_t real_addr = 0; // used as temporary variable in instruction execution

  static void *jumptable[NUM_INSTR] = {
    [IDX_NOP]      = && INSN_NOP,
    [IDX_ADD64XC]  = && INSN_ADD64XC,
    [IDX_ADD64XY]  = && INSN_ADD64XY,
    [IDX_LSH64XC]  = && INSN_LSH64XC,
    [IDX_LSH64XY]  = && INSN_LSH64XY,
    [IDX_RSH64XC]  = && INSN_RSH64XC,
    [IDX_RSH64XY]  = && INSN_RSH64XY,
    [IDX_MOV64XC]  = && INSN_MOV64XC,
    [IDX_MOV64XY]  = && INSN_MOV64XY,
    [IDX_ARSH64XC] = && INSN_ARSH64XC,
    [IDX_ARSH64XY] = && INSN_ARSH64XY,
    [IDX_ADD32XC]  = && INSN_ADD32XC,
    [IDX_ADD32XY]  = && INSN_ADD32XY,
    [IDX_OR32XC]   = && INSN_OR32XC,
    [IDX_OR32XY]   = && INSN_OR32XY,
    [IDX_AND32XC]  = && INSN_AND32XC,
    [IDX_AND32XY]  = && INSN_AND32XY,
    [IDX_LSH32XC]  = && INSN_LSH32XC,
    [IDX_LSH32XY]  = && INSN_LSH32XY,
    [IDX_RSH32XC]  = && INSN_RSH32XC,
    [IDX_RSH32XY]  = && INSN_RSH32XY,
    [IDX_MOV32XC]  = && INSN_MOV32XC,
    [IDX_MOV32XY]  = && INSN_MOV32XY,
    [IDX_ARSH32XC] = && INSN_ARSH32XC,
    [IDX_ARSH32XY] = && INSN_ARSH32XY,
    [IDX_LE]       = && INSN_LE,
    [IDX_BE]       = && INSN_BE,
    [IDX_LDXB]     = && INSN_LDXB,
    [IDX_STXB]     = && INSN_STXB,
    [IDX_LDXH]     = && INSN_LDXH,
    [IDX_STXH]     = && INSN_STXH,
    [IDX_LDXW]     = && INSN_LDXW,
    [IDX_STXW]     = && INSN_STXW,
    [IDX_LDXDW]    = && INSN_LDXDW,
    [IDX_STXDW]    = && INSN_STXDW,
    [IDX_JA]       = && INSN_JA,
    [IDX_JEQXC]    = && INSN_JEQXC,
    [IDX_JEQXY]    = && INSN_JEQXY,
    [IDX_JGTXC]    = && INSN_JGTXC,
    [IDX_JGTXY]    = && INSN_JGTXY,
    [IDX_JSGTXC]   = && INSN_JSGTXC,
    [IDX_JSGTXY]   = && INSN_JSGTXY,
    [IDX_CALL]     = && INSN_CALL,
    [IDX_EXIT]     = && INSN_EXIT,
  };

#define CONT {                                                     \
      insn++;                                                      \
      if (insn < program + length) {                               \
        goto *jumptable[opcode_2_idx(insn->_opcode)];              \
      } else goto out;                                             \
  }

select_insn:
  goto *jumptable[opcode_2_idx(insn->_opcode)];

INSN_NOP:
  CONT;

  ALU_UNARY(MOV64XC, mov, IMM)
  ALU_UNARY(MOV64XY, mov, SRC)
  ALU_BINARY(ADD64XC, add, DST, IMM)
  ALU_BINARY(ADD64XY, add, DST, SRC)
  ALU_BINARY(LSH64XC, lsh, DST, IMM)
  ALU_BINARY(LSH64XY, lsh, DST, SRC_L6)
  ALU_BINARY(RSH64XC, rsh, DST, IMM)
  ALU_BINARY(RSH64XY, rsh, DST, SRC_L6)
  ALU_BINARY(ARSH64XC, arsh, DST, IMM)
  ALU_BINARY(ARSH64XY, arsh, DST, SRC_L6)

  ALU_UNARY(MOV32XC, mov32, IMM)
  ALU_UNARY(MOV32XY, mov32, SRC)
  ALU_BINARY(ADD32XC, add32, DST, IMM)
  ALU_BINARY(ADD32XY, add32, DST, SRC)
  ALU_BINARY(OR32XC, or32, DST, IMM)
  ALU_BINARY(OR32XY, or32, DST, SRC)
  ALU_BINARY(AND32XC, and32, DST, IMM)
  ALU_BINARY(AND32XY, and32, DST, SRC)
  ALU_BINARY(LSH32XC, lsh32, DST, IMM)
  ALU_BINARY(LSH32XY, lsh32, DST, SRC_L5)
  ALU_BINARY(RSH32XC, rsh32, DST, IMM)
  ALU_BINARY(RSH32XY, rsh32, DST, SRC_L5)
  ALU_BINARY(ARSH32XC, arsh32, DST, IMM)
  ALU_BINARY(ARSH32XY, arsh32, DST, SRC_L5)

  LDST(B,  8)
  LDST(H,  16)
  LDST(W,  32)
  LDST(DW, 64)

  BYTESWAP(LE, le)
  BYTESWAP(BE, be)

INSN_JA:
  insn += OFF;
  CONT;

  COND_JMP(JEQXC, ==, DST, IMM)
  COND_JMP(JEQXY, ==, DST, SRC)
  COND_JMP(JGTXC, >, UDST, UIMM)
  COND_JMP(JGTXY, >, UDST, USRC)
  COND_JMP(JSGTXC, >, DST, IMM)
  COND_JMP(JSGTXY, >, DST, SRC)

INSN_CALL:
  R0 = compute_helper_function(IMM, R1, R2, R3, R4, R5, MEM, SIMU_R10, REAL_R10);
  CONT;

INSN_EXIT:
  update_output_by_ps(output, ps);
  return;

error_label:
  cout << "Error in processing instruction; unknown opcode" << endl;
  return; /* return default output value */

out:
  update_output_by_ps(output, ps);
  return;
}
