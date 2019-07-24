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
  int nrolls = 10000;
  int nstars = 200;
  std::unordered_map<prog, int, progHash> prog_freq;

  // the first program
  prog orig(instructions, N);
  curr = prog::make_prog(orig);

  // MCMC iterations
  // IMPORTANT MEM ALLOC NOTE: The following possibilities have been tested
  // on os x g++ version 4.2.1:
  // - the memory of the provided key object is not used as is by storing a
  //   pointer in the map (later `free' of the key object from the map
  //   produced an "object not allocated" error)
  // - the object is not shallow copied into a fixed-size key object in the
  //   buckets of the map (`free' of enclosed allocated objects in the key
  //   produced an "object not allocated" error)
  // - the existing object looks to be deep-copied to form the new key.
  // I couldn't find the documented behavior of the standard allocator used by
  // operator[] of unordered_map. My approach right now is to assume that a
  // deep copy does indeed happen through the new_prog(provided_prog) copy
  // constructor.  This code needs to be investigated further for memory
  // leaks.
  for (int i=0; i<nrolls; i++) {
    cout << "Attempting mh iteration " << i << endl;
    next = mh_next(curr, orig, ex_set, 2);
    if (prog_freq.find(*next) == prog_freq.end()) prog_freq[*next] = 1;
    else prog_freq[*next]++;
    if (curr != next) prog::clear_prog(curr);
    else curr = next;
  }

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
