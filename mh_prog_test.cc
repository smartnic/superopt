#include <iostream>
#include <unordered_map>
#include <random>
#include <algorithm>
#include "prog.h"
#include "inout.h"
#include "mh_prog.h"

using namespace std;

#define N 6
inst instructions[N] = {inst(MOVXC, 2, 4),  /* mov r2, 4  */
                        inst(ADDXY, 1, 2),  /* add r1, r2 */
                        inst(MOVXC, 3, 15),  /* mov r3, 15  */
                        inst(JMPGT, 1, 3, 1),  /* if r1 <= r3: */
                        inst(RETX, 3),      /* ret r3 */
                        inst(RETX, 1),      /* else ret r1 */
};

inout ex_set[2];

void test1() {
  int nrolls = 10000;
  std::unordered_map<prog, int, progHash> prog_freq;

  prog orig(instructions, N);
  mcmc_iter(nrolls, orig, prog_freq, ex_set, 2);

  // Get the best program(s)
  int max = 0;
  int concurrent_max = 0;
  prog *best;
  for (std::pair<prog, int> element: prog_freq) {
    if (element.second >= max) {
      if (element.second == max) concurrent_max++;
      else concurrent_max = 0;
      max = element.second;
      best = &element.first;
    }
  }
  cout << "One of the best programs: " << endl;
  best->print();
  cout << "Observed frequency " << max << " out of " << nrolls << endl;
  cout << "Number of concurrently best programs:" << concurrent_max << endl;
  cout << "number of unique programs observed: " << prog_freq.size() << endl;
}

int main() {
  ex_set[0].set_in_out(10, 15);
  ex_set[1].set_in_out(16, 20);
  test1();
  return 0;
}
