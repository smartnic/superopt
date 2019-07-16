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
