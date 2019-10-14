#include <iostream>
#include <random>
#include <algorithm>
#include <string>
#include "inst.h"
#include "inout.h"

/* Requires support for advanced bit manipulation (ABM) instructions on the
 * architecture where this program is run. */
unsigned int pop_count_asm(unsigned int x) {
  unsigned int y = x;
  unsigned int z;
  asm ("popcnt %1, %0"
       : "=a" (z)
       : "b" (y)
       );
  return z;
}

/* Compute correctness error metric between two programs on outputs */
int error_cost(inout* examples, int num_ex, inst* orig,
               inst* synth, bool exec_orig=false) {
  double total_cost = 0;
  prog_state ps;
  int output1, output2;
  for (int i = 0; i < num_ex; i++) {
    if (exec_orig)
      output1 = interpret(orig,  MAX_PROG_LEN, ps, examples[i].input);
    else
      output1 = examples[i].output;
    output2 = interpret(synth, MAX_PROG_LEN, ps, examples[i].input);
    cout << "Expected output: " << output1 << " Got output " << output2 << endl;
    // int ex_cost = pop_count_asm(output1 ^ output2);
    int ex_cost = abs(output1 - output2);
    total_cost += ex_cost;
    if (output1 != examples[i].output)
      cout << "Error: Original program output does not match provided "
          "input-output pair" << endl;
  }
  if (num_ex > 0) return (int)(total_cost);
  else return 0;
}

int num_real_instructions(inst* program) {
  int count = 0;
  for (int i=0; i < MAX_PROG_LEN; i++) {
    if (program[i]._opcode != NOP) count++;
  }
  return count;
}

int perf_cost(inout* examples, int num_ex, inst* orig,
              inst* synth) {
  return MAX_PROG_LEN - num_real_instructions(orig) +
      num_real_instructions(synth);
}
