#include <vector>
#include <iostream>
#include <algorithm>
#include "../../../src/verify/cfg.h"
#include "canonicalize.h"

void canonicalize_prog_without_branch(unordered_set<int>& live_regs,
                                      inst* program, int start, int end,
                                      const unordered_set<int>& initial_live_regs) {
  live_regs = initial_live_regs;
  // liveness analysis is from the program end to the program start
  for (int i = end; i >= start; i--) {
    vector<int> regs_to_read;
    program[i].regs_to_read(regs_to_read);
    int reg_to_write = program[i].reg_to_write();
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

void remove_nops(inst* program, int len) {
  // steps: 1. modify jmp distances 2. remove nops
  // 1. modify jmp distances
  for (int i = 0; i < len; i++) {
    // search jmp instructions
    int op_type = program[i].get_opcode_type();
    if ((op_type != OP_UNCOND_JMP) && (op_type != OP_COND_JMP)) continue;
    // calculate new jmp distance and set it as jmp insn's jmp distance
    int jmp_dis = program[i].get_jmp_dis();
    int insn_start = min(i, jmp_dis + i) + 1;
    int insn_end = max(i, jmp_dis + i);
    int nop_counter = 0;
    for (int j = insn_start; j <= insn_end; j++) {
      if (program[j]._opcode == LDMAPID) { // ldmapid has two insns, the second one's opcode is nop
        j++;
      }
      if (program[j].get_opcode_type() == OP_NOP) {
        nop_counter++;
      }
    }
    int new_jmp_dis = 0;
    if (jmp_dis > 0) new_jmp_dis = jmp_dis - nop_counter;
    else new_jmp_dis = jmp_dis + nop_counter;
    program[i].set_jmp_dis(new_jmp_dis);
  }

  // 2. remove nops
  // move nops to the program end, i.e., nops follow by real instructions
  // program = concat(real instructions, nops)
  int next_new_insn = 0;
  for (int i = 0; i < len; i++) {
    bool is_nop = false;
    if (program[i].get_opcode_type() == OP_NOP) {
      if (i > 0) {
        if (program[i - 1]._opcode != LDMAPID) {
          is_nop = true;
        }
      }
    }
    if (is_nop) continue;
    if (i != next_new_insn) {
      program[next_new_insn] = program[i];
    }
    next_new_insn++;
  }
  for (int i = next_new_insn; i < len; i++) {
    program[i].set_as_nop_inst();
  }
}

// 1. set dead code as NOP
void canonicalize(inst* program, int len) {
  // get cfg of the program
  graph g(program, len);
  vector<unsigned int> blocks;
  topo_sort_for_graph(blocks, g);
  reverse(blocks.begin(), blocks.end());  // backward
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
    canonicalize_prog_without_branch(block_live_regs[b],
                                     program, g.nodes[b]._start, g.nodes[b]._end,
                                     initial_live_regs);
  }

  // remove nops
  remove_nops(program, len);
}
