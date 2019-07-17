#include <iostream>
#include "cost.h"

using namespace std;

int main() {
  #define NUM_INTS 6
  unsigned int ints_list[NUM_INTS] = {0, 1, 5, 7, 63, 114};
  for (int i = 0; i < NUM_INTS; i++) {
    cout << i << ": " << endl;
    pop_count_asm(ints_list[i]);
  }
  return 0;
}
