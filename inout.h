#include <iostream>

/* A class representing one input-output example.  Currently, very simple and
   assumes a single integer input and a single integer output. */
class inout {
public:
  int input;
  int output;
  void set_in_out(int in, int out);
};
