#include <vector>
#include <iostream>
#include "canonicalize.h"

void liveness_analysis(unordered_set<int>& live_regs,
                       inst* program, int start, int end,
                       const unordered_set<int>& initial_live_regs) {
  live_regs = initial_live_regs;
  // liveness analysis is from the program end to the program start
  for (int i = end - 1; i >= start; i--) {
    cout << i << ": ";
    program[i].print();
    vector<int> regs_to_read;
    program[i].regs_to_read(regs_to_read);
    int reg_to_write = program[i].reg_to_write();
    cout << "live regs: ";
    for (const int& x : live_regs) cout << x << " ";
    cout << endl;
    cout << "reg_to_write: " << reg_to_write << endl;
    cout << endl;
    cout << "regs_to_read: ";
    for (int i = 0; i < regs_to_read.size(); i++) cout << regs_to_read[i] << " ";
    cout << endl;
    // check whether the current insn is dead code, i.e., regs_to_write is not live
    bool is_dead_code = false;
    if (reg_to_write != -1) {
      if (live_regs.find(reg_to_write) == live_regs.end()) {
        is_dead_code = true;
      }
    }
    if (! is_dead_code) { // if not the dead code, update the live regs
      // remove reg_to_write in currrent live regs
      if (reg_to_write != -1) live_regs.erase(reg_to_write);
      // add regs_to_read in current live regs
      for (int i = 0; i < regs_to_read.size(); i++) {
        live_regs.insert(regs_to_read[i]);
      }
    } else { // if the dead code, set the current insn as NOP
      program[i].set_as_nop_inst();
    }
  }
}
