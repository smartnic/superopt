#pragma once

#include <iostream>
#include <vector>
#include "utils.h"
#include "../src/isa/inst_header.h"

using namespace std;

/* A class representing one input-output example.  Currently, very simple and
   assumes a single integer input and a single integer output. */
class inout {
 public:
  inout_t input;
  inout_t output;
  inout();
  inout(const inout_t& in, const inout_t& out);
  void set_in_out(const inout_t& in, const inout_t& out);
  void clear();
  void operator=(const inout &rhs);
  friend ostream& operator<< (ostream& out, const inout &_inout);
  friend ostream& operator<< (ostream& out, const vector<inout> &_inout_vec);
};

/* Class examples is a set of inouts with different input values. */
class examples {
 public:
  vector<inout> _exs;
  examples();
  ~examples();
  void insert(const inout& ex);
  unsigned int size() {return _exs.size();}
  void clear();
};
