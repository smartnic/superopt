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
  bool operator==(const prog &x) const;
  static prog* make_prog(const prog &x);
  static void clear_prog(prog* p);
};

struct progHash {
  size_t operator()(const prog &x) const;
};

