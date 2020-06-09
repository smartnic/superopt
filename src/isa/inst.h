#pragma once

#include <vector>
#include <unordered_set>
#include "../../src/utils.h"
#include "../../src/isa/inst_var.h"

#if ISA_TOY_ISA
#include "../../src/isa/toy-isa/inst_var.h"
#elif ISA_EBPF
#include "../../src/isa/ebpf/inst_var.h"
#endif

using namespace std;

enum ISA_TYPES {
  TOY_ISA = 0,
  EBPF,
};

// Opcode types for instructions
#define OP_NOP 0
#define OP_RET 1
#define OP_UNCOND_JMP 2
#define OP_COND_JMP 3
#define OP_OTHERS 4
#define OP_ST 5
#define OP_LD 6

// Return opcode types for the end instruction of a program
#define RET_C 0   // return immediate number
#define RET_X 1   // return register

class inst_base {
 public:
  static int max_prog_len;
  int _opcode;
  vector<op_t> _args;
  inst_base() {}
  void to_abs_bv(vector<op_t>& abs_vec) const;
  int get_operand(int op_index) const;
  void set_operand(int op_index, op_t op_value);
  int get_opcode() const;
  int get_opcode_by_idx(int idx) const;
  void set_opcode(int op_value);

  /* Functions class inst should support */
  // inst& operator=(const inst &rhs)
  bool operator==(const inst_base &x) const {RAISE_EXCEPTION("inst::operator==");}
  void print() const {RAISE_EXCEPTION("inst::print");}
  // get_canonical_reg_list returns the list of regs which can be modified by prog canonicalize
  vector<int> get_canonical_reg_list() const {RAISE_EXCEPTION("inst::get_canonical_reg_list");}
  static vector<int> get_isa_canonical_reg_list() {RAISE_EXCEPTION("inst::get_isa_canonical_reg_list");}
  string opcode_to_str(int) const {RAISE_EXCEPTION("inst::opcode_to_str");}
  op_t get_max_operand_val(int op_index, int inst_index = 0) const {RAISE_EXCEPTION("inst::get_max_operand_val");}
  op_t get_min_operand_val(int op_index, int inst_index = 0) const {RAISE_EXCEPTION("inst::get_min_operand_val");}
  int get_jmp_dis() const {RAISE_EXCEPTION("inst::get_jmp_dis");}
  // insert all jmp opcode in jmp_set, used by proposals.cc to
  // avoid jumps in the last line of the program
  void insert_jmp_opcodes(unordered_set<int>& jmp_set) const {RAISE_EXCEPTION("inst::insert_jmp_opcodes");}
  int inst_output_opcode_type() const {RAISE_EXCEPTION("inst::inst_output_opcode_type");}
  int inst_output() const {RAISE_EXCEPTION("inst::inst_output");}
  bool is_real_inst() const {RAISE_EXCEPTION("inst::is_real_inst");}
  bool is_reg(int op_index) const {RAISE_EXCEPTION("inst::is_reg");}
  // If ISA allows an implicit register, return the register, else return -1
  int implicit_ret_reg() const {RAISE_EXCEPTION("inst::implicit_ret_reg");}
  void set_as_nop_inst() {RAISE_EXCEPTION("inst::set_as_nop_inst");}
  unsigned int get_input_reg() const {RAISE_EXCEPTION("inst::get_input_reg");}
  int get_num_operands() const {RAISE_EXCEPTION("inst::get_num_operands");}
  int get_insn_num_regs() const {RAISE_EXCEPTION("inst::get_insn_num_regs");}
  int get_opcode_type() const {RAISE_EXCEPTION("inst::get_opcode_type");}
  // smt
  // return SMT for the given OP_OTHERS type instruction, other types return false
  z3::expr smt_inst(smt_var& sv, z3::expr cond = Z3_true) const {RAISE_EXCEPTION("inst::smt_inst");}
  // return SMT for the given OP_COND_JMP type instruction, other types return false
  z3::expr smt_inst_jmp(smt_var& sv) const {RAISE_EXCEPTION("inst::smt_inst_jmp");}
  static z3::expr smt_set_pre(z3::expr input, smt_var& sv) {RAISE_EXCEPTION("inst::smt_set_pre");}
};

struct instHash {
  size_t operator()(const inst_base &x) const;
};

/* inst.cc should support */
// void interpret(inout_t& output, inst* program, int length, prog_state &ps, inout_t& output);

