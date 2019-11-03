#include "inout.h"

using namespace std;

void inout::set_in_out(int _input, int _output) {
  input = _input;
  output = _output;
}

ostream& operator<< (ostream& out, const inout &_inout) {
  out << _inout.input << "," << _inout.output;
  return out;
}

ostream& operator<< (ostream& out, const vector<inout> &_inout_vec) {
  for (size_t i = 0; i < _inout_vec.size(); i++) {
    out << _inout_vec[i] << " ";
  }
  return out;
}
