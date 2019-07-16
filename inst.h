using namespace std;

#define NUM_REGS 4

class prog_state {
  int pc = 0; /* Assume only straight line code execution for now */
 public:
  int regs[NUM_REGS] = {}; /* assume only registers for now */
  void print();
};

class inst {
 private:
  string opcode_to_str();
 public:
  int _opcode;
  int _arg1;
  int _arg2;
  int _jmp_off;
  inst(int opcode, int arg1, int arg2=0, int jmp_off=0) {
    _opcode  = opcode;
    _arg1    = arg1;
    _arg2    = arg2;
    _jmp_off = jmp_off;
  }
  void print();
};

// Operand types for instructions
#define OP_UNUSED 0
#define OP_REG 1
#define OP_IMM 2
#define OP_OFF 3

// Instruction opcodes
#define ADDXY 0
#define MOVXC 1
#define RETX 2
#define RETC 3
#define JMPEQ 4
#define JMPGT 5
#define JMPGE 6
#define JMPLT 7
#define JMPLE 8
#define MAXC 9
#define MAXX 10

#define NUM_INSTR 11

/* The definitions below assume a minimum 16-bit integer data type */
#define FSTOP(x) (x)
#define SNDOP(x) (x << 5)
#define TRDOP(x) (x << 10)
#define OPTYPE(opcode, opindex) ((optable[opcode] >> ((opindex-1) * 5)) & 31)

#define JMP_OPS (FSTOP(OP_REG) | SNDOP(OP_REG) | TRDOP(OP_OFF))

static int optable[256] = {
  [ADDXY] = FSTOP(OP_REG) | SNDOP(OP_REG) | TRDOP(OP_UNUSED),
  [MOVXC] = FSTOP(OP_REG) | SNDOP(OP_IMM) | TRDOP(OP_UNUSED),
  [RETX]  = FSTOP(OP_REG) | SNDOP(OP_UNUSED) | TRDOP(OP_UNUSED),
  [RETC]  = FSTOP(OP_IMM) | SNDOP(OP_UNUSED) | TRDOP(OP_UNUSED),
  [JMPEQ] = JMP_OPS,
  [JMPGT] = JMP_OPS,
  [JMPGE] = JMP_OPS,
  [JMPLT] = JMP_OPS,
  [JMPLE] = JMP_OPS,
  [MAXC]  = FSTOP(OP_REG) | SNDOP(OP_IMM) | TRDOP(OP_UNUSED),
  [MAXX]  = FSTOP(OP_REG) | SNDOP(OP_REG) | TRDOP(OP_UNUSED),
  [NUM_INSTR ... 255] = FSTOP(OP_UNUSED) | SNDOP(OP_UNUSED) | TRDOP(OP_UNUSED),
};

static int num_operands[256] = {
  [ADDXY] = 2,
  [MOVXC] = 2,
  [RETX]  = 1,
  [RETC]  = 1,
  [JMPEQ] = 3,
  [JMPGT] = 3,
  [JMPGE] = 3,
  [JMPLT] = 3,
  [JMPLE] = 3,
  [MAXC]  = 2,
  [MAXX]  = 2,
  [NUM_INSTR ... 255] = 0,
};

void print_program(inst* program, int length);
int interpret(inst* program, int length, prog_state &ps);
