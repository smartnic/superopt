#include <fstream>
#include "../src/isa/ebpf/inst.h"

using namespace std;

void str_to_inst(vector<inst>& insns, string str) {
  insns.clear();
  vector<string> substrs;
  split_string(str, substrs, " ");
  assert(substrs.size() > 0);
  string opcode_str = substrs[0];

  int opcode = inst::str_to_opcode(opcode_str);
  if (opcode == -1) {
    string err_msg = "illegal inst string";
    throw (err_msg);
  }
  int num_insts = inst::num_inst(opcode);
  insns.resize(num_insts);
  if (num_insts == 1) {
    insns[0]._opcode = opcode;
    for (int i = 1; i < substrs.size(); i++) {
      int operand = stoi(substrs[i]);
      int operand_type = OPTYPE(opcode, i - 1);
      switch (operand_type) {
        case OP_DST_REG: insns[0]._dst_reg = operand; break;
        case OP_SRC_REG: insns[0]._src_reg = operand; break;
        case OP_OFF: insns[0]._off = operand; break;
        case OP_IMM: insns[0]._imm = operand; break;
        default:
          cout << "Cannot find operand type for instruction: " << str << endl;
          return;
      }
    }
  } else if (num_insts == 2) {
    if (opcode_str == "LDMAPID") {
      assert(substrs.size() == 3);
      inst insns_array[2] = {INSN_LDMAPID(stoi(substrs[1]), stoi(substrs[2])),};
      insns[0] = insns_array[0];
      insns[1] = insns_array[1];
    } else if (opcode_str == "MOVDWXC") {
      assert(substrs.size() == 3);
      inst insns_array[2] = {INSN_MOVDWXC(stoi(substrs[1]), stoi(substrs[2])),};
      insns[0] = insns_array[0];
      insns[1] = insns_array[1];
    } else {
      string err_msg = "illegal inst string";
      throw (err_msg);
    }
  }
}

void read_inst_from_k2_inst_file(vector<inst>& insns, string input_file) {
  // read lines from input file
  ifstream file(input_file);
  if (! file.is_open()) {
    cout << "Cannot open the file " << input_file << endl;
    return;
  }
  string inst_str;
  while (getline(file, inst_str)) {
    vector<inst> insn;
    str_to_inst(insn, inst_str);
    for (int i = 0; i < insn.size(); i++) {
      insns.push_back(insn[i]);
    }
  }
}

int main(int argc, char *argv[]) {
  if (argc != 3) {
    cout << "No input or output file argument" << endl;
    return 1;
  }
  string input_file = argv[1];
  string output_file = argv[2];
  vector<inst> insns;
  read_inst_from_k2_inst_file(insns, input_file);
  write_insns_to_file_in_bpf_insn(insns, output_file);
}
