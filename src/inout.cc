#include "inout.h"

using namespace std;

inout::inout() {
  input.init();
  output.init();
}

inout::inout(const inout_t& in, const inout_t& out) {
  input.init();
  output.init();
  input = in;
  output = out;
}

void inout::set_in_out(const inout_t& _input, const inout_t& _output) {
  input.init();
  output.init();
  input = _input;
  output = _output;
}

void inout::operator=(const inout &rhs) {
  input = rhs.input;
  output = rhs.output;
}

void inout::clear() {
  input.clear();
  output.clear();
}

ostream& operator<< (ostream& out, const inout &_inout) {
  out << "input:" << _inout.input << " output:" << _inout.output;
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
  _exs.push_back(io);
  _exs[size() - 1].set_in_out(ex.input, ex.output);
}

void examples::clear() {
  _exs.clear();
}
