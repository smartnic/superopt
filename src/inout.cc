#include "inout.h"

using namespace std;

inout::inout() {}

inout::inout(const inout_t& in, const inout_t& out) {
  input = in;
  output = out;
}

void inout::set_in_out(const inout_t& _input, const inout_t& _output) {
  input = _input;
  output = _output;
}

void inout::clear() {
  input.clear();
  output.clear();
}

ostream& operator<< (ostream& out, const inout &_inout) {
  out << "input:" << _inout.input << "output:" << _inout.output;
  return out;
}

ostream& operator<< (ostream& out, const vector<inout> &_inout_vec) {
  for (size_t i = 0; i < _inout_vec.size(); i++) {
    out << _inout_vec[i] << " ";
  }
  return out;
}

examples::examples() {}

examples::~examples() {}

void examples::insert(const inout& ex) {
  inout io;
  io.set_in_out(ex.input, ex.output);
  _exs.push_back(io);
}

void examples::clear() {
  _exs.clear();
}
