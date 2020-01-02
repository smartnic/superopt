#pragma once

#include <vector>

using namespace std;

// Opcode types for instructions
#define OP_NOP 0
#define OP_RET 1
#define OP_UNCOND_JMP 2
#define OP_COND_JMP 3
#define OP_OTHERS 4

// Return opcode types for the end instruction of a program
#define RET_C 0   // return immediate number
#define RET_X 1   // return register

class prog_state {
  int pc = 0; /* Assume only straight line code execution for now */
 public:
  vector<int> regs; /* assume only registers for now */
  void print();
  void clear();
};
