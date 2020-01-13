#include <random>
#include <unordered_set>
#include "utils.h"

using namespace std;

void print_test_res(bool res, string test_name) {
  if (res) {
    std::cout << "check " + test_name + " SUCCESS\n";
  } else {
    std::cout << "check " + test_name + " NOT SUCCESS\n";
  }
}

default_random_engine gen_utils;
uniform_real_distribution<double> unidist_utils(0.0, 1.0);

void gen_random_input(vector<int>& inputs, int min, int max) {
  unordered_set<int> input_set;
  for (size_t i = 0; i < inputs.size();) {
    int input = min + (max - min) * unidist_utils(gen_utils);
    if (input_set.find(input) == input_set.end()) {
      input_set.insert(input);
      inputs[i] = input;
      i++;
    }
  }
}

ostream& operator<<(ostream& out, const vector<double>& vec) {
  for (size_t i = 0; i < vec.size(); i++) {
    out << vec[i] << " ";
  }
  return out;
}

void split_string(const string& s, vector<string>& v, const string& c) {
  std::string::size_type pos1, pos2;
  pos2 = s.find(c);
  pos1 = 0;
  while (std::string::npos != pos2) {
    v.push_back(s.substr(pos1, pos2 - pos1));
    pos1 = pos2 + c.size();
    pos2 = s.find(c, pos1);
  }
  if (pos1 != s.length())
    v.push_back(s.substr(pos1));
}


/* Requires support for advanced bit manipulation (ABM) instructions on the
 * architecture where this program is run. */
unsigned int pop_count_asm(unsigned int x) {
  unsigned int y = x;
  unsigned int z;
  asm ("popcnt %1, %0"
       : "=a" (z)
       : "b" (y)
      );
  return z;
}

bool is_little_endian() {
  int i = 1;
  char *p = (char *)&i;
  if (*p == 1) return true;
  return false;
}
