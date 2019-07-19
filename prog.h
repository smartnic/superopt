#include <iostream>
#include "inst.h"

using namespace std;

class prog {
 public:
  inst* inst_list;
  int prog_length;
  prog(const prog& other);
  prog(inst* instructions, int prog_length);
  prog();
  void print();
  ~prog();
};


