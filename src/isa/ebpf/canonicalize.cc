#include <vector>
#include <iostream>
#include "../../../src/verify/cfg.h"
#include "canonicalize.h"

void liveness_analysis(unordered_set<int>& live_regs,
                       inst* program, int start, int end,
                       const unordered_set<int>& initial_live_regs) {
  live_regs = initial_live_regs;
  // liveness analysis is from the program end to the program start
  for (int i = end; i >= start; i--) {
    cout << i << ": ";
    program[i].print();
    vector<int> regs_to_read;
    program[i].regs_to_read(regs_to_read);
    int reg_to_write = program[i].reg_to_write();
    cout << "live regs: ";
    for (const int& x : live_regs) cout << x << " ";
    cout << endl;
    cout << "reg_to_write: " << reg_to_write << endl;
    cout << endl;
    cout << "regs_to_read: ";
    for (int i = 0; i < regs_to_read.size(); i++) cout << regs_to_read[i] << " ";
    cout << endl;
    // check whether the current insn is dead code, i.e., regs_to_write is not live
    bool is_dead_code = false;
    // if insn is memory write, the insn is not dead code
    // because live memory has not been implemented
    bool is_mem_write = false;
    int op_class = BPF_CLASS(program[i]._opcode);
    if ((op_class == BPF_ST) || (op_class == BPF_STX)) is_mem_write = true;
    if ((! is_mem_write) && (reg_to_write != -1)) {
      if (live_regs.find(reg_to_write) == live_regs.end()) {
        is_dead_code = true;
      }
    }
    if (! is_dead_code) { // if not the dead code, update the live regs
      // remove reg_to_write in currrent live regs
      if (reg_to_write != -1) live_regs.erase(reg_to_write);
      // add regs_to_read in current live regs
      for (int i = 0; i < regs_to_read.size(); i++) {
        live_regs.insert(regs_to_read[i]);
      }
    } else { // if the dead code, set the current insn as NOP
      program[i].set_as_nop_inst();
    }
  }
}

// 1. set dead code as NOP
void canonicalize(inst* program, int len) {
  // get cfg of the program
  graph g(program, len);
  // cout << g << endl;
  vector<unsigned int> blocks;
  topo_sort_for_graph(blocks, g);
  reverse(blocks.begin(), blocks.end());  // backward
  //
  vector<unordered_set<int>> block_live_regs(blocks.size());
  // init the initial live regs (register 0) for end blocks
  unordered_set<int> end_block_initial_live_regs = {0};
  // canonicalize basic block one by one backward
  for (int i = 0; i < blocks.size(); i++) {
    // 1. get the initial live regs
    unsigned int b = blocks[i];
    unordered_set<int> initial_live_regs;
    if (g.nodes_out[b].size() == 0) { // end block
      initial_live_regs = end_block_initial_live_regs;
    } else {
      // merge the live regs from all outgoing blocks (union operation)
      for (int j = 0; j < g.nodes_out[b].size(); j++) {
        unsigned int block_out = g.nodes_out[b][j];
        for (auto reg : block_live_regs[block_out]) {
          initial_live_regs.insert(reg);
        }
      }
    }

    // 2. canonicalize the current block and get the final live regs
    liveness_analysis(block_live_regs[b],
                      program, g.nodes[b]._start, g.nodes[b]._end,
                      initial_live_regs);
    cout << "final live regs " << i << ": " << b << " ";
    for (auto reg : block_live_regs[b]) cout << reg << " ";
    cout << endl;
  }
}
