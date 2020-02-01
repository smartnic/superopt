#pragma once

#include <vector>
#include <unordered_set>
#include "z3++.h"
#include "../../../src/verify/smt_var.h"
#include "../inst.h"
#include "inst_codegen.h"

using namespace std;

class toy_isa {
 public:
  static constexpr int NUM_REGS = 4;
  static constexpr int MAX_PROG_LEN = 7;
  // Max number of operands in one instruction
  static constexpr int MAX_OP_LEN = 3;

  // Number of bits of a single opcode or operand
  static constexpr int OP_NUM_BITS = 5;
  // Number of bits of a single instruction
  static constexpr int INST_NUM_BITS = 20;

  // Instruction opcodes
  enum OPCODES {
    NOP = 0,
    ADDXY,
    MOVXC,
    RETX,
    RETC,
    JMP,
    JMPEQ,
    JMPGT,
    JMPGE,
    JMPLT,
    JMPLE,
    MAXC,
    MAXX,
    NUM_INSTR, // Number of opcode types
  };

  static constexpr int num_operands[NUM_INSTR] = {
    [NOP]   = 0,
    [ADDXY] = 2,
    [MOVXC] = 2,
    [RETX]  = 1,
    [RETC]  = 1,
    [JMP]   = 1,
    [JMPEQ] = 3,
    [JMPGT] = 3,
    [JMPGE] = 3,
    [JMPLT] = 3,
    [JMPLE] = 3,
    [MAXC]  = 2,
    [MAXX]  = 2,
  };

  static constexpr int insn_num_regs[NUM_INSTR] = {
    [NOP]   = 0,
    [ADDXY] = 2,
    [MOVXC] = 1,
    [RETX]  = 1,
    [RETC]  = 0,
    [JMP]   = 0,
    [JMPEQ] = 2,
    [JMPGT] = 2,
    [JMPGE] = 2,
    [JMPLT] = 2,
    [JMPLE] = 2,
    [MAXC]  = 1,
    [MAXX]  = 2,
  };

  static constexpr int opcode_type[NUM_INSTR] = {
    [NOP]   = OP_NOP,
    [ADDXY] = OP_OTHERS,
    [MOVXC] = OP_OTHERS,
    [RETX]  = OP_RET,
    [RETC]  = OP_RET,
    [JMP]   = OP_UNCOND_JMP,
    [JMPEQ] = OP_COND_JMP,
    [JMPGT] = OP_COND_JMP,
    [JMPGE] = OP_COND_JMP,
    [JMPLT] = OP_COND_JMP,
    [JMPLE] = OP_COND_JMP,
    [MAXC]  = OP_OTHERS,
    [MAXX]  = OP_OTHERS,
  };

  // Max value for immediate operand
  static constexpr int MAX_CONST = 20;
  // Operand types for instructions
  static constexpr int OP_UNUSED = 0;
  static constexpr int OP_REG = 1;
  static constexpr int OP_IMM = 2;
  static constexpr int OP_OFF = 3;

  /* The definitions below assume a minimum 16-bit integer data type */
#define TOY_ISA_OPTYPE(opcode, opindex) ((toy_isa::optable[opcode] >> ((opindex) * 5)) & 31)
#define FSTOP(x) (x)
#define SNDOP(x) (x << 5)
#define TRDOP(x) (x << 10)
#define JMP_OPS (FSTOP(OP_REG) | SNDOP(OP_REG) | TRDOP(OP_OFF))
#define UNUSED_OPS (FSTOP(OP_UNUSED) | SNDOP(OP_UNUSED) | TRDOP(OP_UNUSED))
  static constexpr int optable[NUM_INSTR] = {
    [NOP]   = UNUSED_OPS,
    [ADDXY] = FSTOP(OP_REG) | SNDOP(OP_REG) | TRDOP(OP_UNUSED),
    [MOVXC] = FSTOP(OP_REG) | SNDOP(OP_IMM) | TRDOP(OP_UNUSED),
    [RETX]  = FSTOP(OP_REG) | SNDOP(OP_UNUSED) | TRDOP(OP_UNUSED),
    [RETC]  = FSTOP(OP_IMM) | SNDOP(OP_UNUSED) | TRDOP(OP_UNUSED),
    [JMP]   = FSTOP(OP_OFF) | SNDOP(OP_UNUSED) | TRDOP(OP_UNUSED),
    [JMPEQ] = JMP_OPS,
    [JMPGT] = JMP_OPS,
    [JMPGE] = JMP_OPS,
    [JMPLT] = JMP_OPS,
    [JMPLE] = JMP_OPS,
    [MAXC]  = FSTOP(OP_REG) | SNDOP(OP_IMM) | TRDOP(OP_UNUSED),
    [MAXX]  = FSTOP(OP_REG) | SNDOP(OP_REG) | TRDOP(OP_UNUSED),
  };
#undef FSTOP
#undef SNDOP
#undef TRDOP
#undef JMP_OPS
#undef UNUSED_OPS
};

class toy_isa_prog_state: public prog_state {
 public:
  toy_isa_prog_state() {regs.resize(toy_isa::NUM_REGS, 0);}
};

class toy_isa_inst: public inst {
 public:
  static toy_isa _isa;
  toy_isa_inst(int opcode = _isa.NOP, int arg1 = 0, int arg2 = 0, int arg3 = 0) {
    _args.resize(_isa.MAX_OP_LEN);
    _opcode  = opcode;
    _args[0] = arg1;
    _args[1] = arg2;
    _args[2] = arg3;
  }
  toy_isa_inst& operator=(const inst &rhs);
  string opcode_to_str(int) const;
  int32_t get_max_operand_val(int op_index, int inst_index = 0) const;
  void make_insts(vector<inst*> &instptr_list, const vector<inst*> &other) const;
  void make_insts(vector<inst*> &instptr_list, const inst* instruction) const;
  void clear_insts();
  int get_jmp_dis() const;
  void insert_jmp_opcodes(unordered_set<int>& jmp_set) const;
  int inst_output_opcode_type() const;
  int inst_output() const;
  bool is_real_inst() const;
  void set_as_nop_inst();
  // return the register for storing the given input
  unsigned int get_input_reg() const {return 0;}
  // for class toy_isa
  int get_num_regs() const {return _isa.NUM_REGS;}
  int get_max_prog_len() const {return _isa.MAX_PROG_LEN;}
  int get_max_op_len() const {return _isa.MAX_OP_LEN;}
  int get_op_num_bits() const {return _isa.OP_NUM_BITS;}
  int get_num_instr() const {return _isa.NUM_INSTR;}
  int get_num_operands() const {return _isa.num_operands[_opcode];}
  int get_insn_num_regs() const {return _isa.insn_num_regs[_opcode];}
  int get_opcode_type() const {return _isa.opcode_type[_opcode];}
  int64_t interpret(const vector<inst*> &instptr_list, prog_state &ps, int64_t input) const;
  // smt
  z3::expr smt_inst(smt_var& sv) const;
  z3::expr smt_inst_jmp(smt_var& sv) const;
};
