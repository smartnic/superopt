#pragma once

#include <bitset>
#include <vector>

using namespace std;

#define NUM_REGS 4
#define MAX_CONST 20
#define MAX_PROG_LEN 7
// Max number of operands in one instruction
#define MAX_OP_LEN 3

// Operand types for instructions
#define OP_UNUSED 0
#define OP_REG 1
#define OP_IMM 2
#define OP_OFF 3

// Opcode types for instructions
#define OP_NOP 0
#define OP_RET 1
#define OP_UNCOND_JMP 2
#define OP_COND_JMP 3
#define OP_OTHERS 4

// Return opcode types for the end instruction of a program
#define RET_C 0   // return immediate number
#define RET_X 1   // return register

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

#define NUM_INSTR 13

#define OP_ABS_BIT_LEN 5
#define INST_ABS_BIT_LEN 20
// For absolute coding of each instruction
typedef bitset<INST_ABS_BIT_LEN> abs_bv_inst;

class prog_state {
  int pc = 0; /* Assume only straight line code execution for now */
 public:
  int regs[NUM_REGS] = {}; /* assume only registers for now */
  void print();
  void clear();
};

class inst {
 public:
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
};

struct instHash {
  size_t operator()(const inst &x) const;
};

/* The definitions below assume a minimum 16-bit integer data type */
#define FSTOP(x) (x)
#define SNDOP(x) (x << 5)
#define TRDOP(x) (x << 10)
#define OPTYPE(opcode, opindex) ((optable[opcode] >> ((opindex) * 5)) & 31)

#define JMP_OPS (FSTOP(OP_REG) | SNDOP(OP_REG) | TRDOP(OP_OFF))
#define UNUSED_OPS (FSTOP(OP_UNUSED) | SNDOP(OP_UNUSED) | TRDOP(OP_UNUSED))

static int optable[256] = {
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
  [NUM_INSTR ... 255] = UNUSED_OPS,
};

static int num_operands[256] = {
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
  [NUM_INSTR ... 255] = 0,
};

static int num_regs[256] = {
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
  [NUM_INSTR ... 255] = 0,
};

static int opcode_type[256] = {
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
  [NUM_INSTR ... 255] = 0,
};

#define DSTREG(inst_var) (inst_var)._args[0]
#define SRCREG(inst_var) (inst_var)._args[1]
#define IMM1VAL(inst_var) (inst_var)._args[0]
#define IMM2VAL(inst_var) (inst_var)._args[1]

void print_program(const inst* program, int length);
int interpret(inst* program, int length, prog_state &ps, int input);
int num_real_instructions(inst* program, int len);
string to_string(const abs_bv_inst &bv);
// assume `inst_end` is the end instruction of a program
int inst_output_opcode_type(const inst& inst_end);
int inst_output(const inst& inst_end);
