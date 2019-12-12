#pragma once

#include <iostream>
#include <vector>

using namespace std;

/* A class representing one input-output example.  Currently, very simple and
   assumes a single integer input and a single integer output. */
class inout {
 public:
  int input;
  int output;
  inout();
  inout(int in, int out);
  void set_in_out(int in, int out);
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
  void clear();
};
