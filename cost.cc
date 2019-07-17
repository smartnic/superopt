#include <iostream>
#include <random>
#include <algorithm>
#include <string>
#include "inst.h"

/* Requires support for advanced bit manipulation (ABM) instructions on the
 * architecture where this program is run. */
unsigned int pop_count_asm(unsigned int x) {
  unsigned int y = x;
  unsigned int z;
  asm ("popcnt %1, %0"
       : "=a" (z)
       : "b" (y)
       );
  cout << y << " " << z << endl;
  return z;
}
