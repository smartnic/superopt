#include <iostream>
#include <vector>
#include "ex.h"
#include "test.h"

using namespace std;

void test1() {
  cout << "test 1 starts...\n";
  // output = max(input+4, 15)
  inst instructions[6] = {inst(MOVXC, 2, 4),  /* mov r2, 4  */
                          inst(ADDXY, 0, 2),  /* add r0, r2 */
                          inst(MOVXC, 3, 15),  /* mov r3, 15  */
                          inst(JMPGT, 0, 3, 1),  /* if r0 <= r3: */
                          inst(RETX, 3),      /* ret r3 */
                          inst(RETX, 0),      /* else ret r0 */
                         };
  examples ex_set;
  ex_set.init(instructions, 6, 5);
  bool assert_res = true;
  for (size_t i = 0; i < ex_set._exs.size(); i++) {
    int input = ex_set._exs[i].input;
    int output = ex_set._exs[i].output;
    if (input < 0 || input > 50 || output != max(input + 4, 15)) {
      assert_res = false;
      break;
    }
  }
  print_test_res(assert_res, "examples::init(inst* orig, int len, "\
                 "int num_ex, int min, int max)");

  vector<inout> examples(5);
  for (int i = 0; i < examples.size(); i++) {
    examples[i].set_in_out(i, max(i + 4, 15));
  }
  ex_set.init(examples);
  assert_res = true;
  try {
    for (int i = 0; i < examples.size(); i++) {
      assert_res = (ex_set._exs[i].input == i) && \
                   (ex_set._exs[i].output == max(i + 4, 15));
    }
  } catch (const string err_msg) {
    assert_res = false;
  }
  print_test_res(assert_res, "examples::init(const vector<inout> &ex_set)");
}

void test2() {
  cout << "test 2 starts...\n";
  inout io;
  io.set_in_out(1, 15);
  examples ex_set;
  ex_set.insert(io);
  bool assert_res = true;
  if (ex_set._exs.size() == 1) {
    assert_res = (ex_set._exs[0].input == 1) && \
                 (ex_set._exs[0].output == 15);
  } else {
    assert_res = false;
  }
  print_test_res(assert_res, "examples::insert nonexistent value");
  ex_set.insert(ex_set._exs[0]);
  if (ex_set._exs.size() == 1) {
    assert_res = (ex_set._exs[0].input == 1) && \
                 (ex_set._exs[0].output == 15);
  } else {
    assert_res = false;
  }
  print_test_res(assert_res, "examples::insert existent value");
}

int main() {
  test1();
  test2();
  return 0;
}
