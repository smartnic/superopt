#include <vector>
#include <iostream>
#include <algorithm>
#include <utility>
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
  register_state rs;
  rs.type = type;
  rs.off = off;
  reg_state[reg] = {rs};
}

void inst_static_state::set_reg_state(int reg, register_state rs) {
  assert(reg >= 0);
  assert(reg < NUM_REGS);
  reg_state[reg] = {rs};
}

// insert iss.reg_state into self.reg_state
void inst_static_state::insert_reg_state(inst_static_state& iss) {
  for (int reg = 0; reg < NUM_REGS; reg++) {
    for (int i = 0; i < iss.reg_state[reg].size(); i++) {
      int type = iss.reg_state[reg][i].type;
      int off = iss.reg_state[reg][i].off;
      int64_t val = iss.reg_state[reg][i].val;

      // search <type, off> in self.reg_state. if it is, no need to insert
      bool flag = false;
      for (int j = 0; j < reg_state[reg].size(); j++) {
        if ((reg_state[reg][j].type == type) &&
            (reg_state[reg][j].off == off) &&
            (reg_state[reg][j].val == val)) {
          flag = true;
          break;
        }
      }
      if (! flag) {
        reg_state[reg].push_back(register_state{type, off, val});
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

inst_static_state& inst_static_state::operator=(const inst_static_state &rhs) {
  // reg_state.resize(rhs.reg_state.size());
  // cout << "1.." << endl;
  // for (int i = 0; i < rhs.reg_state.size(); i++) {
  //   cout << i << endl;
  //   for (int j = 0; j < rhs.reg_state[i].size(); j++) {
  //     cout << i << " " << j << endl;
  //     reg_state[i].push_back(rhs.reg_state[i][j]);
  //   }
  // }
  reg_state = rhs.reg_state;
  live_var.regs = rhs.live_var.regs;
  live_var.mem = rhs.live_var.mem;
  return *this;
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

// After executing the insn, update register type in inst_static_state
void type_const_inference_inst(inst_static_state& iss, inst& insn) {
  int opcode_type = insn.get_opcode_type();
  vector<int> opcodes_upd_dst_reg = {OP_OTHERS, OP_LD, OP_CALL};
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
  } else if (opcode == LDMAPID) {
    register_state rs;
    rs.type = CONST_PTR_TO_MAP;
    rs.val = insn._imm;
    iss.set_reg_state(dst_reg, rs);
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
  cout << "get_mem_write_offs" << endl;
  insn.print();
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
    int off_s = iss.reg_state[dst_reg][i].off + insn._off;
    if (mem_write_offs.find(type) == mem_write_offs.end()) {
      mem_write_offs[type] = unordered_set<int> {};
    }
    for (int j = 0; j < write_sz; j++) {
      mem_write_offs[type].insert(off_s + j);
    }
  }
}

void get_mem_read_regs_and_read_sz_from_helper(vector<pair<int, int> >& regs_sz, inst_static_state& iss, inst& insn) {
  cout << "get_mem_read_regs_and_read_sz_from_helper" << endl;
  if (insn._opcode != CALL) return;
  int func_id = insn._imm;

  if ((func_id == BPF_FUNC_map_lookup_elem) || (func_id == BPF_FUNC_map_delete_elem)) {
    assert(iss.reg_state[1].size() == 1); // r1 points to map
    int map_id = iss.reg_state[1][0].val;
    int k_sz = mem_t::map_key_sz(map_id) / NUM_BYTE_BITS;
    regs_sz.push_back({2, k_sz});
    cout << map_id << " " << k_sz << endl;

  } else if (func_id == BPF_FUNC_map_update_elem) {
    assert(iss.reg_state[1].size() == 1);
    int map_id = iss.reg_state[1][0].val;
    int k_sz = mem_t::map_key_sz(map_id) / NUM_BYTE_BITS;
    int v_sz = mem_t::map_val_sz(map_id) / NUM_BYTE_BITS;
    regs_sz.push_back({2, k_sz});
    regs_sz.push_back({3, v_sz});

  }
}

void get_mem_read_offs(unordered_map<int, unordered_set<int>>& mem_read_offs,
                       inst_static_state& iss, inst& insn) {
  cout << "get_mem_read_offs: " << (insn._opcode == CALL) << endl;
  insn.print();
  mem_read_offs.clear();
  vector<pair<int, int> > regs_sz; // regs and read sz
  switch (insn._opcode) {
    case LDXB: regs_sz.push_back({insn._src_reg, 1}); break;
    case LDXH: regs_sz.push_back({insn._src_reg, 2}); break;
    case LDXW: regs_sz.push_back({insn._src_reg, 4}); break;
    case LDXDW: regs_sz.push_back({insn._src_reg, 8}); break;
    case CALL: get_mem_read_regs_and_read_sz_from_helper(regs_sz, iss, insn); break;
    default: return;
  }

  for (int id = 0; id < regs_sz.size(); id++) {
    int reg = regs_sz[id].first;
    int read_sz = regs_sz[id].second;
    for (int i = 0; i < iss.reg_state[reg].size(); i++) {
      int type = iss.reg_state[reg][i].type;
      if (! is_ptr(type)) continue;
      if (mem_read_offs.find(type) == mem_read_offs.end()) {
        mem_read_offs[type] = unordered_set<int> {};
      }
      int off_s = iss.reg_state[reg][i].off + insn._off;
      for (int j = 0; j < read_sz; j++) {
        mem_read_offs[type].insert(off_s + j);
      }
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
    cout << "mem write" << endl;
    // remove mem_write_offs from live memory
    for (auto it = mem_write_offs.begin(); it != mem_write_offs.end(); it++) {
      cout << "type: " << it->first << endl;
      int type = it->first;
      auto found = iss.live_var.mem.find(type);
      if (found == iss.live_var.mem.end()) {
        continue;
      }
      unordered_set<int>& live_offs = found->second;
      unordered_set<int>& write_offs = it->second;
      for (auto it_off = write_offs.begin(); it_off != write_offs.end(); it_off++) {
        cout << "write off: " << *it_off << endl;
        live_offs.erase(*it_off);
      }
    }
  }

  // 2.2 update memory read
  bool is_mem_read = false;
  // update map later
  if (insn._opcode == CALL) is_mem_read = true; // map helpers will read from stack
  if (op_class == BPF_LDX) is_mem_read = true;
  if (is_mem_read) {
    unordered_map<int, unordered_set<int>> mem_read_offs;
    get_mem_read_offs(mem_read_offs, iss, insn);
    cout << "mem_read_offs size: " << mem_read_offs.size() << endl;
    // add read_offs into live memory
    for (auto it = mem_read_offs.begin(); it != mem_read_offs.end(); it++) {
      cout << "type: " << it->first << endl;
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
      cout << j << endl;
      cout << pss[j].live_var << endl;
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

  // for (int i = 0; i < pss.size(); i++) {
  //   cout << "insn " << i << endl;
  //   for (int j = 0; j < pss[i].reg_state.size(); j++) {
  //     cout << "reg" << j << ": ";
  //     for (int k = 0; k < pss[i].reg_state[j].size(); k++) {
  //       cout << pss[i].reg_state[j][k].type << "," << pss[i].reg_state[j][k].off << " ";
  //     }
  //     cout << endl;
  //   }
  // }

  // for (int i = 0; i < pss.size(); i++) {
  //   cout << "insn " << i << endl;
  //   cout << "live regs: ";
  //   for (auto it = pss[i].live_var.regs.begin(); it != pss[i].live_var.regs.end(); it++) {
  //     cout << *it << " ";
  //   }
  //   cout << endl;
  //   cout << "live offs: ";
  //   for (auto it = pss[i].live_var.mem.begin(); it != pss[i].live_var.mem.end(); it++) {
  //     cout << it->first << ":";
  //     for (auto it2 = it->second.begin(); it2 != it->second.end(); it2++) {
  //       cout << *it2 << ",";
  //     }
  //     cout << " ";
  //   }
  //   cout << endl << endl;
  // }
  // cout << "......" << endl;
}

void set_up_smt_inout_orig(prog_static_state& pss, inst* program, int len, int win_start, int win_end) {
  assert(pss.size() >= win_end);
  smt_input::reg_state = pss[win_start - 1].reg_state;
  smt_output::post_prog_r = pss[win_end + 1].live_var;
  cout << "set_up_smt_inout_orig" << endl;
  cout << "smt_input: " << endl;
  for (int i = 0; i < smt_input::reg_state.size(); i++) {
    if (smt_input::reg_state[i].size() > 0) cout << i << ":";
    for (int j = 0; j < smt_input::reg_state[i].size(); j++) {
      cout << smt_input::reg_state[i][j].type << "," << smt_input::reg_state[i][j].off << " ";
    }
    if (smt_input::reg_state[i].size() > 0) cout << endl;
  }
  cout << "smt_output:" << endl;
  cout << "regs:";
  for (auto reg : smt_output::post_prog_r.regs) cout << reg << " ";
  cout << endl;
  cout << "offs:" << endl;;
  for (auto mem : smt_output::post_prog_r.mem) {
    cout << mem.first << ":";
    for (auto off : mem.second) {
      cout << off << " ";
    }
    cout << endl;
  }
}

void compute_win_w(live_variables& win_w, prog_static_state& pss_win, inst* program, int win_start, int win_end) {
  win_w.clear();
  for (int i = win_start; i <= win_end; i++) {
    int reg_to_write = program[i].reg_to_write();
    if (reg_to_write != -1) win_w.regs.insert(reg_to_write);

    bool is_mem_write = false;
    int op_class = BPF_CLASS(program[i]._opcode);
    if ((op_class == BPF_ST) || (op_class == BPF_STX)) is_mem_write = true;
    if (is_mem_write) {
      unordered_map<int, unordered_set<int>> mem_write_offs;
      get_mem_write_offs(mem_write_offs, pss_win[i - win_start], program[i]);
      // remove mem_write_offs from live memory
      for (auto it = mem_write_offs.begin(); it != mem_write_offs.end(); it++) {
        int type = it->first;
        for (auto off : it->second) {
          win_w.mem[type].insert(off);
        }
      }
    }
  }

  cout << "win_w " << endl << win_w << endl;
}

void set_up_smt_output_win(smt_output& sout, prog_static_state& pss_win,
                           prog_static_state& pss_orig,
                           inst* program, int win_start, int win_end) {
  cout << "set_up_smt_output_win" << endl;
  live_variables win_w;
  compute_win_w(win_w, pss_win, program, win_start, win_end);
  // output_var = win_w intersection post_r
  live_variables post_r = pss_orig[win_end + 1].live_var;
  cout << "post_r" << endl << post_r << endl;
  live_variables::intersection(sout.output_var, win_w, post_r);
  cout << "output_var:";
  cout << sout.output_var << endl;
}

void set_up_smt_inout_win(smt_input& sin, smt_output& sout,
                          prog_static_state& pss_orig, inst* program, int win_start, int win_end) {
  cout << "enter set_up_smt_inout_win" << endl;
  // 1. compute pss_win according to the pss_orig and the window program
  prog_static_state pss_win(win_end - win_start + 1); // [win_start, win_end]
  // get the initial state from `win_start-1`
  pss_win[0].reg_state = pss_orig[win_start - 1].reg_state;
  for (int i = 0; i < pss_win[0].reg_state.size(); i++) {
    cout << i << ": ";
    for (int j = 0; j < pss_win[0].reg_state[i].size(); j++) {
      cout << pss_win[0].reg_state[i][j].type << "," << pss_win[0].reg_state[i][j].off << "," << pss_win[0].reg_state[i][j].val << " ";
    }
    cout << endl;
  }
  // todo: extended for window program with branches
  for (int i = 0; i < pss_win.size(); i++) {
    if (i != 0) {
      pss_win[i].reg_state = pss_win[i - 1].reg_state;
    }
    type_const_inference_inst(pss_win[i], program[i + win_start]);
  }

  for (int i = pss_win.size() - 1; i >= 0; i--) { // live analysis backward
    if (i != pss_win.size() - 1) {
      pss_win[i].live_var = pss_win[i + 1].live_var;
    }
    live_analysis_inst(pss_win[i], program[i + win_start]);
  }

  sin.prog_read = pss_win[0].live_var; // set up smt_input
  cout << "set_up_smt_input_win" << endl;
  cout << "win_r: " << endl;
  cout << sin.prog_read << endl;
  set_up_smt_output_win(sout, pss_win, pss_orig, program, win_start, win_end);
}

void init_array_mem_table(smt_var& sv, inst_static_state& iss, int ptr_type, int mem_table_type) {
  auto it = iss.live_var.mem.find(ptr_type);
  if (it == iss.live_var.mem.end()) return;
  unordered_set<int>& stack_offs = it->second;
  int table_id = sv.mem_var.get_mem_table_id(mem_table_type);
  assert(table_id != -1);
  // add each off into urt
  int block = 0;  // set block as the root
  z3::expr is_valid = Z3_true;
  for (auto off : stack_offs) {
    z3::expr addr_off = to_expr(off);
    z3::expr val = sv.new_var(NUM_BYTE_BITS);
    sv.mem_var.add_in_mem_table_urt(table_id, block, is_valid, addr_off, val);
  }
}

void init_pre(smt_var& sv, inst_static_state& iss) {
  // init memory
  init_array_mem_table(sv, iss, PTR_TO_STACK, MEM_TABLE_stack);
  init_array_mem_table(sv, iss, PTR_TO_CTX, MEM_TABLE_pkt);
}

