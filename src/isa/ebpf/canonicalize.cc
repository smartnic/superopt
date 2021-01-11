#include <vector>
#include <iostream>
#include <algorithm>
#include <utility>
#include "canonicalize.h"

default_random_engine gen_ebpf_cano;
uniform_real_distribution<double> unidist_ebpf_cano(0.0, 1.0);

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
    int nop_count = 0;
    for (int j = insn_start; j <= insn_end; j++) {
      if (program[j].get_opcode_type() == OP_NOP) {
        bool is_real_nop = true;
        if (j > 0) {
          // ldmapid has two insns, the second one's opcode is nop
          if (program[j - 1]._opcode == LDMAPID) is_real_nop = false;
        }
        if (is_real_nop) nop_count++;
      }
    }
    int new_jmp_dis = 0;
    if (jmp_dis > 0) new_jmp_dis = jmp_dis - nop_count;
    else new_jmp_dis = jmp_dis + nop_count;
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
      // search reg_state in self.reg_state. if it is, no need to insert
      bool flag = false;
      for (int j = 0; j < reg_state[reg].size(); j++) {
        if (reg_state[reg][j] == iss.reg_state[reg][i]) {
          flag = true;
          break;
        }
      }
      if (! flag) {
        reg_state[reg].push_back(iss.reg_state[reg][i]);
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
  reg_state = rhs.reg_state;
  live_var.regs = rhs.live_var.regs;
  live_var.mem = rhs.live_var.mem;
  return *this;
}

