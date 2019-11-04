#include <iostream>
#include "utils.h"

using namespace std;

void print_test_res(bool res, string test_name) {
  if (res) {
    std::cout << "check " + test_name + " SUCCESS\n";
  } else {
    std::cout << "check " + test_name + " NOT SUCCESS\n";
  }
}