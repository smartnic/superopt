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
      bool flag = false;
      for (int j = 0; j < reg_state[reg].size(); j++) {
        if ((reg_state[reg][j].type == type) &&
            (reg_state[reg][j].off == off)) {
          flag = true;
          break;
        }
      }
      if (! flag) {
        reg_state[reg].push_back(register_state{type, off});
      }
    }

  }
}

void inst_static_state::insert_live_reg(int reg) {
  live_var.regs.insert(reg);
}

void inst_static_state::insert_live_off(int type, int off) {
  auto found = live_var.mem.find(type);
  if (found == live_var.mem.end()) {
    live_var.mem[type] = {off};
  } else {
    found->second.insert(off);
  }
}

void inst_static_state::insert_live_var(inst_static_state& iss) {
  // insert live registers
  unordered_set<int>& live_regs = iss.live_var.regs;
  for (auto it = live_regs.begin(); it != live_regs.end(); it++) {
    insert_live_reg(*it);
  }
  // insert live memory
  unordered_map<int, unordered_set<int>>& live_mem = iss.live_var.mem;
  for (auto it = live_mem.begin(); it != live_mem.end(); it++) {
    int type = it->first;
    unordered_set<int>& offs = it->second;
    auto found = live_var.mem.find(type);
    if (found == live_var.mem.end()) {
      live_var.mem[type] = offs;
    } else {
      for (auto off = offs.begin(); off != offs.end(); off++) {
        live_var.mem[type].insert(*off);
      }
    }
  }
}

static void union_live_var(inst_static_state& iss, inst_static_state& iss1, inst_static_state& iss2) {
  iss.live_var.regs.clear();
  iss.live_var.mem.clear();
  iss.insert_live_var(iss1);
  iss.insert_live_var(iss2);
}

// iss.live_var = iss1.live_var intersection iss2.live_var
static void intersection_live_var(inst_static_state& iss, inst_static_state& iss1, inst_static_state& iss2) {
  iss.live_var.regs.clear();
  iss.live_var.mem.clear();

  // process live registers
  unordered_set<int>& live_reg1 = iss1.live_var.regs;
  unordered_set<int>& live_reg2 = iss2.live_var.regs;
  for (auto it = live_reg1.begin(); it != live_reg1.end(); it++) {
    int reg = *it;
    if (live_reg2.find(reg) != live_reg1.end()) {
      iss.insert_live_reg(reg);
    }
  }

  // process live memory
  unordered_map<int, unordered_set<int>> live_mem1 = iss1.live_var.mem;
  unordered_map<int, unordered_set<int>> live_mem2 = iss2.live_var.mem;
  for (auto it1 = live_mem1.begin(); it1 != live_mem1.end(); it1++) {
    int type = it1->first;
    auto it2 = live_mem2.find(type);
    if (it2 == live_mem2.end()) continue;
    unordered_set<int>& off_set1 = it1->second;
    unordered_set<int>& off_set2 = it2->second;
    for (auto off1 = off_set1.begin(); off1 != off_set1.end(); off1++) {
      if (off_set2.find(*off1) == off_set2.end()) continue;
      iss.insert_live_off(type, *off1);
    }
  }
}

bool is_ptr(int type) {
  vector<int> ptr_array = {PTR_TO_STACK, PTR_TO_CTX};
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
      // todo: need to modify the offset later
      iss.set_reg_state(0, PTR_TO_MAP_VALUE_OR_NULL, 0);
    }
  } else {
    iss.set_reg_state(dst_reg, SCALAR_VALUE);
  }
}

