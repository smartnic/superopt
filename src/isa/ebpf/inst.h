#pragma once

#include <vector>
#include <unordered_set>
#include "z3++.h"
#include "../../../src/utils.h"
#include "../../../src/verify/smt_var.h"
#include "../../../src/isa/inst.h"

using namespace std;

class ebpf {
 public:
  static constexpr int NUM_REGS = 10;
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
    // ALU64
    ADD64XC,
    ADD64XY,
    LSH64XC,
    LSH64XY,
    RSH64XC,
    RSH64XY,
    MOV64XC,
    MOV64XY,
    ARSH64XC,
    ARSH64XY,
    // ALU32
    ADD32XC,
    ADD32XY,
    LSH32XC,
    LSH32XY,
    RSH32XC,
    RSH32XY,
    MOV32XC,
    MOV32XY,
    ARSH32XC,
    ARSH32XY,
    // Byteswap
    LE16,
    LE32,
    LE64,
    BE16,
    BE32,
    BE64,
    // JMP
    JA,
    JEQXC,
    JEQXY,
    JGTXC,
    JGTXY,
    JSGTXC,
    JSGTXY,
    // Exit
    EXIT,
    NUM_INSTR, // Number of opcode types
  };

  static constexpr int num_operands[NUM_INSTR] = {
    [NOP]      = 0,
    [ADD64XC]  = 2,
    [ADD64XY]  = 2,
    [LSH64XC]  = 2,
    [LSH64XY]  = 2,
    [RSH64XC]  = 2,
    [RSH64XY]  = 2,
    [MOV64XC]  = 2,
    [MOV64XY]  = 2,
    [ARSH64XC] = 2,
    [ARSH64XY] = 2,
    [ADD32XC]  = 2,
    [ADD32XY]  = 2,
    [LSH32XC]  = 2,
    [LSH32XY]  = 2,
    [RSH32XC]  = 2,
    [RSH32XY]  = 2,
    [MOV32XC]  = 2,
    [MOV32XY]  = 2,
    [ARSH32XC] = 2,
    [ARSH32XY] = 2,
    [LE16]     = 1,
    [LE32]     = 1,
    [LE64]     = 1,
    [BE16]     = 1,
    [BE32]     = 1,
    [BE64]     = 1,
    [JA]       = 1,
    [JEQXC]    = 3,
    [JEQXY]    = 3,
    [JGTXC]    = 3,
    [JGTXY]    = 3,
    [JSGTXC]   = 3,
    [JSGTXY]   = 3,
    [EXIT]     = 0,
  };

  static constexpr int insn_num_regs[NUM_INSTR] = {
    [NOP]      = 0,
    [ADD64XC]  = 1,
    [ADD64XY]  = 2,
    [LSH64XC]  = 1,
    [LSH64XY]  = 2,
    [RSH64XC]  = 1,
    [RSH64XY]  = 2,
    [MOV64XC]  = 1,
    [MOV64XY]  = 2,
    [ARSH64XC] = 1,
    [ARSH64XY] = 2,
    [ADD32XC]  = 1,
    [ADD32XY]  = 2,
    [LSH32XC]  = 1,
    [LSH32XY]  = 2,
    [RSH32XC]  = 1,
    [RSH32XY]  = 2,
    [MOV32XC]  = 1,
    [MOV32XY]  = 2,
    [ARSH32XC] = 1,
    [ARSH32XY] = 2,
    [LE16]     = 1,
    [LE32]     = 1,
    [LE64]     = 1,
    [BE16]     = 1,
    [BE32]     = 1,
    [BE64]     = 1,
    [JA]       = 0,
    [JEQXC]    = 1,
    [JEQXY]    = 2,
    [JGTXC]    = 1,
    [JGTXY]    = 2,
    [JSGTXC]   = 1,
    [JSGTXY]   = 2,
    [EXIT]     = 0,
  };

  static constexpr int opcode_type[NUM_INSTR] = {
    [NOP]      = OP_NOP,
    [ADD64XC]  = OP_OTHERS,
    [ADD64XY]  = OP_OTHERS,
    [LSH64XC]  = OP_OTHERS,
    [LSH64XY]  = OP_OTHERS,
    [RSH64XC]  = OP_OTHERS,
    [RSH64XY]  = OP_OTHERS,
    [MOV64XC]  = OP_OTHERS,
    [MOV64XY]  = OP_OTHERS,
    [ARSH64XC] = OP_OTHERS,
    [ARSH64XY] = OP_OTHERS,
    [ADD32XC]  = OP_OTHERS,
    [ADD32XY]  = OP_OTHERS,
    [LSH32XC]  = OP_OTHERS,
    [LSH32XY]  = OP_OTHERS,
    [RSH32XC]  = OP_OTHERS,
    [RSH32XY]  = OP_OTHERS,
    [MOV32XC]  = OP_OTHERS,
    [MOV32XY]  = OP_OTHERS,
    [ARSH32XC] = OP_OTHERS,
    [ARSH32XY] = OP_OTHERS,
    [LE16]     = OP_OTHERS,
    [LE32]     = OP_OTHERS,
    [LE64]     = OP_OTHERS,
    [BE16]     = OP_OTHERS,
    [BE32]     = OP_OTHERS,
    [BE64]     = OP_OTHERS,
    [JA]       = OP_UNCOND_JMP,
    [JEQXC]    = OP_COND_JMP,
    [JEQXY]    = OP_COND_JMP,
    [JGTXC]    = OP_COND_JMP,
    [JGTXY]    = OP_COND_JMP,
    [JSGTXC]   = OP_COND_JMP,
    [JSGTXY]   = OP_COND_JMP,
    [EXIT]     = OP_RET,
  };

  // Max and Min value for immediate number(32bits)、offset(16bits)
  static constexpr int64_t MAX_IMM = 0x7fffffff;
  static constexpr int64_t MIN_IMM = 0xffffffff80000000;
  static constexpr int64_t MAX_OFF = 0x7fff;
  static constexpr int64_t MIN_OFF = 0xffffffffffff8000;

  // Operand types for instructions
  static constexpr int OP_UNUSED = 0;
  static constexpr int OP_REG = 1;
  static constexpr int OP_IMM = 2;
  static constexpr int OP_OFF = 3;

  /* The definitions below assume a minimum 16-bit integer data type */