ostream& operator<<(ostream& out, const inst_static_state& x) {
  out << "reg_state: ";
  for (int i = 0; i < x.reg_state.size(); i++) {
    if (x.reg_state[i].size() == 0) continue;
    out << "r" << i << ":";
    for (int j = 0; j < x.reg_state[i].size(); j++) {
      out << x.reg_state[i][j] << " ";
    }
  }
  out << endl;
  out << "live variables: " << x.live_var << endl;
  return out;
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

// update dst_reg's state if insn is JEQXC dst_reg 0 offset, and dst_reg's type == `map value or NULL`
// after JEQXC, dst_reg is either `map value`(for not_jmp, i.e., next insn)
// or `NULL`(for jmp, i.e., next insn + offset)
void type_const_inference_inst_JEQXC(inst_static_state& iss, inst& insn, bool not_jmp) {
  if (insn._opcode != JEQXC) return;
  int dst_reg = insn._dst_reg;
  int imm = insn._imm;
  if (imm != 0) return;
  // check dst_reg's type
  for (int i = 0; i < iss.reg_state[dst_reg].size(); i++) {
    int type = iss.reg_state[dst_reg][i].type;
    if (type == PTR_TO_MAP_VALUE_OR_NULL) {
      register_state rs;
      if (not_jmp) {
        rs.type = PTR_TO_MAP_VALUE;
        rs.map_id = iss.reg_state[dst_reg][i].map_id;
        rs.off = iss.reg_state[dst_reg][i].off;
      } else {
        rs.type = SCALAR_VALUE;
        rs.val_flag = true;
        rs.val = 0;
      }
      // update register state
      iss.reg_state[dst_reg][i] = rs;
    }
  }
}

void type_const_inference_inst_block_start(inst_static_state& iss, int cur_insn,
    int block_in_insn_id, inst& block_in_insn) {
  bool not_jmp = false;
  if ((block_in_insn_id + 1) == cur_insn) {// not jmp
    not_jmp = true;
  }
  type_const_inference_inst_JEQXC(iss, block_in_insn, not_jmp);
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
  int off = insn._off;
  // keep strack of pointers and constant
  int pgm_input_type = mem_t::get_pgm_input_type();
  if (pgm_input_type == PGM_INPUT_pkt_ptrs) {
    // deal with pkt start pointer: check 1. src_reg's type is PTR_TO_CTX, off == 0
    // 2. insn is: LDXW ri rj 0 (ri = *(u32*)(rj+0))
    if ((opcode == LDXW) && (off == 0) && (iss.reg_state[src_reg].size() == 1)) {
      if ((iss.reg_state[src_reg][0].type == PTR_TO_CTX) &&
          (iss.reg_state[src_reg][0].off == 0)) {
        iss.set_reg_state(dst_reg, PTR_TO_PKT, 0);
        return; // return here in case the reg state is overwritten by the remaining code
      }
    }
  }
  if (opcode == MOV64XC) {
    register_state rs;
    rs.type = SCALAR_VALUE;
    rs.val = insn._imm;
    rs.val_flag = true;
    iss.set_reg_state(dst_reg, rs);
  } else if (opcode == MOV32XC) {
    register_state rs;
    rs.type = SCALAR_VALUE;
    rs.val = L32(insn._imm);
    rs.val_flag = true;
    iss.set_reg_state(dst_reg, rs);
  } else if (opcode == MOV64XY) {
    iss.copy_reg_state(dst_reg, src_reg);
  } else if (opcode == ADD64XC) {
    // update pointer offset
    for (int i = 0; i < iss.reg_state[dst_reg].size(); i++) {
      if (is_ptr(iss.reg_state[dst_reg][i].type)) {
        iss.reg_state[dst_reg][i].off += imm;
      } else if (iss.reg_state[dst_reg][i].val_flag) {
        iss.reg_state[dst_reg][i].val += imm;
      }
    }
  } else if (opcode == CALL) {
    iss.set_reg_state(0, SCALAR_VALUE);
    if (imm == BPF_FUNC_map_lookup_elem) {
      // get map id according to r1's value
      int map_id_reg = 1;
      vector<int> map_id_list;
      for (int i = 0; i < iss.reg_state[map_id_reg].size(); i++) {
        if (iss.reg_state[map_id_reg][i].type != CONST_PTR_TO_MAP) {
          if (logger.is_print_level(LOGGER_ERROR)) {
            cout << "ERROR: r1's type is not CONST_PTR_TO_MAP" << endl;
          }
          return;
        } else if (! iss.reg_state[map_id_reg][i].val_flag) {
          if (logger.is_print_level(LOGGER_ERROR)) {
            cout << "ERROR: r1 is not a const" << endl;
          }
          return;
        }
        int map_id = iss.reg_state[map_id_reg][i].val;
        if ((map_id < 0) || (map_id > mem_t::maps_number())) {
          if (logger.is_print_level(LOGGER_ERROR)) {
            cout << "ERROR: map_id " << map_id << " not in [0, # map]" << endl;
          }
          return;
        }
        map_id_list.push_back(map_id);
      }
      // clear register 0's state
      iss.reg_state[0] = {};
      for (int i = 0; i < map_id_list.size(); i++) {
        register_state rs;
        rs.type = PTR_TO_MAP_VALUE_OR_NULL;
        rs.map_id = map_id_list[i];
        rs.off = 0;
        iss.reg_state[0].push_back(rs);
      }
    }
  } else if (opcode == LDMAPID) {
    register_state rs;
    rs.type = CONST_PTR_TO_MAP;
    rs.val = insn._imm;
    rs.val_flag = true;
    iss.set_reg_state(dst_reg, rs);
  } else {
    iss.set_reg_state(dst_reg, SCALAR_VALUE);
  }
}

// compute ss.reg_state, ss.reg_state[i] stores the register states before executing insn i
void type_const_inference_pgm(prog_static_state& pss, inst* program, int len) {
  assert(pss.dag.size() >= 1);
  vector<inst_static_state>& ss = pss.static_state;
  graph& g = pss.g;
  vector<unsigned int>& dag = pss.dag;

  // `bss[i]` (temporarily) stores the register states after executing the basic block i.
  // These states are used for initializing the states of the first basic block insn
  vector<inst_static_state>& bss = pss.block_static_state;
  bss.resize(dag.size());

  // initialize the register states before executing the first insn by input
  ss[0].set_reg_state(1, PTR_TO_CTX); // register 1 contains input
  ss[0].set_reg_state(10, PTR_TO_STACK, STACK_SIZE); // register 10 points to the stack bottom

  // process blocks in order of dag
  for (int i = 0; i < dag.size(); i++) {
    unsigned int block = dag[i];
    unsigned int block_s = g.nodes[block]._start;
    unsigned int block_e = g.nodes[block]._end;
    // get the block initial states by merging incoming states from `bss`
    for (int j = 0; j < g.nodes_in[block].size(); j++) { // root block does not have incoming blocks
      int block_in = g.nodes_in[block][j];
      inst_static_state iss;
      iss = bss[block_in];
      int block_in_insn = g.nodes[block_in]._end;
      type_const_inference_inst_block_start(iss, block_s, block_in_insn, program[block_in_insn]);
      // ss[block_s].insert_reg_state(bss[block_in]);
      ss[block_s].insert_reg_state(iss);
    }
    // process the block from the first block insn
    for (int j = block_s; j < block_e; j++) {
      ss[j + 1].reg_state = ss[j].reg_state; // copy the previous state
      type_const_inference_inst(ss[j + 1], program[j]); // update state according to the insn
    }
    // update the basic block post register states
    bss[block].reg_state = ss[block_e].reg_state;
    type_const_inference_inst(bss[block], program[block_e]);
  }
}

void get_mem_write_offs(unordered_map<int, unordered_set<int>>& mem_write_offs,
                        vector<vector<register_state>>& reg_state, inst& insn) {
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
  for (int i = 0; i < reg_state[dst_reg].size(); i++) {
    int type = reg_state[dst_reg][i].type;
    if (! is_ptr(type)) continue;
    int off_s = reg_state[dst_reg][i].off + insn._off;
    if (mem_write_offs.find(type) == mem_write_offs.end()) {
      mem_write_offs[type] = unordered_set<int> {};
    }
    for (int j = 0; j < write_sz; j++) {
      mem_write_offs[type].insert(off_s + j);
    }
  }
}

void get_mem_read_regs_and_read_sz_from_helper(vector<pair<int, int> >& regs_sz,
    vector<vector<register_state>>& reg_state, inst& insn) {
  if (insn._opcode != CALL) return;
  int func_id = insn._imm;

  if ((func_id == BPF_FUNC_map_lookup_elem) || (func_id == BPF_FUNC_map_delete_elem)) {
    for (int i = 0; i < reg_state[1].size(); i++) { // r1 points to map id
      assert(reg_state[1][i].type == CONST_PTR_TO_MAP);
      assert(reg_state[1][i].val_flag);
      int map_id = reg_state[1][i].val;
      int k_sz = mem_t::map_key_sz(map_id) / NUM_BYTE_BITS;
      regs_sz.push_back({2, k_sz}); // r2 points to the key stored on stack
    }

  } else if (func_id == BPF_FUNC_map_update_elem) {
    for (int i = 0; i < reg_state[1].size(); i++) { // r1 points to map id
      assert(reg_state[1][i].type == CONST_PTR_TO_MAP);
      assert(reg_state[1][i].val_flag);
      int map_id = reg_state[1][i].val;
      int k_sz = mem_t::map_key_sz(map_id) / NUM_BYTE_BITS;
      int v_sz = mem_t::map_val_sz(map_id) / NUM_BYTE_BITS;
      regs_sz.push_back({2, k_sz}); // r2 points to the key stored on stack
      regs_sz.push_back({3, v_sz}); // r3 points to the value stored on stack
    }

  }
}

void get_mem_read_offs(unordered_map<int, unordered_set<int>>& mem_read_offs,
                       vector<vector<register_state>>& reg_state, inst& insn) {
  mem_read_offs.clear();
  vector<pair<int, int> > regs_sz; // regs and read sz
  switch (insn._opcode) {
    case LDXB: regs_sz.push_back({insn._src_reg, 1}); break;
    case LDXH: regs_sz.push_back({insn._src_reg, 2}); break;
    case LDXW: regs_sz.push_back({insn._src_reg, 4}); break;
    case LDXDW: regs_sz.push_back({insn._src_reg, 8}); break;
    case CALL: get_mem_read_regs_and_read_sz_from_helper(regs_sz, reg_state, insn); break;
    default: return;
  }

  for (int id = 0; id < regs_sz.size(); id++) {
    int reg = regs_sz[id].first;
    int read_sz = regs_sz[id].second;
    for (int i = 0; i < reg_state[reg].size(); i++) {
      int type = reg_state[reg][i].type;
      if (! is_ptr(type)) continue;
      if (mem_read_offs.find(type) == mem_read_offs.end()) {
        mem_read_offs[type] = unordered_set<int> {};
      }
      int off_s = reg_state[reg][i].off + insn._off;
      for (int j = 0; j < read_sz; j++) {
        mem_read_offs[type].insert(off_s + j);
      }
    }
  }
}

// live variables before the insn is executed
// input: insn and reg_state before the insn is executed
void live_analysis_inst(live_variables& live_var, vector<vector<register_state>>& reg_state, inst& insn) {
  // 1. update live registers
  vector<int> regs_to_read;
  insn.regs_to_read(regs_to_read);
  int reg_to_write = insn.reg_to_write();
  // remove reg_to_write in currrent live regs
  if (reg_to_write != -1) live_var.regs.erase(reg_to_write);
  // add regs_to_read in current live regs
  for (int i = 0; i < regs_to_read.size(); i++) {
    live_var.regs.insert(regs_to_read[i]);
  }

  // 2. update live memory
  // 2.1 update memory write
  bool is_mem_write = false;
  int op_class = BPF_CLASS(insn._opcode);
  if ((op_class == BPF_ST) || (op_class == BPF_STX)) is_mem_write = true;
  if (is_mem_write) {
    unordered_map<int, unordered_set<int>> mem_write_offs;
    get_mem_write_offs(mem_write_offs, reg_state, insn);
    // remove mem_write_offs from live memory
    for (auto it = mem_write_offs.begin(); it != mem_write_offs.end(); it++) {
      int type = it->first;
      auto found = live_var.mem.find(type);
      if (found == live_var.mem.end()) {
        continue;
      }
      unordered_set<int>& live_offs = found->second;
      unordered_set<int>& write_offs = it->second;
      for (auto it_off = write_offs.begin(); it_off != write_offs.end(); it_off++) {
        live_offs.erase(*it_off);
      }
      if (live_offs.size() == 0) {
        live_var.mem.erase(type);
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
    get_mem_read_offs(mem_read_offs, reg_state, insn);
    // add read_offs into live memory
    for (auto it = mem_read_offs.begin(); it != mem_read_offs.end(); it++) {
      int type = it->first;
      auto found = live_var.mem.find(type);
      if (found == live_var.mem.end()) {
        live_var.mem[type] = unordered_set<int> {};
      }
      found = live_var.mem.find(type);
      unordered_set<int>& live_offs = found->second;
      unordered_set<int>& read_offs = it->second;
      for (auto it_off = read_offs.begin(); it_off != read_offs.end(); it_off++) {
        live_offs.insert(*it_off);
      }
    }
  }
}

// compute ss.live_var, ss.live_var[i] stores the live variables after executing insn i
void live_analysis_pgm(prog_static_state& pss, inst* program, int len) {
  vector<inst_static_state>& ss = pss.static_state;
  graph& g = pss.g;
  vector<unsigned int>& dag = pss.dag;

  // `bss[i]` (temporarily) stores the live variables before executing the basic block i.
  // These states are used for initializing the states of the last basic block insn
  vector<inst_static_state>& bss = pss.block_static_state;
  bss.resize(dag.size());

  // process blocks backward
  for (int i = dag.size() - 1; i >= 0; i--) {

    unsigned int block = dag[i];
    int block_s = g.nodes[block]._start;
    int block_e = g.nodes[block]._end;

    // get the block initial live variables by merging outgoing live variables or from output
    if (g.nodes_out[block].size() == 0) { // from output (r0 + pkt)
      ss[block_e].insert_live_reg(0); // r0
      int pgm_input_type = mem_t::get_pgm_input_type();
      if (pgm_input_type == PGM_INPUT_pkt) {
        for (int j = 0; j < mem_t::_layout._pkt_sz; j++) { // pkt
          ss[block_e].insert_live_off(PTR_TO_CTX, j);
        }
      } else if (pgm_input_type == PGM_INPUT_pkt_ptrs) {
        for (int j = 0; j < 8; j++) { // input pointers, 8 bytes for two 32-bit pointers
          ss[block_e].insert_live_off(PTR_TO_CTX, j);
        }
        for (int j = 0; j < mem_t::_layout._pkt_sz; j++) { // pkt
          ss[block_e].insert_live_off(PTR_TO_PKT, j);
        }
      }
    } else {
      for (int j = 0; j < g.nodes_out[block].size(); j++) { // merging outgoing live variables
        int block_out = g.nodes_out[block][j];
        ss[block_e].insert_live_var(bss[block_out]);
      }
    }
    // process each block instruction backward
    for (int j = block_e; j > block_s; j--) {
      ss[j - 1].live_var = ss[j].live_var; // copy the previous state
      live_analysis_inst(ss[j - 1].live_var, ss[j].reg_state, program[j]); // update state according to the instruction
    }
    // update bss[i]'s live variables
    bss[block].live_var = ss[block_s].live_var;
    live_analysis_inst(bss[block].live_var, ss[block_s].reg_state, program[block_s]); // update state according to the instruction
  }
}

void static_analysis(prog_static_state& pss, inst* program, int len) {
  pss.clear();
  pss.static_state.resize(len + 1);
  // get program cfg
  pss.g.gen_graph(program, len);
  topo_sort_for_graph(pss.dag, pss.g);
  type_const_inference_pgm(pss, program, len);
  live_analysis_pgm(pss, program, len);
}

void safety_chk_insn(inst& insn, const vector<vector<register_state>>& reg_state) {
  vector<int> not_ptr_regs;
  insn.regs_cannot_be_ptrs(not_ptr_regs);
  for (int i = 0; i < not_ptr_regs.size(); i++) {
    int reg = not_ptr_regs[i];
    for (int j = 0; j < reg_state[reg].size(); j++) {
      int type = reg_state[reg][j].type;
      // todo: add PTR_TO_PKT
      unordered_set<int> ptrs = {PTR_TO_CTX, PTR_TO_STACK, PTR_TO_MAP_VALUE_OR_NULL};
      if (ptrs.find(type) == ptrs.end()) continue;
      // `JEQXC r 0` is legal if r.type == PTR_TO_MAP_VALUE_OR_NULL
      if ((type == PTR_TO_MAP_VALUE_OR_NULL) && (insn._opcode == JEQXC) && (insn._imm == 0)) {
        continue;
      }
      string err_msg = "illegal pointer operation of r" + to_string(reg);
      throw (err_msg);
    }
  }
}

void static_safety_check_pgm(inst* program, int len) {
  prog_static_state pss;
  pss.static_state.resize(len + 1);
  pss.g.gen_graph(program, len);
  topo_sort_for_graph(pss.dag, pss.g);
  type_const_inference_pgm(pss, program, len);
  for (int i = 0; i < len; i++) {
    safety_chk_insn(program[i], pss.static_state[i].reg_state);
  }
}

// update the original program's pre-condition and post-condition
void set_up_smt_inout_orig(prog_static_state& pss, inst* program, int len, int win_start, int win_end) {
  vector<inst_static_state>& ss = pss.static_state;
  assert(ss.size() >= win_end);
  smt_input::reg_state = ss[win_start].reg_state;
  smt_output::post_prog_r = ss[win_end].live_var;
}

void compute_win_w(live_variables& win_w, vector<inst_static_state>& ss_win, inst* program, int win_start, int win_end) {
  win_w.clear();
  for (int i = win_start; i <= win_end; i++) {
    int reg_to_write = program[i].reg_to_write();
    if (reg_to_write != -1) win_w.regs.insert(reg_to_write);

    bool is_mem_write = false;
    int op_class = BPF_CLASS(program[i]._opcode);
    if ((op_class == BPF_ST) || (op_class == BPF_STX)) is_mem_write = true;
    if (is_mem_write) {
      unordered_map<int, unordered_set<int>> mem_write_offs;
      get_mem_write_offs(mem_write_offs, ss_win[i - win_start].reg_state, program[i]);
      // remove mem_write_offs from live memory
      for (auto it = mem_write_offs.begin(); it != mem_write_offs.end(); it++) {
        int type = it->first;
        for (auto off : it->second) {
          win_w.mem[type].insert(off);
        }
      }
    }
  }
}

void set_up_smt_output_win(smt_output& sout, vector<inst_static_state>& ss_win,
                           vector<inst_static_state>& ss_orig,
                           inst* program, int win_start, int win_end) {
  live_variables win_w;
  compute_win_w(win_w, ss_win, program, win_start, win_end);
  // output_var = win_w intersection post_r
  live_variables post_r = ss_orig[win_end].live_var;
  live_variables::intersection(sout.output_var, win_w, post_r);
}

void set_up_smt_inout_win(smt_input& sin, smt_output& sout,
                          prog_static_state& pss_orig, inst* program, int win_start, int win_end) {
  int win_len = win_end - win_start + 1;
  // 1. compute ss_win according to the ss_orig and the window program
  vector<inst_static_state> ss_win(win_len); // [win_start, win_end]
  // get the initial reg state from pss_orig `win_start`
  ss_win[0].reg_state = pss_orig.static_state[win_start].reg_state;
  // todo: extended for window program with branches
  // update reg_state of win_program from insn 1 to len, reg_state: before executing insn
  for (int i = 0; i < win_len - 1; i++) {
    ss_win[i + 1].reg_state = ss_win[i].reg_state;
    type_const_inference_inst(ss_win[i + 1], program[i + win_start]);
  }

  // get the initial live variable from pss_orig `win_end`, live variable: after executing insn
  ss_win[win_len - 1].live_var = pss_orig.static_state[win_end].live_var;
  for (int i = win_len - 1; i > 0; i--) { // live analysis backward, from insn len to 1
    ss_win[i - 1].live_var = ss_win[i].live_var;
    live_analysis_inst(ss_win[i - 1].live_var, ss_win[i].reg_state, program[i + win_start]);
  }

  live_variables win_prog_r;
  win_prog_r = ss_win[0].live_var;
  live_analysis_inst(win_prog_r, ss_win[0].reg_state, program[win_start]);

  sin.prog_read = win_prog_r; // set up smt_input
  set_up_smt_output_win(sout, ss_win, pss_orig.static_state, program, win_start, win_end);
}

// todo: move random number generation into utils.h/cc
// Return a uniformly random integer from start to end inclusive
int random_int(int start, int end) {
  end++;
  int val;
  do {
    val = start + (int)(unidist_ebpf_cano(gen_ebpf_cano) * (double)(end - start));
  } while (val == end && end > start);
  return val;
}

uint64_t random_uint64(uint64_t start, uint64_t end) {
  if (end != 0xffffffffffffffff) end++;
  uint64_t val;
  do {
    val = start + (uint64_t)(unidist_ebpf_cano(gen_ebpf_cano) * (double)(end - start));
  } while (val == end && end > start);
  return val;
}

// generate random value of stack bottom
uint64_t gen_random_stack_bottom() {
  uint64_t max_uint32 = 0xffffffff;
  uint64_t stack_bottom_min = STACK_SIZE + 1; // address cannot be 0
  uint64_t mem_size_without_stack = get_mem_size_by_layout() - STACK_SIZE;
  uint64_t stack_bottom_max = max_uint32 - mem_size_without_stack - mem_t::_layout._pkt_sz;
  return random_uint64(stack_bottom_min, stack_bottom_max);
}

uint64_t gen_random_pkt_start(uint64_t stack_bottom) {
  uint64_t max_uint32 = 0xffffffff;
  uint64_t mem_size_without_stack = get_mem_size_by_layout() - STACK_SIZE;
  uint64_t pkt_min = stack_bottom + mem_size_without_stack;
  uint64_t pkt_max = max_uint32;
  return random_uint64(pkt_min, max_uint32);
}

void gen_random_input_for_common(vector<inout_t>& inputs, bool is_win) {
  // Generate common part for window and full program input:
  // 1. pkt, set pkt with random values; 2. is_win flag 3. input_simu_r10
  for (int i = 0; i < inputs.size(); i++) {
    inputs[i].set_pkt_random_val();
    inputs[i].set_skb_random_val();
    inputs[i].set_randoms_u32();
    inputs[i].is_win = is_win;
    inputs[i].input_simu_r10 = gen_random_stack_bottom();
  }
}

void gen_random_input(vector<inout_t>& inputs, int n, int64_t reg_min, int64_t reg_max) {
  inputs.clear();
  inputs.resize(n);
  gen_random_input_for_common(inputs, false);
  // Generate input register r1
  if (mem_t::_layout._pkt_sz == 0) { // case 1: r1 is not used as pkt address
    unordered_set<int64_t> reg_set; // use set to avoid that registers have the same values
    for (int i = 0; i < inputs.size();) {
      int64_t reg = reg_min + (reg_max - reg_min) * unidist_ebpf_cano(gen_ebpf_cano);
      if (reg_set.find(reg) == reg_set.end()) {
        reg_set.insert(reg);
        inputs[i].reg = reg;
        i++;
      }
    }
  } else { // case 2: r1 is used as pkt address
    for (int i = 0; i < inputs.size(); i++) {
      inputs[i].reg = gen_random_pkt_start(inputs[i].input_simu_r10);
      inputs[i].input_simu_pkt_s = inputs[i].reg;
    }
  }
}

void gen_random_input_for_win(vector<inout_t>& inputs, int n, inst_static_state& iss, int win_start, int win_end) {
  inputs.clear();
  inputs.resize(n);
  gen_random_input_for_common(inputs, true);

  // Generate random variables that have been written in precondition
  for (int i = 0; i < inputs.size(); i++) {
    uint64_t stack_bottom = inputs[i].input_simu_r10;
    uint64_t pkt_start = gen_random_pkt_start(stack_bottom);
    uint64_t stack_top = stack_bottom - STACK_SIZE;
    inputs[i].input_simu_pkt_s = pkt_start;
    uint64_t pkt_ptrs_start = random_uint64(0x100000000, 0x110000000);
    inputs[i].input_simu_pkt_ptrs_s = pkt_ptrs_start;
    // 1. Generate input_simu_pkt_ptrs for PGM_INPUT_pkt_ptrs
    int pgm_input_type = mem_t::get_pgm_input_type();
    if (pgm_input_type == PGM_INPUT_pkt_ptrs) {
      int real_pkt_sz = random_int(0, mem_t::_layout._pkt_sz);
      inputs[i].input_simu_pkt_ptrs[0] = pkt_start;
      // inputs[i].input_simu_pkt_ptrs[1] = pkt_start + real_pkt_sz - 1;
      inputs[i].input_simu_pkt_ptrs[1] = pkt_start + mem_t::_layout._pkt_sz - 1;
    }
    // 2. Generate registers
    uint64_t max_u64 = 0xffffffffffffffff;
    uint64_t min_u64 = 0;
    for (int reg = 0; reg < iss.reg_state.size(); reg++) {
      if (iss.reg_state[reg].size() == 0) continue;
      inputs[i].reg_readable[reg] = true;

      int max = iss.reg_state[reg].size() - 1;
      int min = 0;
      int sample = random_int(min, max);
      inputs[i].reg_type[reg] = iss.reg_state[reg][sample].type;

      int64_t reg_v;
      if (is_ptr(inputs[i].reg_type[reg])) {
        if (inputs[i].reg_type[reg] == PTR_TO_STACK) {
          reg_v = stack_top + iss.reg_state[reg][sample].off;
        } else if (inputs[i].reg_type[reg] == PTR_TO_CTX) {
          if (pgm_input_type == PGM_INPUT_pkt_ptrs) {
            reg_v = pkt_ptrs_start + iss.reg_state[reg][sample].off;
          } else {
            reg_v = pkt_start + iss.reg_state[reg][sample].off;
          }
        } else if (inputs[i].reg_type[reg] == PTR_TO_PKT) {
          if (pgm_input_type == PGM_INPUT_pkt_ptrs) {
            reg_v = pkt_start + iss.reg_state[reg][sample].off;
          }
        } else if (inputs[i].reg_type[reg] == PTR_TO_MAP_VALUE) {
          int map_id = iss.reg_state[reg][sample].map_id;
          int idx_in_map = iss.reg_state[reg][sample].off;
          assert(map_id >= 0);
          assert(map_id < mem_t::maps_number);
          unsigned int mem_off = mem_t::get_mem_off_by_idx_in_map(map_id, idx_in_map);
          reg_v = stack_top + mem_off;
        }
      } else {
        reg_v = random_uint64(min_u64, max_u64);
      }
      inputs[i].regs[reg] = reg_v;
    }

    // 3. Generte stack, use live_variable info
    // todo: do we need to compute and use precondition memory write
    auto it = iss.live_var.mem.find(PTR_TO_STACK);
    if (it != iss.live_var.mem.end()) {
      for (auto off : it->second) {
        inputs[i].stack_readble[off] = true;
        inputs[i].stack[off] = random_int(0, 0xff);
      }
    }
  }

}
