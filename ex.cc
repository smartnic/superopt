#include <random>
#include "ex.h"

using namespace std;

default_random_engine gen;
uniform_real_distribution<double> unidist(0.0, 1.0);

examples::examples() {}

examples::~examples() {}

// randomly generate and store num_ex examples according to original program
void examples::init(inst* orig, int len, int num_ex, int min, int max) {
  clear();
  for (int i = 0; i < num_ex; i++) {
    double num = unidist(gen);
    _inputs.insert((int)(min + num * (max - min)));
  }
  prog_state ps;
  for (auto input : _inputs) {
    int output = interpret(orig, len, ps, input);
    inout io;
    io.set_in_out(input, output);
    _exs.push_back(io);
  }
}

// store examples according to inout set
void examples::init(const vector<inout> &ex_set) {
  clear();
  for (size_t i = 0; i < ex_set.size(); i++) {
    insert(ex_set[i]);
  }
}

void examples::insert(const inout& ex) {
  inout io;
  io.set_in_out(ex.input, ex.output);
  if (_inputs.find(io.input) != _inputs.end()) {
    return;
  }
  _inputs.insert(io.input);
  _exs.push_back(io);
}

void examples::clear() {
  _exs.clear();
  _inputs.clear();
}