#define EBPF_OPTYPE(opcode, opindex) ((ebpf::optable[opcode] >> ((opindex) * 5)) & 31)
#define FSTOP(x) (x)
#define SNDOP(x) (x << 5)
#define TRDOP(x) (x << 10)
#define ALU_OPS_IMM (FSTOP(OP_REG) | SNDOP(OP_IMM) | TRDOP(OP_UNUSED))
#define ALU_OPS_REG (FSTOP(OP_REG) | SNDOP(OP_REG) | TRDOP(OP_UNUSED))
#define BYTESWAP (FSTOP(OP_REG) | SNDOP(OP_UNUSED) | TRDOP(OP_UNUSED))
#define JMP_OPS_IMM (FSTOP(OP_REG) | SNDOP(OP_IMM) | TRDOP(OP_OFF))
#define JMP_OPS_REG (FSTOP(OP_REG) | SNDOP(OP_REG) | TRDOP(OP_OFF))
#define UNUSED_OPS (FSTOP(OP_UNUSED) | SNDOP(OP_UNUSED) | TRDOP(OP_UNUSED))
  static constexpr int optable[NUM_INSTR] = {
    [NOP]      = UNUSED_OPS,
    [ADD64XC]  = ALU_OPS_IMM,
    [ADD64XY]  = ALU_OPS_REG,
    [LSH64XC]  = ALU_OPS_IMM,
    [LSH64XY]  = ALU_OPS_REG,
    [RSH64XC]  = ALU_OPS_IMM,
    [RSH64XY]  = ALU_OPS_REG,
    [MOV64XC]  = ALU_OPS_IMM,
    [MOV64XY]  = ALU_OPS_REG,
    [ARSH64XC] = ALU_OPS_IMM,
    [ARSH64XY] = ALU_OPS_REG,
    [ADD32XC]  = ALU_OPS_IMM,
    [ADD32XY]  = ALU_OPS_REG,
    [LSH32XC]  = ALU_OPS_IMM,
    [LSH32XY]  = ALU_OPS_REG,
    [RSH32XC]  = ALU_OPS_IMM,
    [RSH32XY]  = ALU_OPS_REG,
    [MOV32XC]  = ALU_OPS_IMM,
    [MOV32XY]  = ALU_OPS_REG,
    [ARSH32XC] = ALU_OPS_IMM,
    [ARSH32XY] = ALU_OPS_REG,
    [LE16]     = BYTESWAP,
    [LE32]     = BYTESWAP,
    [LE64]     = BYTESWAP,
    [BE16]     = BYTESWAP,
    [BE32]     = BYTESWAP,
    [BE64]     = BYTESWAP,
    [JA]       = FSTOP(OP_OFF) | SNDOP(OP_UNUSED) | TRDOP(OP_UNUSED),
    [JEQXC]    = JMP_OPS_IMM,
    [JEQXY]    = JMP_OPS_REG,
    [JGTXC]    = JMP_OPS_IMM,
    [JGTXY]    = JMP_OPS_REG,
    [JSGTXC]   = JMP_OPS_IMM,
    [JSGTXY]   = JMP_OPS_REG,
    [EXIT]     = UNUSED_OPS,
  };
