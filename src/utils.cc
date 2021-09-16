#include "utils.h"

using namespace std;

logger_class logger;

void print_test_res(bool res, string test_name) {
  if (res) {
    std::cout << "check " + test_name + " SUCCESS\n";
  } else {
    std::cout << "check " + test_name + " NOT SUCCESS\n";
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
