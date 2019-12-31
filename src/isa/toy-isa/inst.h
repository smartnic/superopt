#pragma once

#include <bitset>
#include <vector>
#include "../inst.h"

using namespace std;

#define MAX_CONST 20

// Operand types for instructions
#define OP_UNUSED 0
#define OP_REG 1
#define OP_IMM 2
#define OP_OFF 3

// Instruction opcodes
#define NOP 0
#define ADDXY 1
#define MOVXC 2
#define RETX 3
#define RETC 4
#define JMP 5
#define JMPEQ 6
#define JMPGT 7
#define JMPGE 8
#define JMPLT 9
#define JMPLE 10
#define MAXC 11
#define MAXX 12

#define OP_ABS_BIT_LEN 5
#define INST_ABS_BIT_LEN 20
// For absolute coding of each instruction
typedef bitset<INST_ABS_BIT_LEN> abs_bv_inst;

/* The definitions below assume a minimum 16-bit integer data type */
#define FSTOP(x) (x)
#define SNDOP(x) (x << 5)
#define TRDOP(x) (x << 10)
#define OPTYPE(opcode, opindex) ((optable[opcode] >> ((opindex) * 5)) & 31)

#define JMP_OPS (FSTOP(OP_REG) | SNDOP(OP_REG) | TRDOP(OP_OFF))
#define UNUSED_OPS (FSTOP(OP_UNUSED) | SNDOP(OP_UNUSED) | TRDOP(OP_UNUSED))

class toy_isa {
 public:
  static constexpr int NUM_REGS = 4;
  static constexpr int MAX_PROG_LEN = 7;
  // Max number of operands in one instruction
  static constexpr int MAX_OP_LEN = 3;
  // Number of opcode types
  static constexpr int NUM_INSTR = 13;
};

class prog_state {
  int pc = 0; /* Assume only straight line code execution for now */
 public:
  int regs[toy_isa::NUM_REGS] = {}; /* assume only registers for now */
  void print();
  void clear();
};

class inst {
 public:
  static toy_isa _isa;
  int _opcode;
  int _args[3];
  inst(int opcode = NOP, int arg1 = 0, int arg2 = 0, int arg3 = 0) {
    _opcode  = opcode;
    _args[0] = arg1;
    _args[1] = arg2;
    _args[2] = arg3;
  }
  void print() const;
  string opcode_to_str(int) const;
  abs_bv_inst inst_to_abs_bv() const;
  vector<int> get_reg_list() const;
  bool operator==(const inst &x) const;
  inst& operator=(const inst &rhs);
  int get_max_operand_val(int op_index, int inst_index = 0) const;
  int get_operand(int op_index) const;
  void set_operand(int op_index, int op_value);
  int get_opcode() const;
  void set_opcode(int op_value);
  int get_jmp_dis() const;
  // for class toy_isa
  int get_num_regs() const {return _isa.NUM_REGS;}
  int get_max_prog_len() const {return _isa.MAX_PROG_LEN;}
  int get_max_op_len() const {return _isa.MAX_OP_LEN;}
  int get_num_instr() const {return _isa.NUM_INSTR;}
};

struct instHash {
  size_t operator()(const inst &x) const;
};

static int optable[toy_isa::NUM_INSTR] = {
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

static int num_operands[toy_isa::NUM_INSTR] = {
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

static int num_regs[toy_isa::NUM_INSTR] = {
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

static int opcode_type[toy_isa::NUM_INSTR] = {
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

#define DSTREG(inst_var) (inst_var)._args[0]
#define SRCREG(inst_var) (inst_var)._args[1]
#define IMM1VAL(inst_var) (inst_var)._args[0]
#define IMM2VAL(inst_var) (inst_var)._args[1]

void print_program(const inst* program, int length);
void print_program(const vector<inst>& program);
int interpret(inst* program, int length, prog_state &ps, int input);
int num_real_instructions(inst* program, int len);
int num_real_instructions(const vector<inst>& program);
string to_string(const abs_bv_inst &bv);
// assume `inst_end` is the end instruction of a program
int inst_output_opcode_type(const inst& inst_end);
int inst_output(const inst& inst_end);