#undef FSTOP
#undef SNDOP
#undef TRDOP
#undef ALU_OPS_IMM
#undef ALU_OPS_REG
#undef BYTESWAP
#undef JMP_OPS_IMM
#undef JMP_OPS_REG
#undef UNUSED_OPS
};

class ebpf_prog_state: public prog_state {
 public:
  ebpf_prog_state() {regs.resize(ebpf::NUM_REGS, 0);}
};

class ebpf_inst: public inst {
 public:
  static ebpf _isa;
  ebpf_inst(int opcode = _isa.NOP, int32_t arg1 = 0, int32_t arg2 = 0, int32_t arg3 = 0) {
    _args.resize(_isa.MAX_OP_LEN);
    _opcode  = opcode;
    _args[0] = arg1;
    _args[1] = arg2;
    _args[2] = arg3;
  }
  ebpf_inst& operator=(const inst &rhs);
  string opcode_to_str(int) const;
  int get_max_operand_val(int op_index, int inst_index = 0) const;
  void make_insts(vector<inst*> &instptr_list, const vector<inst*> &other) const;
  void make_insts(vector<inst*> &instptr_list, const inst* instruction) const;
  void clear_insts();
  int get_jmp_dis() const;
  void insert_jmp_opcodes(unordered_set<int>& jmp_set) const;
  int inst_output_opcode_type() const;
  int inst_output() const;
  bool is_real_inst() const;
  void set_as_nop_inst();
  // for class ebpf
  int get_num_regs() const {return _isa.NUM_REGS;}
  int get_max_prog_len() const {return _isa.MAX_PROG_LEN;}
  int get_max_op_len() const {return _isa.MAX_OP_LEN;}
  int get_op_num_bits() const {return _isa.OP_NUM_BITS;}
  int get_num_instr() const {return _isa.NUM_INSTR;}
  int get_num_operands() const {return _isa.num_operands[_opcode];}
  int get_insn_num_regs() const {return _isa.insn_num_regs[_opcode];}
  int get_opcode_type() const {return _isa.opcode_type[_opcode];}
  int64_t interpret(const vector<inst*> &instptr_list, prog_state &ps, int input = 0) const;
  // smt
  z3::expr smt_inst(smt_var& sv) const;
  z3::expr smt_inst_jmp(smt_var& sv) const;
};
