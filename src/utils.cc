#include <random>
#include "utils.h"

using namespace std;

logger_class logger;

default_random_engine gen_utils;
uniform_real_distribution<double> unidist_utils(0.0, 1.0);

// Return a uniformly random integer from start to end inclusive
int random_int(int start, int end) {
  end++;
  int val;
  do {
    val = start + (int)(unidist_utils(gen_utils) * (double)(end - start));
  } while (val == end && end > start);
  return val;
}

uint64_t random_uint64(uint64_t start, uint64_t end) {
  if (end != 0xffffffffffffffff) end++;
  uint64_t val;
  do {
    val = start + (uint64_t)(unidist_utils(gen_utils) * (double)(end - start));
  } while (val == end && end > start);
  return val;
}

double random_double_unit() {
  return unidist_utils(gen_utils);
}

/* Return a uniformly random integer from start to end inclusive, with the
 * exception of  `except`. */
int random_int_with_exception(int start, int end, int except) {
  end++;
  int val;
  do {
    val = start + (int)(unidist_utils(gen_utils) * (double)(end - start));
  } while ((val == end || val == except) && ((end - start) > 1));
  return val;
}

void print_test_res(bool res, string test_name) {
  if (res) {
    std::cout << "check " + test_name + " SUCCESS" << endl;
  } else {
    std::cout << "check " + test_name + " NOT SUCCESS" << endl;
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

// convert uint8_t vector to hex string
// e.g. addr[2] = {0x1, 0xff}, hex string: "01ff"
string uint8_t_vec_2_hex_str(const vector<uint8_t>& a) {
  stringstream ss;
  ss << hex << setfill('0');

  for (int i = 0; i < a.size(); i++) {
    ss << hex << setw(2) << static_cast<int>(a[i]);
  }

  return ss.str();
}
