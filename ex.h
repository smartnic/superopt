#pragma once

#include <unordered_set>
#include "inout.h"

using namespace std;

class examples {
 private:
  unordered_set<int> _inputs;
 public:
  vector<inout> _exs;
  examples();
  ~examples();
  void insert(const inout& ex);
  void clear();
};
