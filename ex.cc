#include "ex.h"

using namespace std;

examples::examples() {}

examples::~examples() {}

void examples::insert(const inout& ex) {
  if (_inputs.find(ex.input) != _inputs.end()) {
    return;
  }
  inout io;
  io.set_in_out(ex.input, ex.output);
  _inputs.insert(io.input);
  _exs.push_back(io);
}

void examples::clear() {
  _exs.clear();
  _inputs.clear();
}
