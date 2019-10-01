#pragma once

#include <iostream>
#include "inst.h"

using namespace std;

class prog {
 public:
  inst inst_list[MAX_PROG_LEN];
  int freq_count;
  prog(const prog& other);
  prog(inst* instructions);
  prog();
  void print();
  static void print(const prog &p);
  ~prog();
  bool operator==(const prog &x) const;
  static prog* make_prog(const prog &x);
  static void clear_prog(prog* p);
};

struct progHash {
  size_t operator()(const prog &x) const;
};

