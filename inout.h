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
  void set_in_out(int in, int out);
};

ostream& operator<< (ostream& out, const inout &_inout);
ostream& operator<< (ostream& out, const vector<inout>& _inout_vec);
