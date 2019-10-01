#include "prog.h"

using namespace std;

// TODO: find canonical way to invoke one constructor from another
prog::prog(const prog& other) {
  freq_count = other.freq_count;
  for (int i=0; i < MAX_PROG_LEN; i++) {
    inst_list[i] = other.inst_list[i];
  }
}

prog::prog(inst* instructions) {
  freq_count = 0;
  for (int i=0; i < MAX_PROG_LEN; i++) {
    inst_list[i] = instructions[i];
  }
}

// TODO: find canonical way to invoke such a constructor + destructor
prog* prog::make_prog(const prog &other) {
  prog* new_prog = (prog*)malloc(sizeof(prog));
  new_prog->freq_count = 0;
  for (int i=0; i < MAX_PROG_LEN; i++) {
    new_prog->inst_list[i] = other.inst_list[i];
  }
  return new_prog;
}

void prog::clear_prog(prog* p) {
  free(p);
}

prog::prog() {
  freq_count = 0;
}

prog::~prog() {
}

void prog::print() {
  print_program(inst_list, MAX_PROG_LEN);
}

void prog::print(const prog &p) {
  print_program(p.inst_list, MAX_PROG_LEN);
}

bool prog::operator==(const prog &x) const {
  for (int i=0; i < MAX_PROG_LEN; i++) {
    if (! (inst_list[i] == x.inst_list[i])) return false;
  }
  return true;
}

size_t progHash::operator()(const prog &x) const {
  size_t hval = 0;
  for (int i=0; i < MAX_PROG_LEN; i++) {
    hval = hval ^ (instHash()(x.inst_list[i]) << (i % 4));
  }
  return hval;
}