void type_const_inference_pgm(prog_static_state& pss, inst* program, int len,
                              graph& g, vector<unsigned int>& dag) {
  assert(dag.size() >= 1);

  // init root block
  int root = dag[0];
  pss[root].set_reg_state(1, PTR_TO_CTX, 0); // register 1 contains input
  pss[root].set_reg_state(10, PTR_TO_STACK, STACK_SIZE); // register 10 points to stack bottom

  // process blocks
  for (int i = 0; i < dag.size(); i++) {
    unsigned int block = dag[i];
    unsigned int block_s = g.nodes[block]._start;
    unsigned int block_e = g.nodes[block]._end;
    // get the block initial states by merging incoming states
    for (int j = 0; j < g.nodes_in[block].size(); j++) {
      int block_in = g.nodes_in[block][j];
      unsigned int block_in_e = g.nodes[block_in]._end;
      pss[block_s].insert_reg_state(pss[block_in_e]);
    }
    // process the block from the first instruction
    for (int j = block_s; j <= block_e; j++) {
      if (j != block_s) { // insn 0 is set in `init root block`
        pss[j].reg_state = pss[j - 1].reg_state; // copy the previous state
      }
      type_const_inference_inst(pss[j], program[j]); // update state according to the instruction
    }
  }
}

void get_mem_write_offs(unordered_map<int, unordered_set<int>>& mem_write_offs,
                        inst_static_state& iss, inst& insn) {
  mem_write_offs.clear();
  int write_sz;
  switch (insn._opcode) {
    case STB:
    case STXB: write_sz = 1; break;
    case STH:
    case STXH: write_sz = 2; break;
    case STW:
    case STXW: write_sz = 4; break;
    case STDW:
    case STXDW: write_sz = 8; break;
    default: return;
  }

  int dst_reg = insn._dst_reg;
  for (int i = 0; i < iss.reg_state[dst_reg].size(); i++) {
    int type = iss.reg_state[dst_reg][i].type;
    if (! is_ptr(type)) continue;
    int off_s = iss.reg_state[dst_reg][i].off;
    if (mem_write_offs.find(type) == mem_write_offs.end()) {
      mem_write_offs[type] = unordered_set<int> {};
    }
    for (int j = 0; j < write_sz; j++) {
      mem_write_offs[type].insert(off_s + j);
    }
  }
}

void get_mem_read_offs(unordered_map<int, unordered_set<int>>& mem_read_offs,
                       inst_static_state& iss, inst& insn) {
  mem_read_offs.clear();
  int read_sz;
  switch (insn._opcode) {
    case LDXB: read_sz = 1; break;
    case LDXH: read_sz = 2; break;
    case LDXW: read_sz = 4; break;
    case LDXDW: read_sz = 8; break;
    default: return;
  }

  int src_reg = insn._src_reg;
  for (int i = 0; i < iss.reg_state[src_reg].size(); i++) {
    int type = iss.reg_state[src_reg][i].type;
    if (! is_ptr(type)) continue;
    if (mem_read_offs.find(type) == mem_read_offs.end()) {
      mem_read_offs[type] = unordered_set<int> {};
    }
    int off_s = iss.reg_state[src_reg][i].off;
    for (int j = 0; j < read_sz; j++) {
      mem_read_offs[type].insert(off_s + j);
    }
  }
}

