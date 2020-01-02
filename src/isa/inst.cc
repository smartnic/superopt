#include <iostream>
#include "inst.h"

using namespace std;

void prog_state::print() {
  for (int i = 0; i < regs.size(); i++) {
    cout << "Register "  << i << " " << regs[i] << endl;
  }
};

void prog_state::clear() {
  pc = 0;
  for (int i = 0; i < regs.size(); i++) {
    regs[i] = 0;
  }
};
