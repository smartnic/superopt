#include <iostream>
#include <unordered_map>
#include <random>
#include <algorithm>
#include "prog.h"
#include "inout.h"
#include "mh_prog.h"

using namespace std;

#define N 7
inst instructions[N] = {inst(MOVXC, 2, 4),  /* mov r2, 4  */
                        inst(ADDXY, 0, 2),  /* add r0, r2 */
                        inst(MOVXC, 3, 15),  /* mov r3, 15  */
                        inst(JMPGT, 0, 3, 1),  /* if r0 <= r3: */
                        inst(RETX, 3),      /* ret r3 */
                        inst(RETX, 0),      /* else ret r0 */
                        NOP,  /* control never reaches here */
};

inout ex_set[2];

void test1() {
  int nrolls = 10000;
  std::unordered_map<int, vector<prog*> > prog_freq;

  prog orig(instructions);
  mcmc_iter(nrolls, orig, prog_freq, ex_set, 2);

  // Get the best program(s)
  int max = 0;
  int concurrent_max = 0;
  prog *best;
  int nprogs = 0;
  for (std::pair<int, vector <prog*> > element: prog_freq) {
    vector<prog*> pl = element.second; // list of progs with the same hash
    for (auto p : pl) {
      nprogs++;
      if (p->freq_count > max) {
        concurrent_max = 0;
        best = p;
        max = p->freq_count;
      } else if (p->freq_count == max) {
        concurrent_max++;
      }
    }
  }
  cout << "One of the best programs: " << endl;

  cout << "Observed frequency " << max << " out of " << nrolls << endl;
  cout << "Number of concurrently best programs:" << concurrent_max << endl;
  cout << "number of unique hashes observed: " << prog_freq.size() << endl;
  cout << "number of unique programs observed: " << nprogs << endl;
  best->print();
}

int main() {
  ex_set[0].set_in_out(10, 15);
  ex_set[1].set_in_out(16, 20);
  test1();
  return 0;
}
