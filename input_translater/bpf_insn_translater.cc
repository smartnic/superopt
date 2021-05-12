#include <fstream>
#include <iostream>
#include "../src/isa/ebpf/inst.h"
#include "bpf_insn.h"
#include "bpf_insn_prog.h"

using namespace std;

void bpf_insn_prog_to_inst(vector<inst>& insns) {
  insns.clear();
  int prog_len = sizeof(prog) / sizeof(bpf_insn);
  insns.resize(prog_len);
  for (int i = 0; i < prog_len; i++) {
    insns[i]._opcode = prog[i].code;
    insns[i]._dst_reg = prog[i].dst_reg;
    insns[i]._src_reg = prog[i].src_reg;
    insns[i]._imm = prog[i].imm;
    insns[i]._off = prog[i].off;
  }
}

int main(int argc, char *argv[]) {
  if (argc != 2) {
    cout << "No output file argument" << endl;
    return 1;
  }
  string output_file = argv[1];
  vector<inst> insns;
  bpf_insn_prog_to_inst(insns);
  write_insns_to_file_in_bpf_insn(insns, output_file);
}