// live variables before the insn is executed
void live_analysis_inst(inst_static_state& iss, inst& insn) {
  cout << "live_analysis_inst" << endl;
  insn.print();
  // 1. update live registers
  vector<int> regs_to_read;
  insn.regs_to_read(regs_to_read);
  int reg_to_write = insn.reg_to_write();
  // remove reg_to_write in currrent live regs
  if (reg_to_write != -1) iss.live_var.regs.erase(reg_to_write);
  // add regs_to_read in current live regs
  for (int i = 0; i < regs_to_read.size(); i++) {
    iss.live_var.regs.insert(regs_to_read[i]);
  }

  // 2. update live memory
  // 2.1 update memory write
  bool is_mem_write = false;
  int op_class = BPF_CLASS(insn._opcode);
  if ((op_class == BPF_ST) || (op_class == BPF_STX)) is_mem_write = true;
  if (is_mem_write) {
    unordered_map<int, unordered_set<int>> mem_write_offs;
    get_mem_write_offs(mem_write_offs, iss, insn);
    // remove mem_write_offs from live memory
    for (auto it = mem_write_offs.begin(); it != mem_write_offs.end(); it++) {
      int type = it->first;
      auto found = iss.live_var.mem.find(type);
      if (found == iss.live_var.mem.end()) {
        continue;
      }
      unordered_set<int>& live_offs = found->second;
      unordered_set<int>& write_offs = it->second;
      for (auto it_off = write_offs.begin(); it_off != write_offs.end(); it_off++) {
        live_offs.erase(*it_off);
      }
    }
  }

  // 2.2 update memory read
  bool is_mem_read = false;
  // update map helper later
  if (op_class == BPF_LDX) is_mem_read = true;
  if (is_mem_read) {
    unordered_map<int, unordered_set<int>> mem_read_offs;
    get_mem_read_offs(mem_read_offs, iss, insn);
    cout << "mem_read_offs size: " << mem_read_offs.size() << endl;
    // add read_offs into live memory
    for (auto it = mem_read_offs.begin(); it != mem_read_offs.end(); it++) {
      int type = it->first;
      auto found = iss.live_var.mem.find(type);
      if (found == iss.live_var.mem.end()) {
        iss.live_var.mem[type] = unordered_set<int> {};
      }
      found = iss.live_var.mem.find(type);
      unordered_set<int>& live_offs = found->second;
      unordered_set<int>& read_offs = it->second;
      for (auto it_off = read_offs.begin(); it_off != read_offs.end(); it_off++) {
        cout << "read_off: " << *it_off << endl;
        live_offs.insert(*it_off);
      }
    }
  }
}

void live_analysis_pgm(prog_static_state& pss, inst* program, int len,
                       graph& g, vector<unsigned int>& dag) {
  cout << "live_analysis_pgm" << endl;
  // process blocks backward
  for (int i = dag.size() - 1; i >= 0; i--) {

    unsigned int block = dag[i];
    int block_s = g.nodes[block]._start;
    int block_e = g.nodes[block]._end;
    // get the block initial live variables by merging outgoing live variables
    cout << "get block initial live variables" << endl;
    for (int j = 0; j < g.nodes_out[block].size(); j++) {
      int block_out = g.nodes_out[block][j];
      unsigned int block_out_s = g.nodes[block_out]._start;
      pss[block_e].insert_live_var(pss[block_out_s]);
    }
    // process each block instruction backward
    cout << "processing block instruction" << endl;
    for (int j = block_e; j >= block_s; j--) {
      if (j != block_e) { // insn 0 is set in `init root block`
        pss[j].live_var = pss[j + 1].live_var; // copy the previous state
      }
      live_analysis_inst(pss[j], program[j]); // update state according to the instruction
    }
  }
  cout << "live_analysis_pgm end" << endl;

}

void static_analysis(prog_static_state& pss, inst* program, int len) {
  pss.clear();
  pss.resize(len);
  // get program cfg
  graph g(program, len);
  cout << g << endl;
  vector<unsigned int> dag;
  topo_sort_for_graph(dag, g);
  type_const_inference_pgm(pss, program, len, g, dag);
  live_analysis_pgm(pss, program, len, g, dag);
  cout << "static_analysis end" << endl;

  for (int i = 0; i < pss.size(); i++) {
    cout << "insn " << i << endl;
    for (int j = 0; j < pss[i].reg_state.size(); j++) {
      cout << "reg" << j << ": ";
      for (int k = 0; k < pss[i].reg_state[j].size(); k++) {
        cout << pss[i].reg_state[j][k].type << "," << pss[i].reg_state[j][k].off << " ";
      }
      cout << endl;
    }
  }

  for (int i = 0; i < pss.size(); i++) {
    cout << "insn " << i << endl;
    cout << "live regs: ";
    for (auto it = pss[i].live_var.regs.begin(); it != pss[i].live_var.regs.end(); it++) {
      cout << *it << " ";
    }
    cout << endl;
    cout << "live offs: ";
    for (auto it = pss[i].live_var.mem.begin(); it != pss[i].live_var.mem.end(); it++) {
      cout << it->first << ":";
      for (auto it2 = it->second.begin(); it2 != it->second.end(); it2++) {
        cout << *it2 << ",";
      }
      cout << " ";
    }
    cout << endl << endl;
  }
  cout << "......" << endl;
}


