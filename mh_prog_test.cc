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
  prog *curr, *next;
  int nrolls = 3;
  int nstars = 200;
  std::unordered_map<prog, int, progHash> prog_freq;
  std::unordered_map<prog, int, progHash>::const_iterator iter;

  // the first program
  prog orig(instructions, N);
  curr = prog::make_prog(orig);
  cout << "Original program:" << endl;
  orig.print();

  // MCMC iterations
  for (int i=0; i<nrolls; i++) {
    cout << "Attempting mh iteration " << i << endl;
    next = mh_next(curr, orig, ex_set, 2);
    iter = prog_freq.find(*next);
    if (iter == prog_freq.end()) {
      prog* key_prog = prog::make_prog(*next);
      prog_freq[*key_prog] = 1;
    } else prog_freq[*next]++;
    prog::clear_prog(curr);
    curr = next;
    cout << "Completed mh iteration " << i << endl;
  }

  // TODO Print best program 

  // clear all the keys in cost_freq
  for (std::pair<prog, int> element: prog_freq) {
    prog::clear_prog(&element.first);
  }
}

int main() {
  ex_set[0].set_in_out(10, 15);
  ex_set[1].set_in_out(16, 20);
  test1();
  return 0;
}
