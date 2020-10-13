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

inst_static_state::inst_static_state() {
  reg_state.resize(NUM_REGS);
}

void inst_static_state::copy_reg_state(int dst_reg, int src_reg) {
  if (dst_reg == src_reg) return;
  assert(dst_reg >= 0);
  assert(dst_reg < NUM_REGS);
  assert(src_reg >= 0);
  assert(src_reg < NUM_REGS);
  reg_state[dst_reg] = reg_state[src_reg];
}

void inst_static_state::set_reg_state(int reg, int type, int off) {
  assert(reg >= 0);
  assert(reg < NUM_REGS);
  reg_state[reg] = {register_state{type, off}};
}

// insert iss.reg_state into self.reg_state
void inst_static_state::insert_reg_state(inst_static_state& iss) {
  for (int reg = 0; reg < NUM_REGS; reg++) {
    for (int i = 0; i < iss.reg_state[reg].size(); i++) {
      int type = iss.reg_state[reg][i].type;
      int off = iss.reg_state[reg][i].off;

      // search <type, off> in self.reg_state. if it is, no need to insert
      for (int j = 0; j < reg_state[reg].size(); j++) {
        if ((reg_state[reg][j].type == type) &&
            (reg_state[reg][j].off == off)) {
          break;
        }
      }
      reg_state[reg].push_back(register_state{type, off});
    }

  }
}

bool is_ptr(int type) {
  vector<int> ptr_array = {PTR_TO_STACK, PTR_TO_CTX, PTR_TO_MAP_VALUE_OR_NULL};
  for (int i = 0; i < ptr_array.size(); i++) {
    if (type == ptr_array[i]) {
      return true;
    }
  }
  return false;
}

// After executing the insn, update register type in inst_static_state
void type_const_inference_inst(inst_static_state& iss, inst& insn) {
  int opcode_type = insn.get_opcode_type();
  vector<int> opcodes_upd_dst_reg = {OP_OTHERS, OP_ST, OP_LD, OP_CALL};
  bool flag = false;
  for (int i = 0; i < opcodes_upd_dst_reg.size(); i++) {
    if (opcode_type == opcodes_upd_dst_reg[i]) {
      flag = true;
      break;
    }
  }
  if (! flag) return;
  int opcode = insn._opcode;
  int dst_reg = insn._dst_reg;
  int src_reg = insn._src_reg;
  int imm = insn._imm;
  // keep strack of pointers
  if (opcode == MOV64XY) {
    iss.copy_reg_state(dst_reg, src_reg);
  } else if (opcode == ADD64XC) {
    // update pointer offset
    for (int i = 0; i < iss.reg_state[dst_reg].size(); i++) {
      if (is_ptr(iss.reg_state[dst_reg][i].type)) {
        iss.reg_state[dst_reg][i].off += imm;
      }
    }
  } else if (opcode == CALL) {
    if (imm == BPF_FUNC_map_lookup_elem) {
      iss.set_reg_state(0, PTR_TO_MAP_VALUE_OR_NULL, 0);
    }
  } else {
    iss.set_reg_state(dst_reg, SCALAR_VALUE);
  }
}

void type_const_inference_pgm(prog_static_state& pss, inst* program, int len,
                              graph& g, vector<unsigned int>& dag) {
  assert(dag.size() >= 1);

  pss.resize(len);
  // init root block
  int root = dag[0];
  pss[root].set_reg_state(1, PTR_TO_CTX, 0); // register 1 contains input
  pss[root].set_reg_state(10, PTR_TO_STACK, STACK_SIZE); // register 10 points to stack bottom

  // process blocks
  for (int i = 0; i < dag.size(); i++) {
    unsigned int block = dag[i];
    unsigned int block_s = g.nodes[block]._start;
    unsigned int block_e = g.nodes[block]._end;
    // get the block first instruction states by merging incoming states
    for (int j = 0; j < g.nodes_in[block].size(); j++) {
      int block_in = g.nodes_in[block][j];
      unsigned int block_in_e = g.nodes[block_in]._end;
      pss[block_s].insert_reg_state(pss[block_in_e]);
    }
    // process the block from the second instruction
    for (int j = block_s; j <= block_e; j++) {
      if (j != 0) { // insn 0 is set in `init root block`
        pss[j] = pss[j - 1]; // copy the previous state
      }
      type_const_inference_inst(pss[j], program[j]); // update state according to the instruction
    }
  }
}

void static_analysis(inst* program, int len) {
  // get program cfg
  graph g(program, len);
  vector<unsigned int> dag;
  topo_sort_for_graph(dag, g);
  prog_static_state pss;
  type_const_inference_pgm(pss, program, len, g, dag);

  for (int i = 0; i < pss.size(); i++) {
    cout << "insn " << i << endl;
    for (int j = 0; j < pss[i].reg_state.size(); j++) {
      cout << "reg " << j << ": ";
      for (int k = 0; k < pss[i].reg_state[j].size(); k++) {
        cout << pss[i].reg_state[j][k].type << "," << pss[i].reg_state[j][k].off << " ";
      }
      cout << endl;
    }
  }
  cout << endl;
}
