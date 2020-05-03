#include <iostream>
#include <vector>
#include "utils.h"
#include "inout.h"

using namespace std;

void test1() {
  cout << "test 1 starts...\n";
  inout_t in, out;
  in.init();
  out.init();
  in.reg = 1;
  out.reg = 15;
  inout io;
  io.set_in_out(in, out);
  examples ex_set;
  ex_set.insert(io);
  bool assert_res = true;
  if (ex_set._exs.size() == 1) {
    assert_res = (ex_set._exs[0].input.reg == 1) && \
                 (ex_set._exs[0].output.reg == 15);
  } else {
    assert_res = false;
  }
  print_test_res(assert_res, "examples::insert nonexistent value");
  ex_set.insert(ex_set._exs[0]);
  if (ex_set._exs.size() == 1) {
    assert_res = (ex_set._exs[0].input.reg == 1) && \
                 (ex_set._exs[0].output.reg == 15);
  } else {
    assert_res = false;
  }
  print_test_res(!assert_res, "examples::insert existent value");
}

int main() {
  test1();
  return 0;
}
