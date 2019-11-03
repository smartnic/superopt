#pragma once

#include <unordered_set>
#include "inout.h"
#include "inst.h"

using namespace std;

class examples {
 private:
  unordered_set<int> _inputs;
 public:
  vector<inout> _exs;
  examples();
  ~examples();
  void init(inst* orig, int len, int num_ex, int min = 0, int max = 50);
  void init(const vector<inout> &ex_set);
  void insert(const inout& ex);
  void clear();
};
