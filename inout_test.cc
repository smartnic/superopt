#include <iostream>
#include <vector>
#include "inout.h"
#include "utils.h"

using namespace std;

void test1() {
  cout << "test 1 starts...\n";
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
  return 0;
}
