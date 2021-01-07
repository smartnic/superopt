#include <iostream>
#include <fstream>
#include <cassert>
#include <string.h>
#include "inst.h"

using namespace std;

vector<int32_t> inst::_sample_neg_imms;
vector<int32_t> inst::_sample_pos_imms;
vector<int32_t> inst::_sample_neg_offs;
vector<int32_t> inst::_sample_pos_offs;
unordered_map<string, double> inst::_runtime;

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

bool inst::sample_unmodifiable() const {
  if ((_opcode == CALL) && (_imm == BPF_FUNC_get_prandom_u32)) return true;
  return false;
}

void inst::set_imm(int op_value) {
  // if it is the second operand of LE or BE, the op_value is the type index
  if ((_opcode == LE) || (_opcode == BE)) {
    int value_map[3] = {16, 32, 64};
    _imm = value_map[op_value];
    return;
  }
  if (_opcode == CALL) {
    assert(op_value < SAMPLE_BPF_FUNC_MAX_ID);
    _imm = sample_bpf_func[op_value];
    return;
  }
  // opcodes_set constains opcodes whose sample imm range = the default imm range + imms from source program
  unordered_set<int32_t> opcodes_set = {ADD64XC, OR64XC, AND64XC, MOV64XC, ADD32XC, OR32XC,
                                        AND32XC, MOV32XC, STB, STH, STW, STDW, LDABSH,
                                        JEQXC, JGTXC, JNEXC, JSGTXC, JEQ32XC, JNE32XC,
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

void inst::set_off(int op_value) {
  int op_type = get_opcode_type();
  if ((op_type == OP_UNCOND_JMP) || (op_type == OP_COND_JMP)) {
    _off = op_value;
    return;
  }
  // deal with the case: OP_ST and OP_LD
  if (op_value < MIN_OFF) {
    int idx = op_value - MIN_OFF + _sample_neg_offs.size();
    assert(idx < _sample_neg_offs.size());
    _off = _sample_neg_offs[idx];
  } else if (op_value > MAX_OFF) {
    int idx = op_value - MAX_OFF - 1;
    assert(idx < _sample_pos_offs.size());
    _off = _sample_pos_offs[idx];
  } else {
    _off = op_value;
  }
}

int32_t inst::get_max_imm() const {
  switch (_opcode) {
    case ADD64XC:
    case OR64XC:
    case AND64XC:
    case MOV64XC:
    case ADD32XC:
    case OR32XC:
    case AND32XC:
    case MOV32XC:
    case STB:
    case STH:
    case STW:
    case STDW:
    case LDABSH:
    case JEQXC:
    case JGTXC:
    case JNEXC:
    case JSGTXC:
    case JEQ32XC:
    case JNE32XC: return MAX_IMM + _sample_pos_imms.size();
    case LSH64XC:
    case RSH64XC:
    case ARSH64XC: return MAX_IMM_SH64;
    case LSH32XC:
    case RSH32XC:
    case ARSH32XC: return MAX_IMM_SH32;
    case LE:
    case BE: return MAX_TYPES_IMM_ENDIAN;
    case LDMAPID: return mem_t::maps_number() - 1;
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
    case OP_ST: return MAX_OFF + _sample_pos_offs.size();
    case OP_UNCOND_JMP:
    case OP_COND_JMP: return inst::max_prog_len - inst_index - 2;
    default: cout << "Error: no off in instruction: ";
      print();
      return 0;
  }
}

int32_t inst::get_min_imm() const {
  switch (_opcode) {
    case ADD64XC:
    case OR64XC:
    case AND64XC:
    case MOV64XC:
    case ADD32XC:
    case OR32XC:
    case AND32XC:
    case MOV32XC:
    case STB:
    case STH:
    case STW:
    case STDW:
    case LDABSH:
    case JEQXC:
    case JGTXC:
    case JNEXC:
    case JSGTXC:
    case JEQ32XC:
    case JNE32XC: return MIN_IMM - _sample_neg_imms.size();
    case LSH64XC:
    case RSH64XC:
    case ARSH64XC:
    case LSH32XC:
    case RSH32XC:
    case ARSH32XC:
    case LE:
    case BE: return 0;
    case LDMAPID: return 0;
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
    case OP_ST: return MIN_OFF - _sample_neg_offs.size();
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
    case OP_OFF: set_off(op_value); return;
    case OP_IMM: set_imm(op_value); return;
    default: cout << "Error: set_operand cannot find operand_type for instruction: ";
      print();
      return;
  }
}

string inst::swap_byte_to_str(int opcode, int imm) {
  if ((opcode != LE) && (opcode != BE)) {
    return "unknown swap byte opcode";
  }
  if ((imm != 16) && (imm != 32) && (imm != 64)) {
    return "unknown swap byte immediate number";
  }
  string str = "LE";
  if (opcode == BE) {
    str = "BE";
  }
  str = str + to_string(imm);
  return str;
}

#define MAPPER(func) case BPF_FUNC_##func: return string("BPF_FUNC_") + #func;
string inst::func_to_str(int func_id) {
  switch (func_id) {
      MAPPER(map_lookup_elem)
      MAPPER(map_update_elem)
      MAPPER(map_delete_elem)
      MAPPER(tail_call)
      MAPPER(get_prandom_u32)
    default: return "unknown function id";
  }
}
#undef MAPPER

#define MAPPER(OP) case OP: return #OP;
string inst::opcode_to_str(int opcode) {
  switch (opcode) {
      MAPPER(NOP)
      MAPPER(ADD64XC)
      MAPPER(ADD64XY)
      MAPPER(OR64XC)
      MAPPER(OR64XY)
      MAPPER(AND64XC)
      MAPPER(AND64XY)
      MAPPER(LSH64XC)
      MAPPER(LSH64XY)
      MAPPER(RSH64XC)
      MAPPER(RSH64XY)
      MAPPER(MOV64XC)
      MAPPER(MOV64XY)
      MAPPER(ARSH64XC)
      MAPPER(ARSH64XY)
      MAPPER(ADD32XC)
      MAPPER(ADD32XY)
      MAPPER(OR32XC)
      MAPPER(OR32XY)
      MAPPER(AND32XC)
      MAPPER(AND32XY)
      MAPPER(LSH32XC)
      MAPPER(LSH32XY)
      MAPPER(RSH32XC)
      MAPPER(RSH32XY)
      MAPPER(MOV32XC)
      MAPPER(MOV32XY)
      MAPPER(ARSH32XC)
      MAPPER(ARSH32XY)
      MAPPER(LE)
      MAPPER(BE)
      MAPPER(LDMAPID)
      MAPPER(LDXB)
      MAPPER(STXB)
      MAPPER(LDXH)
      MAPPER(STXH)
      MAPPER(LDXW)
      MAPPER(STXW)
      MAPPER(STB)
      MAPPER(STH)
      MAPPER(STW)
      MAPPER(STDW)
      MAPPER(LDXDW)
      MAPPER(STXDW)
      MAPPER(XADD64)
      MAPPER(XADD32)
      MAPPER(LDABSH)
      MAPPER(LDINDH)
      MAPPER(JA)
      MAPPER(JEQXC)
      MAPPER(JEQXY)
      MAPPER(JGTXC)
      MAPPER(JGTXY)
      MAPPER(JNEXC)
      MAPPER(JNEXY)
      MAPPER(JSGTXC)
      MAPPER(JSGTXY)
      MAPPER(JEQ32XC)
      MAPPER(JEQ32XY)
      MAPPER(JNE32XC)
      MAPPER(JNE32XY)
      MAPPER(CALL)
      MAPPER(EXIT)
    default: return "unknown opcode";
  }
}
#undef MAPPER

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
}

void inst::add_sample_off(const vector<int16_t>& nums) {
  for (int i = 0; i < nums.size(); i++) {
    int16_t num = nums[i];
    if (num < MIN_OFF) {
      sorted_vec_insert(num, _sample_neg_offs);
    } else if (num > MAX_OFF) {
      sorted_vec_insert(num, _sample_pos_offs);
    }
  }
}

// read runtime from inst.runtime
void inst::init_runtime() {
  ifstream file("./src/isa/ebpf/inst.runtime");
  string line;
  double default_runtime = 1; // set the default runtime as 1 ns
  for (int i = 0; i < NUM_INSTR; i++) {
    int op = idx_2_opcode[i];
    if (op == CALL) continue;
    _runtime[opcode_to_str(op)] = default_runtime;
  }
  for (int i = 0; i < SP_BPF_FUNC_MAX_ID; i++) {
    int func_id = sp_bpf_func[i];
    _runtime[func_to_str(func_id)] = default_runtime;
  }
  int opcodes[2] = {LE, BE};
  int imms[3] = {16, 32, 64};
  for (int i = 0; i < 2; i++) {
    for (int j = 0; j < 3; j++) {
      _runtime[swap_byte_to_str(opcodes[i], imms[j])] = default_runtime;
    }
  }
  // set special opcodes
  _runtime["NOP"] = 0;
  _runtime["EXIT"] = 0;
  if (! file) {
    string err_msg = "Error: cannot open ./src/isa/ebpf/inst.runtime";
    throw (err_msg);
  }
  while (getline(file, line)) {
    vector<string> vec;
    string delimiter = " ";
    split_string(line, vec, delimiter);
    if (vec.size() != 2) continue;
    auto found = _runtime.find(vec[0]);
    if (found != _runtime.end()) {
      found->second = stod(vec[1]);
    }
  }
}

double inst::get_runtime() const {
  string str;
  if ((_opcode == LE) || (_opcode == BE)) {
    str = swap_byte_to_str(_opcode, _imm);
  } else if (_opcode == CALL) {
    str = func_to_str(_imm); // _imm is the function id
  } else {
    str = opcode_to_str(_opcode);
  }
  auto found = _runtime.find(str);
  if (found == _runtime.end()) {
    string err_msg = string("Error: cannot get runtime of ") + str;
    throw (err_msg);
  }
  return found->second;
}

inst& inst::operator=(const inst & rhs) {
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

int32_t inst::get_max_sample_dst_reg() const {
  // 1. memory write: r1 ~ r10
  // 2. register write: r1 ~ r9
  // 3. todo: check jmp
  int op_class = BPF_CLASS(_opcode);
  if ((op_class == BPF_ST) || (op_class == BPF_STX)) {
    return NUM_REGS - 1;
  }
  return NUM_REGS - 2;
}

// For jmp opcode, it can only jump forward
// TODO: modify the name
int32_t inst::get_max_operand_val(int op_index, int inst_index) const {
  int operand_type = OPTYPE(_opcode, op_index);
  switch (operand_type) {
    case OP_UNUSED: return 0;
    case OP_DST_REG: return get_max_sample_dst_reg();
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

void inst::set_jmp_dis(int off) {
  switch (get_opcode_type()) {
    case (OP_UNCOND_JMP): _off = off; return;
    case (OP_COND_JMP): _off = off; return;
    default: cout << "Error: opcode is not jmp" << endl; return;
  }
}

void inst::insert_jmp_opcodes(unordered_set<int>& jmp_set) const {
  jmp_set.insert(IDX_JA);
  jmp_set.insert(IDX_JEQXC);
  jmp_set.insert(IDX_JEQXY);
  jmp_set.insert(IDX_JGTXC);
  jmp_set.insert(IDX_JGTXY);
  jmp_set.insert(IDX_JNEXC);
  jmp_set.insert(IDX_JNEXY);
  jmp_set.insert(IDX_JSGTXC);
  jmp_set.insert(IDX_JSGTXY);
  jmp_set.insert(IDX_JEQ32XC);
  jmp_set.insert(IDX_JEQ32XY);
  jmp_set.insert(IDX_JNE32XC);
  jmp_set.insert(IDX_JNE32XY);
}

void inst::insert_opcodes_not_gen(unordered_set<int>& opcode_set) const {
  opcode_set.insert(IDX_CALL);
  opcode_set.insert(IDX_LDMAPID);
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
#define IMM to_expr(imm)
#define OFF to_expr(off)
#define CURDST_L32 (CURDST & to_expr((int64_t)0xffffffff))
#define CURSRC_L32 (CURSRC & to_expr((int64_t)0xffffffff))
#define IMM_L32 (IMM & to_expr((int64_t)0xffffffff))
#define CURSRC_L6 (CURSRC & to_expr((int64_t)0x3f))
#define CURSRC_L5 (CURSRC & to_expr((int64_t)0x1f))
#define R0 newDst
#define R1 sv.get_cur_reg_var(1)
#define R2 sv.get_cur_reg_var(2)
#define R3 sv.get_cur_reg_var(3)
#define R4 sv.get_cur_reg_var(4)
#define R5 sv.get_cur_reg_var(5)

z3::expr inst::smt_inst(smt_var & sv, unsigned int block) const {
  // cout << endl << "...... block id: " << block << endl;;
  // print();
  // check whether opcode is valid. If invalid, curDst cannot be updated to get newDst
  // If opcode is valid, then define curDst, curSrc, imm and newDst
  int op_type = get_opcode_type();
  if ((op_type != OP_OTHERS) && (op_type != OP_LD) &&
      (op_type != OP_ST) && (op_type != OP_CALL))
    return Z3_true;
  if ((op_type == OP_CALL) && (_imm == BPF_FUNC_tail_call)) {
    return Z3_true;
  }
  bool enable_addr_off = smt_var::enable_addr_off;
  bool is_win = smt_var::is_win;
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
  z3::expr path_cond = sv.mem_var.get_block_path_cond(block);
  // todo: move the track of pointers in predicate functions
  switch (_opcode) {
    case ADD64XC: sv.mem_var.add_ptr(NEWDST, CURDST, IMM, path_cond); return predicate_add(CURDST, IMM, NEWDST);
    case ADD64XY: sv.mem_var.add_ptr(NEWDST, CURDST, CURSRC, path_cond); return predicate_add(CURDST, CURSRC, NEWDST);
    case OR64XC: return predicate_or(CURDST, IMM, NEWDST);
    case OR64XY: return predicate_or(CURDST, CURSRC, NEWDST);
    case AND64XC: return predicate_and(CURDST, IMM, NEWDST);
    case AND64XY: return predicate_and(CURDST, CURSRC, NEWDST);
    case LSH64XC: return predicate_lsh(CURDST, IMM, NEWDST);
    case LSH64XY: return predicate_lsh(CURDST, CURSRC_L6, NEWDST);
    case RSH64XC: return predicate_rsh(CURDST, IMM, NEWDST);
    case RSH64XY: return predicate_rsh(CURDST, CURSRC_L6, NEWDST);
    case MOV64XC: return predicate_mov(IMM, NEWDST);
    case MOV64XY: sv.mem_var.add_ptr(NEWDST, CURSRC, ZERO_ADDR_OFF_EXPR, path_cond); return predicate_mov(CURSRC, NEWDST);
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
    case LDMAPID: return predicate_ldmapid(IMM, NEWDST, sv, block);
    case LDXB: return predicate_ld8(CURSRC, OFF, sv, NEWDST, block, enable_addr_off, is_win);
    case LDXH: return predicate_ld16(CURSRC, OFF, sv, NEWDST, block, enable_addr_off, is_win);
    case LDXW: return predicate_ld32(CURSRC, OFF, sv, NEWDST, block, enable_addr_off, is_win);
    case LDXDW: return predicate_ld64(CURSRC, OFF, sv, NEWDST, block, enable_addr_off, is_win);
    case STXB: return predicate_st8(CURSRC, CURDST, OFF, sv, block, false, enable_addr_off); // bpf_st = false
    case STXH: return predicate_st16(CURSRC, CURDST, OFF, sv, block, false, enable_addr_off);
    case STXW: return predicate_st32(CURSRC, CURDST, OFF, sv, block, false, enable_addr_off);
    case STXDW: return predicate_st64(CURSRC, CURDST, OFF, sv, block, false, enable_addr_off);
    case STB: return predicate_st8(IMM, CURDST, OFF, sv, block, true, enable_addr_off); // bpf_st = true
    case STH: return predicate_st16(IMM, CURDST, OFF, sv, block, true, enable_addr_off);
    case STW: return predicate_st32(IMM, CURDST, OFF, sv, block, true, enable_addr_off);
    case STDW: return predicate_st64(IMM, CURDST, OFF, sv, block, true, enable_addr_off);
    case XADD64: return predicate_xadd64(CURSRC, CURDST, OFF, sv, block, enable_addr_off, is_win);
    case XADD32: return predicate_xadd32(CURSRC, CURDST, OFF, sv, block, enable_addr_off, is_win);
    case LDABSH: return predicate_ldskbh(IMM, sv, R0, block);
    case LDINDH: return predicate_ldskbh(CURSRC, sv, R0, block); // todo: modify the function name
    case CALL: return predicate_helper_function(imm, R1, R2, R3, R4, R5, R0, sv, block, enable_addr_off, is_win);
    default: return string_to_expr("false");
  }
}

z3::expr inst::smt_inst_jmp(smt_var & sv) const {
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
    case JNEXC: return (CURDST != IMM);
    case JNEXY: return (CURDST != CURSRC);
    case JSGTXC: return (CURDST > IMM);
    case JSGTXY: return (CURDST > CURSRC);
    case JEQ32XC: return (CURDST_L32 == IMM_L32);
    case JEQ32XY: return (CURDST_L32 == CURSRC_L32);
    case JNE32XC: return (CURDST_L32 != IMM_L32);
    case JNE32XY: return (CURDST_L32 != CURSRC_L32);
    default: return string_to_expr("false");
  }
}

z3::expr inst::smt_inst_end(smt_var & sv) const {
  // there are two cases for a program end instruction: the default exit or tail call exit
  z3::expr f = Z3_true;
  if ((_opcode == CALL) && (_imm == BPF_FUNC_tail_call)) {
    sv.smt_out.pgm_has_tail_call = true;
    f = (sv.smt_out.pgm_exit_type == PGM_EXIT_TYPE_tail_call);
    f = f &&
        (sv.smt_out.tail_call_args[0] == R1) &&
        (sv.smt_out.tail_call_args[1] == R2) &&
        (sv.smt_out.tail_call_args[2] == R3);
  } else {
    f = (sv.smt_out.pgm_exit_type == PGM_EXIT_TYPE_default);
    if (smt_var::is_win) {
      unordered_set<int>& regs = sv.smt_out.output_var.regs;
      for (auto reg : regs) {
        f = f && (sv.smt_out.reg_expr(reg) == sv.get_cur_reg_var(reg));
      }
    } else {
      f = f && (sv.smt_out.ret_val == sv.get_cur_reg_var(0));
    }
  }
  return f;
}

z3::expr inst::smt_inst_safety_chk(smt_var & sv) const {
  z3::expr curSrc = sv.get_cur_reg_var(_src_reg);
  z3::expr curDst = sv.get_cur_reg_var(_dst_reg);
  int64_t imm = (int64_t)_imm;
  int64_t off = (int64_t)_off;

  switch (_opcode) {
    case LDXB: return safety_chk_ldx(CURSRC, OFF, 1, sv);
    case LDXH: return safety_chk_ldx(CURSRC, OFF, 2, sv);
    case LDXW: return safety_chk_ldx(CURSRC, OFF, 4, sv);
    case LDXDW: return safety_chk_ldx(CURSRC, OFF, 8, sv);
    case STXB: return safety_chk_stx(CURDST, OFF, 1, sv);
    case STXH: return safety_chk_stx(CURDST, OFF, 2, sv);
    case STXW: return safety_chk_stx(CURDST, OFF, 4, sv);
    case STXDW: return safety_chk_stx(CURDST, OFF, 8, sv);
    case STB: return safety_chk_st(CURDST, OFF, 1, sv);
    case STH: return safety_chk_st(CURDST, OFF, 2, sv);
    case STW: return safety_chk_st(CURDST, OFF, 4, sv);
    case STDW: return safety_chk_st(CURDST, OFF, 8, sv);
    default: return Z3_true; // mean no constraints
  }
}

int opcode_2_idx(int opcode) {
  switch (opcode) {
    case NOP: return IDX_NOP;
    case ADD64XC: return IDX_ADD64XC;
    case ADD64XY: return IDX_ADD64XY;
    case OR64XC: return IDX_OR64XC;
    case OR64XY: return IDX_OR64XY;
    case AND64XC: return IDX_AND64XC;
    case AND64XY: return IDX_AND64XY;
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
    case LDMAPID: return IDX_LDMAPID;
    case LDXB: return IDX_LDXB;
    case STXB: return IDX_STXB;
    case LDXH: return IDX_LDXH;
    case STXH: return IDX_STXH;
    case LDXW: return IDX_LDXW;
    case STXW: return IDX_STXW;
    case LDXDW: return IDX_LDXDW;
    case STXDW: return IDX_STXDW;
    case STB: return IDX_STB;
    case STH: return IDX_STH;
    case STW: return IDX_STW;
    case STDW: return IDX_STDW;
    case XADD64: return IDX_XADD64;
    case XADD32: return IDX_XADD32;
    case LDABSH: return IDX_LDABSH;
    case LDINDH: return IDX_LDINDH;
    case JA: return IDX_JA;
    case JEQXC: return IDX_JEQXC;
    case JEQXY: return IDX_JEQXY;
    case JGTXC: return IDX_JGTXC;
    case JGTXY: return IDX_JGTXY;
    case JNEXC: return IDX_JNEXC;
    case JNEXY: return IDX_JNEXY;
    case JSGTXC: return IDX_JSGTXC;
    case JSGTXY: return IDX_JSGTXY;
    case JEQ32XC: return IDX_JEQ32XC;
    case JEQ32XY: return IDX_JEQ32XY;
    case JNE32XC: return IDX_JNE32XC;
    case JNE32XY: return IDX_JNE32XY;
    case CALL: return IDX_CALL;
    case EXIT: return IDX_EXIT;
    default: cout << "unknown opcode" << endl; return 0;
  }
}

// TODO: set the stack memory as 0
z3::expr inst::smt_set_pre(z3::expr input, smt_var & sv) {
  z3::expr f = string_to_expr("true");
  f = (sv.get_cur_reg_var(1) == input) &&
      (sv.get_cur_reg_var(10) == sv.get_stack_bottom_addr()) &&
      sv.mem_layout_constrain();
  // add the relationship of r1 and address according to the program inpu type
  int pgm_input_type = mem_t::get_pgm_input_type();
  if ((pgm_input_type == PGM_INPUT_pkt) || (pgm_input_type == PGM_INPUT_skb)) {
    f = f && (sv.get_cur_reg_var(1) == sv.get_pkt_start_addr());
  } else if (pgm_input_type == PGM_INPUT_pkt_ptrs) {
    f = f && (sv.get_cur_reg_var(1) == sv.get_pkt_start_ptr_addr());
  }
  return f;
}

bool inst::is_cfg_basic_block_end() const {
  int op_type = get_opcode_type();
  if ((op_type == OP_RET) || (op_type == OP_UNCOND_JMP)) {
    return true;
  }
  if ((op_type == OP_CALL) && (_imm == BPF_FUNC_tail_call)) {
    return true;
  }
  return false;
}

bool inst::is_pgm_end() const {
  int op_type = get_opcode_type();
  if (op_type == OP_RET) {
    return true;
  }
  if ((op_type == OP_CALL) && (_imm == BPF_FUNC_tail_call)) {
    return true;
  }
  return false;
}

string inst::get_bytecode_str() const {
  string str = ("{"
                + to_string(_opcode) + ", " + to_string(_dst_reg) + ", "
                + to_string(_src_reg) + ", " + to_string(_off) + ", "
                + to_string(_imm)
                + "}");
  if (_opcode == LDMAPID) str += ",{0, 0, 0, 0, 0}";
  return str;
}

void inst::regs_to_read(vector<int>& regs) const {
  regs.clear();
  switch (_opcode) {
    case NOP:      return;
    case ADD64XC:  regs = {_dst_reg}; return;
    case ADD64XY:  regs = {_dst_reg, _src_reg}; return;
    case OR64XC:   regs = {_dst_reg}; return;
    case OR64XY:   regs = {_dst_reg, _src_reg}; return;
    case AND64XC:  regs = {_dst_reg}; return;
    case AND64XY:  regs = {_dst_reg, _src_reg}; return;
    case LSH64XC:  regs = {_dst_reg}; return;
    case LSH64XY:  regs = {_dst_reg, _src_reg}; return;
    case RSH64XC:  regs = {_dst_reg}; return;
    case RSH64XY:  regs = {_dst_reg, _src_reg}; return;
    case MOV64XC:  return;
    case MOV64XY:  regs = {_src_reg}; return;
    case ARSH64XC: regs = {_dst_reg}; return;
    case ARSH64XY: regs = {_dst_reg, _src_reg}; return;
    case ADD32XC:  regs = {_dst_reg}; return;
    case ADD32XY:  regs = {_dst_reg, _src_reg}; return;
    case OR32XC:   regs = {_dst_reg}; return;
    case OR32XY:   regs = {_dst_reg, _src_reg}; return;
    case AND32XC:  regs = {_dst_reg}; return;
    case AND32XY:  regs = {_dst_reg, _src_reg}; return;
    case LSH32XC:  regs = {_dst_reg}; return;
    case LSH32XY:  regs = {_dst_reg, _src_reg}; return;
    case RSH32XC:  regs = {_dst_reg}; return;
    case RSH32XY:  regs = {_dst_reg, _src_reg}; return;
    case MOV32XC:  return;
    case MOV32XY:  regs = {_src_reg}; return;
    case ARSH32XC: regs = {_dst_reg}; return;
    case ARSH32XY: regs = {_dst_reg, _src_reg}; return;
    case LE:       regs = {_dst_reg}; return;
    case BE:       regs = {_dst_reg}; return;
    case LDMAPID:  return;
    case LDXB:     regs = {_src_reg}; return;
    case STXB:     regs = {_dst_reg, _src_reg}; return;
    case LDXH:     regs = {_src_reg}; return;
    case STXH:     regs = {_dst_reg, _src_reg}; return;
    case LDXW:     regs = {_src_reg}; return;
    case STXW:     regs = {_dst_reg, _src_reg}; return;
    case LDXDW:    regs = {_src_reg}; return;
    case STXDW:    regs = {_dst_reg, _src_reg}; return;
    case STB:      regs = {_dst_reg}; return;
    case STH:      regs = {_dst_reg}; return;
    case STW:      regs = {_dst_reg}; return;
    case STDW:     regs = {_dst_reg}; return;
    case XADD64:   regs = {_dst_reg, _src_reg}; return;
    case XADD32:   regs = {_dst_reg, _src_reg}; return;
    case LDABSH:   return;
    case LDINDH:   regs = {_src_reg}; return;
    case JA:       return;
    case JEQXC:    regs = {_dst_reg}; return;
    case JEQXY:    regs = {_dst_reg, _src_reg}; return;
    case JGTXC:    return;
    case JGTXY:    regs = {_dst_reg, _src_reg}; return;
    case JNEXC:    regs = {_dst_reg}; return;
    case JNEXY:    regs = {_dst_reg, _src_reg}; return;
    case JSGTXC:   regs = {_dst_reg}; return;
    case JSGTXY:   regs = {_dst_reg, _src_reg}; return;
    case JEQ32XC:  regs = {_dst_reg}; return;
    case JEQ32XY:  regs = {_dst_reg, _src_reg}; return;
    case JNE32XC:  regs = {_dst_reg}; return;
    case JNE32XY:  regs = {_dst_reg, _src_reg}; return;
    case CALL:
      switch (_imm) {
        case BPF_FUNC_map_lookup_elem: regs = {1, 2}; return;
        case BPF_FUNC_map_update_elem: regs = {1, 2, 3}; return;
        case BPF_FUNC_map_delete_elem: regs = {1, 2}; return;
        case BPF_FUNC_tail_call: regs = {1, 2, 3}; return;
        case BPF_FUNC_get_prandom_u32: return;
        case BPF_FUNC_redirect: regs = {1, 2}; return;
        case BPF_FUNC_xdp_adjust_head: regs = {1, 2}; return;
        case BPF_FUNC_redirect_map: regs = {1, 2, 3}; return;
        default: cout << "Error: unknown function id " << _imm << endl; return;
      }
    case EXIT: return;
    default: cout << "unknown opcode" << endl;
  }
}

// return -1 if no reg to write, else return reg
int inst::reg_to_write() const {
  if (_opcode == NOP) { // NOP is not in eBPF, the bpf_class is the same as BPF_LD
    return -1;
  }
  if (_opcode == CALL) {
    return 0; // dst_reg is reg 0
  }
  int op_class = BPF_CLASS(_opcode);
  vector<int> reg_write_op_classes = {BPF_LD, BPF_LDX, BPF_ALU, BPF_ALU64};
  for (int i = 0; i < reg_write_op_classes.size(); i++) {
    if (op_class == reg_write_op_classes[i]) {
      return _dst_reg;
    }
  }
  return -1;
}

bool inst::is_mem_inst() const {
  if (_opcode == NOP) return false;
  int op_class = BPF_CLASS(_opcode);
  vector<int> mem_class = {BPF_LDX, BPF_STX, BPF_ST};
  for (int i = 0; i < mem_class.size(); i++) {
    if (op_class == mem_class[i]) {
      return true;
    }
  }
  return false;
}

bool inst::is_ldx_mem() const {
  unordered_set<int> set = {LDXB, LDXH, LDXW, LDXDW};
  if (set.find(_opcode) != set.end()) return true;
  else return false;
}

bool inst::is_stx_mem() const {
  unordered_set<int> set = {STXB, STXH, STXW, STXDW};
  if (set.find(_opcode) != set.end()) return true;
  else return false;
}

bool inst::is_st_mem() const {
  unordered_set<int> set = {STB, STH, STW, STDW};
  if (set.find(_opcode) != set.end()) return true;
  else return false;
}

bool inst::is_xadd() const {
  unordered_set<int> set = {XADD64, XADD32};
  if (set.find(_opcode) != set.end()) return true;
  else return false;
}

vector<int> ldx_sample_opcodes = {LDXB, LDXH, LDXW, LDXDW};
vector<int> stx_sample_opcodes = {STXB, STXH, STXW, STXDW, STB, STH, STW, STDW, XADD64, XADD32};
vector<int> st_sample_opcodes = {STB, STH, STW, STDW};
vector<int> xadd_sample_opcodes = {XADD64, XADD32};

int inst::num_sample_mem_opcodes() const {
  if (is_ldx_mem()) return ldx_sample_opcodes.size();
  else if (is_stx_mem()) return stx_sample_opcodes.size();
  else if (is_st_mem()) return st_sample_opcodes.size();
  else if (is_xadd()) return xadd_sample_opcodes.size();

  return 0;
}

int inst::get_mem_opcode_by_sample_idx(int sample_idx) const {
  assert(sample_idx >= 0);
  if (is_ldx_mem()) {
    assert(sample_idx < ldx_sample_opcodes.size());
    return ldx_sample_opcodes[sample_idx];
  } else if (is_stx_mem()) {
    assert(sample_idx < stx_sample_opcodes.size());
    return stx_sample_opcodes[sample_idx];
  } else if (is_st_mem()) {
    assert(sample_idx < st_sample_opcodes.size());
    return st_sample_opcodes[sample_idx];
  } else if (is_xadd()) {
    assert(sample_idx < xadd_sample_opcodes.size());
    return xadd_sample_opcodes[sample_idx];
  };
  return 0;
}

void interpret(inout_t& output, inst * program, int length, prog_state & ps, const inout_t& input) {
#undef IMM
#undef OFF
#undef MEM
#undef R0
#undef R1
#undef R2
#undef R3
#undef R4
#undef R5
#undef DST_L32
#undef SRC_L32
#undef IMM_L32
// type: int64_t
#define DST ps._regs[insn->_dst_reg]
#define SRC ps._regs[insn->_src_reg]
#define IMM (int64_t)insn->_imm
#define DST_L32 L32(DST)
#define SRC_L32 L32(SRC)
#define IMM_L32 insn->_imm
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
#define SR sr

#define DST_ID (insn->_dst_reg)
#define SRC_ID (insn->_src_reg)

  int start_insn = input.start_insn;
  if (smt_var::is_win) { // length is from 0
    length = inout_t::end_insn + 1;
  }
  inst* insn = &program[inout_t::start_insn];
  ps.clear();
  // register r10 is set by update_ps_by_input
  update_ps_by_input(ps, input);
  // set real_r10 as frame pointer, the bottom of the stack
  uint64_t real_r10 = (uint64_t)ps._mem.get_stack_bottom_addr();
  uint64_t simu_r10 = (uint64_t)ps._regs[10];
  int pgm_input_type = mem_t::get_pgm_input_type();
  simu_real sr;
  if (pgm_input_type != PGM_INPUT_pkt_ptrs) {
    uint64_t real_r1 = (uint64_t)ps._mem.get_pkt_start_addr();
    uint64_t simu_r1 = input.reg; // pkt start
    if (input.is_win) simu_r1 = (uint64_t)input.input_simu_pkt_s;
    if (real_r1 == 0) real_r1 = simu_r1;
    sr.set_vals(simu_r10, real_r10, simu_r1, real_r1);
  } else { // PGM_INPUT_pkt_ptrs
    // r1 is pkt_ptrs address
    uint64_t real_r1 = (uint64_t)ps._mem.get_pkt_ptrs_start_addr();
    uint64_t simu_r1 = (uint64_t)ps._regs[1];
    uint64_t real_pkt = (uint64_t)ps._mem.get_pkt_start_addr();
    uint64_t simu_pkt = (uint64_t)input.input_simu_pkt_ptrs[0];
    if (input.is_win) {
      simu_r1 = (uint64_t)ps._mem._simu_pkt_ptrs_s; // pkt_ptrs_s
    }
    sr.set_vals(simu_r10, real_r10, simu_r1, real_r1, simu_pkt, real_pkt);
  }
  uint64_t real_addr = 0; // used as temporary variable in instruction execution

  static void *jumptable[NUM_INSTR] = {
    [IDX_NOP]      = && INSN_NOP,
    [IDX_ADD64XC]  = && INSN_ADD64XC,
    [IDX_ADD64XY]  = && INSN_ADD64XY,
    [IDX_OR64XC]   = && INSN_OR64XC,
    [IDX_OR64XY]   = && INSN_OR64XY,
    [IDX_AND64XC]  = && INSN_AND64XC,
    [IDX_AND64XY]  = && INSN_AND64XY,
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
    [IDX_LDMAPID]  = && INSN_LDMAPID,
    [IDX_LDXB]     = && INSN_LDXB,
    [IDX_STXB]     = && INSN_STXB,
    [IDX_LDXH]     = && INSN_LDXH,
    [IDX_STXH]     = && INSN_STXH,
    [IDX_LDXW]     = && INSN_LDXW,
    [IDX_STXW]     = && INSN_STXW,
    [IDX_LDXDW]    = && INSN_LDXDW,
    [IDX_STXDW]    = && INSN_STXDW,
    [IDX_STB]      = && INSN_STB,
    [IDX_STH]      = && INSN_STH,
    [IDX_STW]      = && INSN_STW,
    [IDX_STDW]     = && INSN_STDW,
    [IDX_XADD64]   = && INSN_XADD64,
    [IDX_XADD32]   = && INSN_XADD32,
    [IDX_LDABSH]   = && INSN_LDABSH,
    [IDX_LDINDH]   = && INSN_LDINDH,
    [IDX_JA]       = && INSN_JA,
    [IDX_JEQXC]    = && INSN_JEQXC,
    [IDX_JEQXY]    = && INSN_JEQXY,
    [IDX_JGTXC]    = && INSN_JGTXC,
    [IDX_JGTXY]    = && INSN_JGTXY,
    [IDX_JNEXC]    = && INSN_JNEXC,
    [IDX_JNEXY]    = && INSN_JNEXY,
    [IDX_JSGTXC]   = && INSN_JSGTXC,
    [IDX_JSGTXY]   = && INSN_JSGTXY,
    [IDX_JEQ32XC]  = && INSN_JEQ32XC,
    [IDX_JEQ32XY]  = && INSN_JEQ32XY,
    [IDX_JNE32XC]  = && INSN_JNE32XC,
    [IDX_JNE32XY]  = && INSN_JNE32XY,
    [IDX_CALL]     = && INSN_CALL,
    [IDX_EXIT]     = && INSN_EXIT,
  };

#define CONT {                                                     \
      safety_chk(*insn, ps);                                       \
      insn++;                                                      \
      if (insn < program + length) {                               \
        goto *jumptable[opcode_2_idx(insn->_opcode)];              \
      } else goto out;                                             \
  }

select_insn:
  goto *jumptable[opcode_2_idx(insn->_opcode)];

INSN_NOP:
  CONT;

#define ALU_UNARY(OPCODE, OP)                                      \
  INSN_##OPCODE##64XC:                                             \
    ps.reg_safety_chk(DST_ID);                                     \
    DST = compute_##OP(IMM);                                       \
    CONT;                                                          \
  INSN_##OPCODE##64XY:                                             \
    ps.reg_safety_chk(DST_ID, vector<int>{SRC_ID});                \
    DST = compute_##OP(SRC);                                       \
    CONT;                                                          \
  INSN_##OPCODE##32XC:                                             \
    ps.reg_safety_chk(DST_ID);                                     \
    DST = compute_##OP##32(IMM);                                   \
    CONT;                                                          \
  INSN_##OPCODE##32XY:                                             \
    ps.reg_safety_chk(DST_ID, vector<int>{SRC_ID});                \
    DST = compute_##OP##32(SRC);                                   \
    CONT;
  ALU_UNARY(MOV, mov)
#undef ALU_UNARY

#define ALU_BINARY(OPCODE, OP)                                     \
  INSN_##OPCODE##64XC:                                             \
    ps.reg_safety_chk(DST_ID, vector<int>{DST_ID});                \
    DST = compute_##OP(DST, IMM);                                  \
    CONT;                                                          \
  INSN_##OPCODE##64XY:                                             \
    ps.reg_safety_chk(DST_ID, vector<int>{DST_ID, SRC_ID});        \
    DST = compute_##OP(DST, SRC);                                  \
    CONT;                                                          \
  INSN_##OPCODE##32XC:                                             \
    ps.reg_safety_chk(DST_ID, vector<int>{DST_ID});                \
    DST = compute_##OP##32(DST, IMM);                              \
    CONT;                                                          \
  INSN_##OPCODE##32XY:                                             \
    ps.reg_safety_chk(DST_ID, vector<int>{DST_ID, SRC_ID});        \
    DST = compute_##OP##32(DST, SRC);                              \
    CONT;
  ALU_BINARY(ADD, add)
  ALU_BINARY(OR, or )
  ALU_BINARY(AND, and )
#undef ALU_BINARY

#define ALU_BINARY_SHIFT(OPCODE, OP)                               \
  INSN_##OPCODE##64XC:                                             \
    ps.reg_safety_chk(DST_ID, vector<int>{DST_ID});                \
    DST = compute_##OP(DST, IMM);                                  \
    CONT;                                                          \
  INSN_##OPCODE##64XY:                                             \
    ps.reg_safety_chk(DST_ID, vector<int>{DST_ID, SRC_ID});        \
    DST = compute_##OP(DST, SRC_L6);                               \
    CONT;                                                          \
  INSN_##OPCODE##32XC:                                             \
    ps.reg_safety_chk(DST_ID, vector<int>{DST_ID});                \
    DST = compute_##OP##32(DST, IMM);                              \
    CONT;                                                          \
  INSN_##OPCODE##32XY:                                             \
    ps.reg_safety_chk(DST_ID, vector<int>{DST_ID, SRC_ID});        \
    DST = compute_##OP##32(DST, SRC_L5);                           \
    CONT;
  ALU_BINARY_SHIFT(LSH, lsh)
  ALU_BINARY_SHIFT(RSH, rsh)
  ALU_BINARY_SHIFT(ARSH, arsh)
#undef ALU_BINARY_SHIFT

#define LDST(SIZEOP, SIZE)                                           \
  INSN_LDX##SIZEOP:                                                  \
    ps.reg_safety_chk(DST_ID, vector<int>{SRC_ID});                  \
    real_addr = get_real_addr_by_simu(SRC + OFF, MEM, SR, ps.get_reg_type(SRC_ID)); \
    ps.memory_access_and_safety_chk(real_addr, SIZE/8, true, true, true);  \
    DST = compute_ld##SIZE(real_addr, 0);                            \
    CONT;                                                            \
  INSN_STX##SIZEOP:                                                  \
    ps.reg_safety_chk(DST_ID, vector<int>{DST_ID, SRC_ID});          \
    real_addr = get_real_addr_by_simu(DST + OFF, MEM, SR, ps.get_reg_type(DST_ID)); \
    ps.memory_access_and_safety_chk(real_addr, SIZE/8, true, false, true); \
    compute_st##SIZE(SRC, real_addr, 0);                             \
    CONT;                                                            \
  INSN_ST##SIZEOP:                                                   \
    ps.reg_safety_chk(DST_ID, vector<int>{DST_ID});                  \
    real_addr = get_real_addr_by_simu(DST + OFF, MEM, SR, ps.get_reg_type(DST_ID)); \
    ps.memory_access_and_safety_chk(real_addr, SIZE/8, true, false, true); \
    compute_st##SIZE(IMM, real_addr, 0);                             \
    CONT;
  LDST(B,  8)
  LDST(H,  16)
  LDST(W,  32)
  LDST(DW, 64)
#undef LDST

#define XADD(SIZE)                                                   \
  INSN_XADD##SIZE:                                                   \
    ps.reg_safety_chk(DST_ID, vector<int>{DST_ID, SRC_ID});          \
    real_addr = get_real_addr_by_simu(DST + OFF, MEM, SR, ps.get_reg_type(DST_ID)); \
    ps.memory_access_and_safety_chk(real_addr, SIZE/8, true, false, true); \
    compute_xadd##SIZE(SRC, real_addr, 0);                           \
    CONT;
  XADD(32)
  XADD(64)
#undef XADD

INSN_LDABSH:
  ps.reg_safety_chk(0); // set r0 is readable
  real_addr = (uint64_t)ps._mem.get_pkt_addr_by_offset(IMM);
  ps.memory_access_and_safety_chk(real_addr, 2, true, true);
  R0 = compute_ld16(real_addr, 0);
  CONT;
INSN_LDINDH:
  ps.reg_safety_chk(0, vector<int> {SRC_ID});
  real_addr = (uint64_t)ps._mem.get_pkt_addr_by_offset(SRC);
  R0 = compute_ld16(real_addr, 0);
  CONT;

#define BYTESWAP(OPCODE, OP)                                       \
  INSN_##OPCODE:                                                   \
    ps.reg_safety_chk(DST_ID, vector<int>{DST_ID});                \
    switch (IMM) {                                                 \
      case 16: DST = compute_##OP##16(DST);break;                  \
      case 32: DST = compute_##OP##32(DST);break;                  \
      case 64: DST = compute_##OP##64(DST);break;                  \
      default: cout << "[Error] imm " << IMM                       \
                    << " is not 16, 32, 64" << endl;               \
               break;                                              \
    }                                                              \
    CONT;
  BYTESWAP(LE, le)
  BYTESWAP(BE, be)
#undef BYTESWAP

INSN_LDMAPID:
  ps.reg_safety_chk(DST_ID);
  DST = compute_ldmapid(IMM);
  CONT;

INSN_JA:
  insn += OFF;
  CONT;

#define COND_JMP(OPCODE, OP, DST_REG, SRC_REG, IMM_NUM)            \
  INSN_##OPCODE##XC:                                               \
    ps.reg_safety_chk(DST_ID, vector<int>{DST_ID});                \
    if (DST_REG OP IMM_NUM)                                        \
      insn += OFF;                                                 \
  CONT;                                                            \
  INSN_##OPCODE##XY:                                               \
    ps.reg_safety_chk(DST_ID, vector<int>{DST_ID, SRC_ID});        \
    if (DST_REG OP SRC_REG)                                        \
      insn += OFF;                                                 \
  CONT;                                                            \
  INSN_##OPCODE##32XC:                                             \
    ps.reg_safety_chk(DST_ID, vector<int>{DST_ID});                \
    if (L32(DST_REG) OP L32(IMM_NUM))                              \
      insn += OFF;                                                 \
  CONT;                                                            \
  INSN_##OPCODE##32XY:                                             \
    ps.reg_safety_chk(DST_ID, vector<int>{DST_ID, SRC_ID});        \
    if (L32(DST_REG) OP L32(SRC_REG))                              \
      insn += OFF;                                                 \
  CONT;
  COND_JMP(JEQ, ==, UDST, USRC, UIMM)
  COND_JMP(JGT,  >, UDST, USRC, UIMM)
  COND_JMP(JNE, !=, UDST, USRC, UIMM)
  COND_JMP(JSGT, >, DST, SRC, IMM)
#undef COND_JMP

INSN_CALL:
  // safety check of helper functions is inside compute_helper_function(),
  // since different functions have different parameters
  R0 = compute_helper_function(IMM, R1, R2, R3, R4, R5, SR, ps);
  if (IMM == BPF_FUNC_tail_call) { // exit regardless of the return value r0
    ps._pgm_exit_type = PGM_EXIT_TYPE_tail_call;
    goto out;
  }
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

// 1. BPF_ST is illegal if src_reg type is PTR_TO_CTX
void safety_chk(inst & insn, prog_state & ps) {
  int op_class = BPF_CLASS(insn._opcode);
  if (op_class == BPF_ST) {
    int dst_reg_type = ps.get_reg_type(insn._dst_reg);
    if (dst_reg_type == PTR_TO_CTX) {
      string err_msg = "BPF_ST stores into PTR_TO_CTX reg is not allowed";
      throw err_msg;
    }
  }
  // update register type
  if (insn._opcode == MOV64XY) {
    ps.set_reg_type(insn._dst_reg, ps.get_reg_type(insn._src_reg));
  } else if (insn._opcode == CALL) {
    ps.set_reg_type(0, SCALAR_VALUE);
  } else if ((insn._opcode != ADD64XC) && // ADD64XC won't change dst_reg type
             (op_class != BPF_STX) &&
             (op_class != BPF_ST) &&
             (op_class != BPF_JMP) &&
             (op_class != BPF_JMP32)) {
    ps.set_reg_type(insn._dst_reg, SCALAR_VALUE);
  }
}

void inst::regs_cannot_be_ptrs(vector<int>& regs) const {
  regs.clear();
  switch (_opcode) {
    case NOP:      return;
    case ADD64XC:  return;
    case ADD64XY:  regs = {_dst_reg, _src_reg}; return; // todo: ADD64XY is not allowed here
    case OR64XC:   regs = {_dst_reg}; return;
    case OR64XY:   regs = {_dst_reg, _src_reg}; return;
    case AND64XC:  regs = {_dst_reg}; return;
    case AND64XY:  regs = {_dst_reg, _src_reg}; return;
    case LSH64XC:  regs = {_dst_reg}; return;
    case LSH64XY:  regs = {_dst_reg, _src_reg}; return;
    case RSH64XC:  regs = {_dst_reg}; return;
    case RSH64XY:  regs = {_dst_reg, _src_reg}; return;
    case MOV64XC:  return;
    case MOV64XY:  return;
    case ARSH64XC: regs = {_dst_reg}; return;
    case ARSH64XY: regs = {_dst_reg, _src_reg}; return;
    case ADD32XC:  regs = {_dst_reg}; return;
    case ADD32XY:  regs = {_dst_reg, _src_reg}; return;
    case OR32XC:   regs = {_dst_reg}; return;
    case OR32XY:   regs = {_dst_reg, _src_reg}; return;
    case AND32XC:  regs = {_dst_reg}; return;
    case AND32XY:  regs = {_dst_reg, _src_reg}; return;
    case LSH32XC:  regs = {_dst_reg}; return;
    case LSH32XY:  regs = {_dst_reg, _src_reg}; return;
    case RSH32XC:  regs = {_dst_reg}; return;
    case RSH32XY:  regs = {_dst_reg, _src_reg}; return;
    case MOV32XC:  return;
    case MOV32XY:  regs = {_src_reg}; return;
    case ARSH32XC: regs = {_dst_reg}; return;
    case ARSH32XY: regs = {_dst_reg, _src_reg}; return;
    case LE:       regs = {_dst_reg}; return;
    case BE:       regs = {_dst_reg}; return;
    case LDMAPID:  return;
    case LDXB:     return;
    case STXB:     regs = {_src_reg}; return; // assume ptr cannot be stored in memory
    case LDXH:     return;
    case STXH:     regs = {_src_reg}; return;
    case LDXW:     return;
    case STXW:     regs = {_src_reg}; return;
    case LDXDW:    return;
    case STXDW:    regs = {_src_reg}; return;
    case STB:      return;
    case STH:      return;
    case STW:      return;
    case STDW:     return;
    case XADD64:   regs = {_src_reg}; return;
    case XADD32:   regs = {_src_reg}; return;
    case LDABSH:   return;
    case LDINDH:   regs = {_src_reg}; return;
    case JA:       return;
    case JEQXC:    regs = {_dst_reg}; return;
    case JEQXY:    regs = {_dst_reg, _src_reg}; return;
    case JGTXC:    return;
    case JGTXY:    regs = {_dst_reg, _src_reg}; return;
    case JNEXC:    regs = {_dst_reg}; return;
    case JNEXY:    regs = {_dst_reg, _src_reg}; return;
    case JSGTXC:   regs = {_dst_reg}; return;
    case JSGTXY:   regs = {_dst_reg, _src_reg}; return;
    case JEQ32XC:  regs = {_dst_reg}; return;
    case JEQ32XY:  regs = {_dst_reg, _src_reg}; return;
    case JNE32XC:  regs = {_dst_reg}; return;
    case JNE32XY:  regs = {_dst_reg, _src_reg}; return;
    case CALL: return;
    case EXIT: return;

    default: cout << "unknown opcode" << endl;
  }
}
