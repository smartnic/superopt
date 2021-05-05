#include <vector>
#include <iostream>
#include <algorithm>
#include <utility>
#include <algorithm>
#include "canonicalize.h"

default_random_engine gen_ebpf_cano;
uniform_real_distribution<double> unidist_ebpf_cano(0.0, 1.0);
void get_mem_write_offs(unordered_map<int, unordered_set<int>>& mem_write_offs,
                        vector<vector<register_state>>& reg_state, inst& insn);

// if an instruction is real NOP, set it as JA 0
void set_nops_as_JA0(inst* program, int len) {
  inst insn_JA0 = inst(JA, 0);
  for (int i = 0; i < len; i++) {
    bool is_nop = false;
    if (program[i]._opcode == NOP) {
      is_nop = true;
      if (i > 0) {
        if (program[i - 1]._opcode == LDDW) { // LDDW contains two insns
          is_nop = false;
        }
      }
    }
    if (! is_nop) continue;
    program[i] = insn_JA0;
  }
}

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
      if (program[j]._opcode == NOP) {
        bool is_real_nop = true;
        if (j > 0) {
          // LDDW has two insns, the second one's opcode is nop
          if (program[j - 1]._opcode == LDDW) is_real_nop = false;
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
    if (program[i]._opcode == NOP) {
      is_nop = true;
      if (i > 0) {
        if (program[i - 1]._opcode == LDDW) { // LDDW contains two insns
          is_nop = false;
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
  min_pkt_sz = mem_t::_layout._pkt_sz;
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

void inst_static_state::insert_stack_state(inst_static_state& iss) {
  // insert stack_state
  for (auto it = iss.stack_state.begin(); it != iss.stack_state.end(); it++) {
    int off = it->first;
    auto it1 = stack_state.find(off);
    if (it1 == stack_state.end()) {
      stack_state[off] = it->second;
    } else {
      for (int i = 0; i < it->second.size(); i++) {
        bool flag = false;
        for (int j = 0; j < it1->second.size(); j++) {
          if (it->second[i] == it1->second[j]) {
            flag = true;
            break;
          }
        }
        if (! flag) {
          it1->second.push_back(it->second[i]);
        }
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
  min_pkt_sz = rhs.min_pkt_sz;
  stack_state = rhs.stack_state;
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
  out << "min pkt sz: " << x.min_pkt_sz << endl;
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
    } else if (type == PTR_TO_MAP_OR_NULL) {
      register_state rs;
      if (not_jmp) {
        rs.type = CONST_PTR_TO_MAP;
        // todo: val i.e.(map id = 2) is set for katran balancer_kern
        rs.val = 2;
        rs.val_flag = true;
        iss.set_reg_state(dst_reg, rs);
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

void type_const_inference_inst_JNEXC(inst_static_state& iss, inst& insn, bool not_jmp) {
  if (insn._opcode != JNEXC) return;
  inst insn_jeq = insn;
  insn_jeq._opcode = JEQXC;
  not_jmp = ! not_jmp;
  type_const_inference_inst_JEQXC(iss, insn_jeq, not_jmp);
}

void type_const_inference_inst_block_start(inst_static_state& iss, int cur_insn,
    int block_in_insn_id, inst& block_in_insn) {
  bool not_jmp = false;
  if ((block_in_insn_id + 1) == cur_insn) {// not jmp
    not_jmp = true;
  }
  type_const_inference_inst_JEQXC(iss, block_in_insn, not_jmp);
  type_const_inference_inst_JNEXC(iss, block_in_insn, not_jmp);
}

bool must_be_ptr_with_type(vector<register_state>& rs, int pointer_type) {
  if (rs.size() == 0) return false;
  for (int i = 0; i < rs.size(); i++) {
    if (rs[i].type != pointer_type) return false;
  }
  return true;
}

bool must_be_ptr(vector<register_state>& rs) {
  if (rs.size() == 0) return false;
  for (int i = 0; i < rs.size(); i++) {
    if (! is_ptr(rs[i].type)) return false;
  }
  return true;
}

// SCALAR_VALUE with val flag
bool must_be_concrete_vals(vector<register_state>& rs) {
  if (rs.size() == 0) return false;
  for (int i = 0; i < rs.size(); i++) {
    // Actually no need to check SCALAR_VALUE,
    // since only SCALAR_VALUE's val_flag can be true
    if (rs[i].type != SCALAR_VALUE) return false;
    if (! rs[i].val_flag) return false;
  }
  return true;
}

void type_const_inference_inst_ADD64XY(inst_static_state& iss, inst& insn) {
  if (insn._opcode != ADD64XY) return;
  int dst_reg = insn._dst_reg;
  int src_reg = insn._src_reg;
  // dst_reg or src_reg is pointer
  bool dst_reg_ptr = must_be_ptr(iss.reg_state[dst_reg]);
  bool src_reg_ptr = must_be_ptr(iss.reg_state[src_reg]);
  bool dst_reg_concrete = must_be_concrete_vals(iss.reg_state[dst_reg]);
  bool src_reg_concrete = must_be_concrete_vals(iss.reg_state[src_reg]);
  vector<register_state> rss = {};

  if (dst_reg_concrete && src_reg_concrete) {
    unordered_set<int64_t> vals = {};
    for (int i = 0; i < iss.reg_state[dst_reg].size(); i++) {
      for (int j = 0; j < iss.reg_state[src_reg].size(); j++) {
        int64_t v = iss.reg_state[dst_reg][i].val + iss.reg_state[src_reg][j].val;
        vals.insert(v);
      }
    }
    for (auto v : vals) {
      register_state rs;
      rs.type = SCALAR_VALUE;
      rs.val = v;
      rs.val_flag = true;
      rss.push_back(rs);
    }
  } else if ((dst_reg_ptr && src_reg_concrete) || (dst_reg_concrete && src_reg_ptr)) {
    int ptr_reg = dst_reg, val_reg = src_reg;
    if (dst_reg_concrete && src_reg_ptr) {
      ptr_reg = src_reg;
      val_reg = dst_reg;
    }
    for (int i = 0; i < iss.reg_state[ptr_reg].size(); i++) {
      for (int j = 0; j < iss.reg_state[val_reg].size(); j++) {
        int off = iss.reg_state[ptr_reg][i].off + iss.reg_state[val_reg][j].val;
        register_state rs;
        rs = iss.reg_state[ptr_reg][i];
        rs.off = off;
        rss.push_back(rs);
      }
    }
  } else {
    register_state rs;
    rs.type = SCALAR_VALUE;
    rss.push_back(rs);
  }

  iss.reg_state[dst_reg].clear();
  for (int i = 0; i < rss.size(); i++) {
    iss.reg_state[dst_reg].push_back(rss[i]);
  }
}

// update pointer stored on stack
void type_const_inference_inst_STXDW(inst_static_state& iss, inst& insn) {
  if (insn._opcode != STXDW) return;
  int dst_reg = insn._dst_reg;
  int src_reg = insn._src_reg;
  int off = insn._off;
  // check whether src_reg is a pointer or CONST_PTR_TO_MAP
  bool src_reg_ptr = must_be_ptr(iss.reg_state[src_reg]);
  src_reg_ptr = src_reg_ptr || must_be_ptr_with_type(iss.reg_state[src_reg], CONST_PTR_TO_MAP);
  // check whether dst_reg is a stack pointer
  bool dst_reg_stack_ptr = must_be_ptr_with_type(iss.reg_state[dst_reg], PTR_TO_STACK);
  if (src_reg_ptr && dst_reg_stack_ptr) { // store a pointer (src_reg) on stack (dst_reg)
    // all offsets of stack (dst_reg)
    unordered_set<int> stack_offs = {};
    for (int i = 0; i < iss.reg_state[dst_reg].size(); i++) {
      stack_offs.insert(iss.reg_state[dst_reg][i].off + off);
    }

    for (int i = 0; i < iss.reg_state[src_reg].size(); i++) {
      register_state rs;
      rs = iss.reg_state[src_reg][i];
      // if (rs.type == CONST_PTR_TO_MAP) {
      //   cout << "type_const_inference_inst_STXDW: adding CONST_PTR_TO_MAP" << endl;
      //   cout << rs << endl;
      // }

      for (auto o : stack_offs) {
        auto it = iss.stack_state.find(o);
        if (it == iss.stack_state.end()) {
          iss.stack_state[o] = {rs};
        } else {
          bool found = false;
          for (int i = 0; i < it->second.size(); i++) {
            if (it->second[i] == rs) {
              found = true;
              break;
            }
          }
          if (! found) it->second.push_back(rs);
        }
      }
    }

  }
}

void  type_const_inference_inst_ST(inst_static_state& iss, inst& insn) {
  // erase stack pointer off if overwritten
  unordered_map<int, unordered_set<int>> mem_write_offs;
  get_mem_write_offs(mem_write_offs, iss.reg_state, insn);
  auto it = mem_write_offs.find(PTR_TO_STACK);
  if (it != mem_write_offs.end()) {
    unordered_set<int>& offs_w = it->second;
    for (auto it1 = offs_w.begin(); it1 != offs_w.end(); it1++) {
      int off = *it1;
      int off_ptr = off - (off % 8); // 8 bytes is the pointer size
      if (iss.stack_state.find(off_ptr) != iss.stack_state.end()) {
        iss.stack_state.erase(off_ptr);
      }
    }
  }

  // update pointer stored on stack
  type_const_inference_inst_STXDW(iss, insn);
}

// read pointer from stack
void type_const_inference_inst_LDXDW(inst_static_state& iss, inst& insn) {
  if (insn._opcode != LDXDW) return;
  int dst_reg = insn._dst_reg;
  int src_reg = insn._src_reg;
  // set the default value
  iss.set_reg_state(dst_reg, SCALAR_VALUE);
  // check whether src_reg is a stack pointer
  bool src_reg_stack_ptr = must_be_ptr_with_type(iss.reg_state[src_reg], PTR_TO_STACK);
  if (src_reg_stack_ptr) {
    iss.reg_state[dst_reg].clear();
    for (int i = 0; i < iss.reg_state[src_reg].size(); i++) {
      int off = iss.reg_state[src_reg][i].off + insn._off;
      auto it = iss.stack_state.find(off);
      if (it != iss.stack_state.end()) {
        for (int j = 0; j < it->second.size(); j++) {
          iss.reg_state[dst_reg].push_back(it->second[j]);
        }
      }
    }
    if (iss.reg_state[dst_reg].size() == 0) {
      iss.set_reg_state(dst_reg, SCALAR_VALUE);
    }
  }
}

void type_const_inference_inst_BPF_FUNC_map_lookup_elem(inst_static_state& iss, inst& insn) {
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
    if (mem_t::map_type(rs.map_id) == MAP_TYPE_array_of_maps) {
      rs.type = PTR_TO_MAP_OR_NULL;
    }
    rs.off = 0;
    iss.reg_state[0].push_back(rs);
  }
}

void type_const_inference_inst_LSH64XC(inst_static_state& iss, inst& insn) {
  if (insn._opcode != LSH64XC) return;
  // if dst_reg's value can be inferred, update the value by lsh
  int dst_reg = insn._dst_reg;
  int imm = insn._imm;
  for (int i = 0; i < iss.reg_state[dst_reg].size(); i++) {
    int type = iss.reg_state[dst_reg][i].type;
    int val_flag = iss.reg_state[dst_reg][i].val_flag;
    if ((type == SCALAR_VALUE) && val_flag) {
      int64_t val = iss.reg_state[dst_reg][i].val;
      int64_t new_val = ((uint64_t)val) << imm;
      // update value
      iss.reg_state[dst_reg][i].val = new_val;
    }
  }
}

void type_const_inference_inst_LSH32XC(inst_static_state& iss, inst& insn) {
  if (insn._opcode != LSH32XC) return;
  // if dst_reg's value can be inferred, update the value by lsh
  int dst_reg = insn._dst_reg;
  int imm = insn._imm;
  for (int i = 0; i < iss.reg_state[dst_reg].size(); i++) {
    int type = iss.reg_state[dst_reg][i].type;
    int val_flag = iss.reg_state[dst_reg][i].val_flag;
    if ((type == SCALAR_VALUE) && val_flag) {
      int64_t val = iss.reg_state[dst_reg][i].val;
      uint64_t new_val = ((uint64_t)val) << imm;
      // update value
      iss.reg_state[dst_reg][i].val = (uint32_t)new_val;
    }
  }
}

// After executing the insn, update register type in inst_static_state
void type_const_inference_inst(inst_static_state& iss, inst& insn) {
  int opcode_type = insn.get_opcode_type();
  if (opcode_type == OP_ST) {
    type_const_inference_inst_ST(iss, insn);
    return;
  }

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
  // keep track of pointers and constant

  // deal with pkt pointers: pkt_start and pkt_end
  // check 1. src_reg's type is PTR_TO_CTX
  // 2. pkt_start: insn is: LDXW ri rj 0 (ri = *(u32*)(rj+pkt_s_off))
  // 3. pkt_end: insn is: LDXW ri rj 4 (ri = *(u32*)(rj+pkt_e_off))
  int pgm_input_type = mem_t::get_pgm_input_type();
  int pkt_s_off = 0, pkt_e_off = 4;
  if (pgm_input_type == PGM_INPUT_skb) {
    pkt_s_off = SKB_data_s_off;
    pkt_e_off = SKB_data_e_off;
  }
  if ((pgm_input_type == PGM_INPUT_pkt_ptrs) ||
      (pgm_input_type == PGM_INPUT_skb)) {
    if ((opcode == LDXW) && (iss.reg_state[src_reg].size() == 1)) {
      if (iss.reg_state[src_reg][0].type == PTR_TO_CTX) {
        if ((iss.reg_state[src_reg][0].off == 0) && (off == pkt_s_off)) {
          iss.set_reg_state(dst_reg, PTR_TO_PKT, 0);
          return; // return here in case the reg state is overwritten by the remaining code
        } else if ((iss.reg_state[src_reg][0].off == 0) && (off == pkt_e_off)) {
          iss.set_reg_state(dst_reg, PTR_TO_PACKET_END, 0);
          return;
        }
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
  } else if (opcode == ADD64XY) {
    // update concrete value or pointer offset
    type_const_inference_inst_ADD64XY(iss, insn);
  } else if (opcode == LSH64XC) {
    type_const_inference_inst_LSH64XC(iss, insn);
  } else if (opcode == LSH32XC) {
    type_const_inference_inst_LSH32XC(iss, insn);
  } else if (opcode == LDXDW) {
    // read pointer stored on stack
    type_const_inference_inst_LDXDW(iss, insn);
  } else if (opcode == CALL) {
    iss.set_reg_state(0, SCALAR_VALUE);
    if (imm == BPF_FUNC_map_lookup_elem) type_const_inference_inst_BPF_FUNC_map_lookup_elem(iss, insn);
    // set r1 - r5 unreadable, i.e., clear their states
    for (int r = 1; r <= 5; r++) {
      iss.reg_state[r] = {};
    }
  } else if (insn.is_ldmapid()) {
    register_state rs;
    rs.type = CONST_PTR_TO_MAP;
    rs.val = insn._imm;
    rs.val_flag = true;
    iss.set_reg_state(dst_reg, rs);
  } else if (insn.is_movdwxc()) {
    register_state rs;
    rs.type = SCALAR_VALUE;
    rs.val = insn._imm64;
    rs.val_flag = true;
    iss.set_reg_state(dst_reg, rs);
  } else if (insn.is_ldmapval()) {
    register_state rs;
    rs.type = PTR_TO_MAP_VALUE;
    rs.map_id = (uint32_t)insn._imm64;
    rs.off = (uint32_t)(insn._imm64 >> 32);
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
    // note: only set regsiters that are set in all incoming branches
    unordered_set<int> regs_can_be_set;
    for (int i = 0; i < NUM_REGS; i++) regs_can_be_set.insert(i);
    for (int j = 0; j < g.nodes_in[block].size(); j++) {
      int block_in = g.nodes_in[block][j];
      for (int r = 0; r < NUM_REGS; r++) {
        if (bss[block_in].reg_state[r].size() == 0) regs_can_be_set.erase(r);
      }
    }
    for (int j = 0; j < g.nodes_in[block].size(); j++) { // root block does not have incoming blocks
      int block_in = g.nodes_in[block][j];
      inst_static_state iss;
      iss = bss[block_in];
      // erase the regsiters whose flags_num != g.nodes_in[block].size()
      for (int r = 0; r < NUM_REGS; r++) {
        if (regs_can_be_set.find(r) == regs_can_be_set.end()) {
          iss.reg_state[r] = {};
        }
      }
      int block_in_insn = g.nodes[block_in]._end;
      type_const_inference_inst_block_start(iss, block_s, block_in_insn, program[block_in_insn]);
      // ss[block_s].insert_reg_state(bss[block_in]);
      ss[block_s].insert_reg_state(iss);
      ss[block_s].insert_stack_state(iss);
    }
    // process the block from the first block insn
    for (int j = block_s; j < block_e; j++) {
      ss[j + 1] = ss[j]; // copy the previous state
      type_const_inference_inst(ss[j + 1], program[j]); // update state according to the insn
    }
    // update the basic block post register states
    bss[block].reg_state = ss[block_e].reg_state;
    bss[block].stack_state = ss[block_e].stack_state;
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
    case STXW:
    case XADD32: write_sz = 4; break;
    case STDW:
    case STXDW:
    case XADD64: write_sz = 8; break;
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
      // assert(reg_state[1][i].type == CONST_PTR_TO_MAP);
      if (reg_state[1][i].type != CONST_PTR_TO_MAP) {
        return;
      }
      assert(reg_state[1][i].val_flag);
      int map_id = reg_state[1][i].val;
      int k_sz = mem_t::map_key_sz(map_id) / NUM_BYTE_BITS;
      int v_sz = mem_t::map_val_sz(map_id) / NUM_BYTE_BITS;
      regs_sz.push_back({2, k_sz}); // r2 points to the key stored on stack
      regs_sz.push_back({3, v_sz}); // r3 points to the value stored on stack
    }
  } else if (func_id == BPF_FUNC_fib_lookup) {
    // todo: assume only one state for r2 and r3
    assert(reg_state[2].size() > 0);
    assert(reg_state[3].size() > 0);
    assert(is_ptr(reg_state[2][0].type));
    assert(reg_state[3][0].val_flag);
    int sz = reg_state[3][0].val;
    regs_sz.push_back({2, sz});
  }
}

void get_mem_read_offs(unordered_map<int, unordered_set<int>>& mem_read_offs,
                       vector<vector<register_state>>& reg_state, inst& insn) {
  mem_read_offs.clear();
  vector<pair<int, int> > regs_sz; // regs and read sz
  switch (insn._opcode) {
    case LDXB: regs_sz.push_back({insn._src_reg, 1}); break;
    case LDXH: regs_sz.push_back({insn._src_reg, 2}); break;
    case XADD32:
    case LDXW: regs_sz.push_back({insn._src_reg, 4}); break;
    case XADD64:
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
  if ((insn._opcode == XADD32) || (insn._opcode == XADD64)) is_mem_read = true;
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

// ebpf program exits explicitly by EXIT or BPF_FUNC_tail_call
int get_block_exit_type(inst* program, int block_s, int block_e) {
  for (int i = block_e; i >= block_s; i--) {
    if (program[i]._opcode == EXIT) return PGM_EXIT_TYPE_default;
    else if ((program[i]._opcode == CALL) && (program[i]._imm == BPF_FUNC_tail_call)) {
      return PGM_EXIT_TYPE_tail_call;
    }
  }
  // todo: maybe need to add an error here
  return PGM_EXIT_TYPE_default;
}

void live_analysis_pgm_add_reg_output(inst_static_state& iss, int pgm_exit_type) {
  if (pgm_exit_type == PGM_EXIT_TYPE_tail_call) {
    iss.insert_live_reg(1);
    iss.insert_live_reg(2);
    iss.insert_live_reg(3);
  } else {
    iss.insert_live_reg(0);
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
      int pgm_exit_type = get_block_exit_type(program, block_s, block_e);
      live_analysis_pgm_add_reg_output(ss[block_e], pgm_exit_type);
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

// rl > rr, rl_s and rr_s are the register states of rl and rr
// check rl points to pkt and rr is PTR_TO_PACKET_END
// return value is the max(min_pkt_sz when rl > rr, min_pkt_sz_default)
unsigned int get_min_pkt_sz_by_gt(int min_pkt_sz_default,
                                  vector<register_state>& rl_s,
                                  vector<register_state>& rr_s) {
  if ((rl_s.size() <= 0) || (rr_s.size() <= 0)) {
    string err_msg = "[get_min_pkt_sz_by_gt] no state for register";
    throw (err_msg);
  }

  // check rl points to pkt
  for (int i = 0; i < rl_s.size(); i++) {
    if (rl_s[i].type != PTR_TO_PKT) return min_pkt_sz_default;
  }
  // check rr is PTR_TO_PACKET_END with 0 offset
  for (int i = 0; i < rl_s.size(); i++) {
    if (rr_s[i].type != PTR_TO_PACKET_END) return min_pkt_sz_default;
    if (rr_s[i].off != 0) return min_pkt_sz_default;
  }
  // update min_pkt_sz_case_true
  int min_pkt_sz = rl_s[0].off;
  for (int i = 1; i < rl_s.size(); i++) {
    min_pkt_sz = min(min_pkt_sz, rl_s[i].off);
  }
  min_pkt_sz = max(min_pkt_sz, min_pkt_sz_default);
  return min_pkt_sz;
}

// not_jmp_min_pkt_sz = min(min_pkt_sz_not_jmp, min_pkt_sz_default)
// jmp_min_pkt_sz = min(min_pkt_sz_jmp, min_pkt_sz_default)
void upd_min_pkt_sz_by_jmp_insn(unsigned int& not_jmp_min_pkt_sz,
                                unsigned int& jmp_min_pkt_sz,
                                unsigned int min_pkt_sz_default,
                                inst& insn,
                                vector<vector<register_state>> regs_s) {
  not_jmp_min_pkt_sz = min_pkt_sz_default;
  jmp_min_pkt_sz = min_pkt_sz_default;
  // check opcode is JMPXY
  int op_type = insn.get_opcode_type();
  if (op_type != OP_COND_JMP) return;
  int src_reg = insn._src_reg;
  int dst_reg = insn._dst_reg;
  switch (insn._opcode) {
    case JGTXY: not_jmp_min_pkt_sz = get_min_pkt_sz_by_gt(min_pkt_sz_default, regs_s[dst_reg], regs_s[src_reg]);
    default: return;
  }
}

// min_pkt_sz_inference for win program without branches
void min_pkt_sz_inference_win(vector<inst_static_state>& iss_win,
                              inst_static_state& iss) {
  unsigned int min_pkt_sz = iss.min_pkt_sz;
  for (int i = 0; i < iss_win.size(); i++) {
    iss_win[i].min_pkt_sz = min_pkt_sz;
  }
}

void min_pkt_sz_inference_pgm(prog_static_state& pss, inst* program, int len) {
  if (mem_t::get_pgm_input_type() != PGM_INPUT_pkt_ptrs) {
    for (int i = 0; i < pss.static_state.size(); i++) {
      pss.static_state[i].min_pkt_sz = mem_t::_layout._pkt_sz;
    }
    return;
  }

  for (int i = 0; i < pss.static_state.size(); i++) {
    pss.static_state[i].min_pkt_sz = 0;
  }
  assert(pss.dag.size() >= 1);
  vector<inst_static_state>& ss = pss.static_state;
  graph& g = pss.g;
  vector<unsigned int>& dag = pss.dag;

  // Since min_pkt_sz is from either input or jmp instruction, instructions in each basic block
  // have the same min_pkt_sz. We can calculate min_pkt_sz for each basic block and then update
  // min_pkt_sz for each instruction
  int n_block = dag.size();
  vector<unsigned int> block_min_pkt_sz(n_block);

  // store min_pkt_sz for block_e instructions
  vector<unsigned int> block_e_not_jmp(n_block);
  vector<unsigned int> block_e_jmp(n_block);

  // initialize the min_pkt_sz for the first block (root)
  block_min_pkt_sz[0] = 0;

  // process block_e instructions and update block's min_pkt_sz in order of dag
  for (int i = 0; i < dag.size(); i++) {
    unsigned int block = dag[i];
    // update block's min_pkt_sz by incoming blocks
    unsigned int min_pkt_sz_blocks_in = 0;
    for (int j = 0; j < g.nodes_in[block].size(); j++) { // root block does not have incoming blocks
      unsigned int block_in = g.nodes_in[block][j];
      unsigned int block_in_insn = g.nodes[block_in]._end;
      // if block_in_insn is not an insn that will change min_pkt_sz,
      // block_e_not_jmp[block_in_insn] == block_e_jmp[block_in_insn].
      // so we only need to care about for jmp/not jmp case, where to get min_pkt_sz
      if ( (program[block_in_insn].get_opcode_type() == OP_COND_JMP) &&
           (program[block_in_insn].get_jmp_dis() == 0) ) {
        unsigned int block_in_min_pkt_sz = min(block_e_not_jmp[block_in], block_e_jmp[block_in]);
        if (min_pkt_sz_blocks_in == 0) min_pkt_sz_blocks_in = block_in_min_pkt_sz;
        else min_pkt_sz_blocks_in = min(min_pkt_sz_blocks_in, block_in_min_pkt_sz);
      } else {
        // check whether block_in -> block is jmp or not jmp
        bool not_jmp = false;
        if ((block_in_insn + 1) == g.nodes[block]._start) {// not jmp
          not_jmp = true;
        }

        if (not_jmp) {
          if (min_pkt_sz_blocks_in == 0) min_pkt_sz_blocks_in = block_e_not_jmp[block_in];
          else min_pkt_sz_blocks_in = min(min_pkt_sz_blocks_in, block_e_not_jmp[block_in]);
        } else {
          if (min_pkt_sz_blocks_in == 0) min_pkt_sz_blocks_in = block_e_jmp[block_in];
          else min_pkt_sz_blocks_in = min(min_pkt_sz_blocks_in, block_e_jmp[block_in]);
        }
      }
    }

    block_min_pkt_sz[block] = min_pkt_sz_blocks_in;

    unsigned int block_e = g.nodes[block]._end;
    upd_min_pkt_sz_by_jmp_insn(block_e_not_jmp[block],
                               block_e_jmp[block],
                               block_min_pkt_sz[block],
                               program[block_e],
                               ss[block_e].reg_state);
  }
  // update min_pkt_sz for each insn by block_min_pkt_sz
  for (int i = 0; i < dag.size(); i++) {
    unsigned int block = dag[i];
    unsigned int min_pkt_sz = block_min_pkt_sz[block];
    for (int j = g.nodes[block]._start; j <= g.nodes[block]._end; j++) {
      ss[j].min_pkt_sz = min_pkt_sz;
    }
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
  min_pkt_sz_inference_pgm(pss, program, len);
}

// return -1 if there is no offs
int get_max_access_off(unordered_map<int, unordered_set<int>>& mem_offs, int ptr_type) {
  auto it = mem_offs.find(ptr_type);
  if (it == mem_offs.end()) return -1;
  unordered_set<int>& offs = it->second;
  int max_off = -1;
  for (auto off : offs) {
    if (max_off < off) max_off = off;
  }
  return max_off;
}

// check pkt memory access: out of bound
void safety_chk_insn_mem_access_pkt(inst& insn, inst_static_state& iss) {
  // check whether there are regs for memory access
  vector<int> mem_access_regs;
  insn.mem_access_regs(mem_access_regs);
  if (mem_access_regs.size() == 0) return;
  // check if registers have PTR_TO_PKT type pointer, its offset is a constant
  bool has_pkt_off = false;
  bool is_pkt_off_constant = true;
  for (int i = 0; i < mem_access_regs.size(); i++) {
    int reg = mem_access_regs[i];
    if (iss.reg_state[reg].size() > 0) {
      if (iss.reg_state[reg][0].type == PTR_TO_PKT) {
        has_pkt_off = true;
        // check whether pkt off is a constant
        if (iss.reg_state[reg].size() != 1) {
          is_pkt_off_constant = false;
          // if there is one case where pkt off is not a constant, break
          break;
        }
      }
    }
  }

  // not able to check symbolic offsets
  if ((! has_pkt_off) || (! is_pkt_off_constant)) return;

  unordered_map<int, unordered_set<int>> mem_write_offs, mem_read_offs;
  get_mem_write_offs(mem_write_offs, iss.reg_state, insn);
  get_mem_read_offs(mem_read_offs, iss.reg_state, insn);
  int pkt_max_off = max(get_max_access_off(mem_write_offs, PTR_TO_PKT),
                        get_max_access_off(mem_read_offs, PTR_TO_PKT));
  if (pkt_max_off == -1) return;

  // check whether pkt_max_off < min_pkt_sz
  if (pkt_max_off >= iss.min_pkt_sz) {
    string err_msg = "pkt (size: " + to_string(iss.min_pkt_sz) +
                     ") out of bound, accessing " + to_string(pkt_max_off);
    throw (err_msg);
  }
}

bool is_valid_xdp_access(int off, int size) {
  if ((off < 0) || (off >= CTX_SIZE_XDP_PROG))
    return false;
  if (off % size != 0)
    return false;
  if (size != sizeof(uint32_t))
    return false;

  return true;
}

// check ctx memory access
void safety_chk_insn_mem_access_ctx(inst& insn, inst_static_state& iss) {
  int op_type = insn.get_opcode_type();
  if ((op_type != OP_ST) && (op_type != OP_LD)) return;

  int prog_input_type = mem_t::get_pgm_input_type();
  // if program type is xdp(i.e., PGM_INPUT_pkt_ptrs)
  // three checks: https://elixir.bootlin.com/linux/v5.4/source/net/core/filter.c#L6858
  if (prog_input_type == PGM_INPUT_pkt_ptrs) {
    vector<int> mem_acc_regs;
    insn.mem_access_regs(mem_acc_regs);
    int size = insn.mem_access_width();
    int off = insn._off;
    for (int i = 0; i < mem_acc_regs.size(); i++) {
      int reg = mem_acc_regs[i];
      for (int j = 0; j < iss.reg_state[reg].size(); j++) {
        if (iss.reg_state[reg][j].type != PTR_TO_CTX) continue;
        int off_s = iss.reg_state[reg][j].off;
        if (is_valid_xdp_access(off_s + off, size)) continue;
        string err_msg = "r" + to_string(reg) +
                         " invalid ctx memory access, off: " + to_string(off) +
                         ", size: " + to_string(size);
        throw (err_msg);
      }
    }
  }
}

void safety_chk_insn_mem_access_stack_one_reg_state(int off, int size) {
  // out of bound check
  if ((off < 0) || (off > STACK_SIZE - size)) {
    string err_msg = "stack access out of bound: off:" + to_string(off) + " size:" + to_string(size);
    throw (err_msg);
  }
  // aligned check
  if (off % size != 0) {
    string err_msg = "stack access not aligned: off:" + to_string(off) + " size:" + to_string(size);
    throw (err_msg);
  }
  // todo: stack[off] readable check
}

void safety_chk_insn_mem_access_map_one_reg_state(int off, int size, int map_id) {
  int val_size = mem_t::map_val_sz(map_id) / NUM_BYTE_BITS;
  // out of map value bound check
  if ((off < 0) || (off > val_size - size)) {
    string err_msg = "map value (size:" + to_string(val_size) +
                     ") access out of bound: off:" + to_string(off) + " size:" + to_string(size);
    throw (err_msg);
  }
}

void safety_chk_insn_mem_access(inst& insn, inst_static_state& iss) {
  safety_chk_insn_mem_access_ctx(insn, iss);
  safety_chk_insn_mem_access_pkt(insn, iss);
  int op_type = insn.get_opcode_type();
  if ((op_type != OP_ST) && (op_type != OP_LD)) return;

  vector<int> mem_acc_regs;
  insn.mem_access_regs(mem_acc_regs);
  int size = insn.mem_access_width();
  int off = insn._off;
  for (int i = 0; i < mem_acc_regs.size(); i++) {
    int reg = mem_acc_regs[i];
    for (int j = 0; j < iss.reg_state[reg].size(); j++) {
      int reg_type = iss.reg_state[reg][j].type;
      if (! is_ptr(reg_type)) {
        string err_msg = "r" + to_string(reg) + " is not a pointer";
        throw (err_msg);
      }
      int reg_map_id = iss.reg_state[reg][j].map_id;
      int access_off = iss.reg_state[reg][j].off + off;
      if (reg_type == PTR_TO_STACK) {
        safety_chk_insn_mem_access_stack_one_reg_state(access_off, size);
      } else if (reg_type == PTR_TO_MAP_VALUE) {
        safety_chk_insn_mem_access_map_one_reg_state(access_off, size, reg_map_id);
      }
    }
  }
}

void safety_chk_insn(inst& insn, inst_static_state& iss) {
  const vector<vector<register_state>>& reg_state = iss.reg_state;
  int op_type = insn.get_opcode_type();
  if ((op_type == OP_NOP) ||
      (op_type == OP_UNCOND_JMP) || // TODO: jmp is not in the window
      (op_type == OP_COND_JMP)) {
    // todo: enable the following check if jmp safety check is added.
    // // `JEQXC r 0` is legal if r.type == PTR_TO_MAP_VALUE_OR_NULL
    // if ((type == PTR_TO_MAP_VALUE_OR_NULL) &&
    //     ((insn._opcode == JEQXC) || (insn._opcode == JNEXC)) &&
    //     (insn._imm == 0)) {
    //   continue;
    // }
    return;
  } else if (op_type == OP_OTHERS) { // ALU operations:
    // if pointers: only mov64xy, add64xc, add64xy, sub64xy are allowed
    // todo: safety check of sub64xy
    if ((insn._opcode == MOV64XY) ||
        (insn._opcode == ADD64XC) ||
        (insn._opcode == SUB64XY)) {
      return;
    } else if (insn._opcode == ADD64XY) {
      // add64xy: only one of src and dst regs can be pointers.
      // static analysis does not track pointers from ADD64XY
      vector<int> regs_r = {insn._dst_reg, insn._src_reg};
      int pointers_count = 0;
      for (int i = 0; i < regs_r.size(); i++) {
        int reg = regs_r[i];
        for (int j = 0; j < reg_state[reg].size(); j++) {
          if (reg_state[reg][j].type != SCALAR_VALUE) { // if pointers
            pointers_count++;
            break;
          }
        }
      }
      if (pointers_count == regs_r.size()) { // all registers are both pointers
        string err_msg = "illegal pointer operation of r" +
                         to_string(regs_r[0]) + " r" + to_string(regs_r[1]);
        throw (err_msg);
      }
      return;
    } else if (insn._opcode == DIV64XC) {
      if (insn._imm == 0) {
        string err_msg = "r" + to_string(insn._dst_reg) + " is divided by 0";
        throw (err_msg);
      }
      return;
    }

    vector<int> regs_r;
    insn.regs_to_read(regs_r);
    for (int i = 0; i < regs_r.size(); i++) {
      int reg = regs_r[i];
      for (int j = 0; j < reg_state[reg].size(); j++) {
        if (reg_state[reg][j].type == SCALAR_VALUE) { // if not pointers, continue
          continue;
        }
        string err_msg = "illegal pointer operation of r" + to_string(reg);
        throw (err_msg);
      }
    }
  } else if (op_type == OP_CALL) {
    return;
  } else if (op_type == OP_RET) {
    return;
  } else if ((op_type == OP_ST) || (op_type == OP_LD)) {// memory access safety check
    safety_chk_insn_mem_access(insn, iss);
  }
}

void static_safety_check_pgm(inst * program, int len) {
  prog_static_state pss;
  pss.static_state.resize(len + 1);
  pss.g.gen_graph(program, len);
  topo_sort_for_graph(pss.dag, pss.g);
  type_const_inference_pgm(pss, program, len);
  min_pkt_sz_inference_pgm(pss, program, len);
  for (int i = 0; i < len; i++) {
    safety_chk_insn(program[i], pss.static_state[i]);
  }
}

void static_safety_check_win(inst * win_prog, int win_start, int win_end, prog_static_state & pss_orig) {
  int win_len = win_end - win_start + 1;
  // 1. compute ss_win according to the ss_orig and the window program
  vector<inst_static_state> ss_win(win_len); // [win_start, win_end]
  // get the initial reg state from pss_orig `win_start`
  ss_win[0].reg_state = pss_orig.static_state[win_start].reg_state;
  ss_win[0].stack_state = pss_orig.static_state[win_start].stack_state;
  // todo: extended for window program with branches
  // update reg_state of win_program from insn 1 to len, reg_state: before executing insn
  for (int i = 0; i < win_len - 1; i++) {
    ss_win[i + 1].reg_state = ss_win[i].reg_state;
    ss_win[i + 1].stack_state = ss_win[i].stack_state;
    type_const_inference_inst(ss_win[i + 1], win_prog[i + win_start]);
  }
  min_pkt_sz_inference_win(ss_win, pss_orig.static_state[win_start]);

  for (int i = 0; i < win_len; i++) {
    safety_chk_insn(win_prog[i + win_start], ss_win[i]);
  }
}

// update the original program's pre-condition and post-condition
void set_up_smt_inout_orig(prog_static_state & pss, inst * program, int len, int win_start, int win_end) {
  vector<inst_static_state>& ss = pss.static_state;
  assert(ss.size() >= win_end);
  smt_input::reg_state = ss[win_start].reg_state;
  smt_input::stack_state = ss[win_start].stack_state;
  smt_output::post_prog_r = ss[win_end].live_var;
}

void compute_win_w(live_variables & win_w, vector<inst_static_state>& ss_win, inst * program, int win_start, int win_end) {
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

void set_up_smt_output_win(smt_output & sout, vector<inst_static_state>& ss_win,
                           vector<inst_static_state>& ss_orig,
                           inst * program, int win_start, int win_end) {
  live_variables win_w;
  compute_win_w(win_w, ss_win, program, win_start, win_end);
  // set output_var = win_w intersection post_r
  live_variables post_r = ss_orig[win_end].live_var;
  live_variables::intersection(sout.output_var, win_w, post_r);
}

void set_up_smt_inout_win(smt_input & sin, smt_output & sout,
                          prog_static_state & pss_orig, inst * program, int win_start, int win_end) {
  int win_len = win_end - win_start + 1;
  // 1. compute ss_win according to the ss_orig and the window program
  vector<inst_static_state> ss_win(win_len); // [win_start, win_end]
  // get the initial reg state from pss_orig `win_start`
  ss_win[0].reg_state = pss_orig.static_state[win_start].reg_state;
  ss_win[0].stack_state = pss_orig.static_state[win_start].stack_state;
  // todo: extended for window program with branches
  // update reg_state of win_program from insn 1 to len, reg_state: before executing insn
  for (int i = 0; i < win_len - 1; i++) {
    ss_win[i + 1].reg_state = ss_win[i].reg_state;
    ss_win[i + 1].stack_state = ss_win[i].stack_state;
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
  for (int i = 0; i < inputs.size(); i++) {
    inputs[i].init();
  }
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

void gen_random_input_for_win(vector<inout_t>& inputs, int n, inst_static_state & iss, inst & insn, int win_start, int win_end) {
  inputs.clear();
  inputs.resize(n);
  for (int i = 0; i < inputs.size(); i++) {
    inputs[i].init();
  }
  gen_random_input_for_common(inputs, true);

  // Generate random variables that have been written in precondition
  for (int i = 0; i < inputs.size(); i++) {
    // set random value for maps mem
    for (int map_id = 0; map_id < inputs[i].maps_mem.size(); map_id++) {
      for (int j = 0; j < inputs[i].maps_mem[map_id].size(); j++) {
        inputs[i].maps_mem[map_id][j] = random_int(0, 0xff); // [0, 0xff]
      }
    }
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
          // todo: assume one value
          assert(map_id >= 0);
          assert(map_id < mem_t::maps_number());
          unsigned int map_v_mem_off_start = mem_t::get_mem_off_by_idx_in_map(map_id, 0);
          int map_v_off = iss.reg_state[reg][sample].off;
          reg_v = stack_top + map_v_mem_off_start + map_v_off;
        }
      } else if (iss.reg_state[reg][sample].val_flag) { // deal with constant
        reg_v = iss.reg_state[reg][sample].val;
      } else {
        reg_v = random_uint64(min_u64, max_u64);
      }
      inputs[i].regs[reg] = reg_v;
    }

    // 3. Generte stack, use live_variable info and stack state
    // live_var is the live vars after executing this insn, what we want is before
    live_variables live_vars_before = iss.live_var;
    live_analysis_inst(live_vars_before, iss.reg_state, insn);
    auto it = live_vars_before.mem.find(PTR_TO_STACK);
    if (it != live_vars_before.mem.end()) {
      // set random values
      for (auto off : it->second) {
        inputs[i].stack_readble[off] = true;
        inputs[i].stack[off] = random_int(0, 0xff);
      }
      // set values from stack state
      for (auto& elem : iss.stack_state) {
        int stack_off_s = elem.first;
        assert(stack_off_s >= 0);
        int ptr_sz = 8; // 8 bytes
        assert(stack_off_s <= (STACK_SIZE - ptr_sz));
        // random choose a ptr
        assert(elem.second.size() >= 1);
        int idx = random_int(0, elem.second.size() - 1);
        int ptr_type = elem.second[idx].type;
        int ptr_off = elem.second[idx].off;
        // get ptr value according
        uint64_t ptr_val = 0;
        if (ptr_type == PTR_TO_STACK) {
          ptr_val = inputs[i].input_simu_r10 - STACK_SIZE + ptr_off;
        } else if (ptr_type == PTR_TO_CTX) {
          if (pgm_input_type == PGM_INPUT_pkt_ptrs) {
            ptr_val = inputs[i].input_simu_pkt_ptrs_s + ptr_off;
          } else {
            ptr_val = inputs[i].input_simu_pkt_s + ptr_off;
          }
        } else if (ptr_type == PTR_TO_PKT) {
          if ((pgm_input_type == PGM_INPUT_pkt_ptrs) || (pgm_input_type = PGM_INPUT_skb)) {
            ptr_val = inputs[i].input_simu_pkt_ptrs[0] + ptr_off;
          } else {
            cout << "ERROR: [gen_random_input_for_win] no pkt memory for this program intput type: "
                 << pgm_input_type << endl;
          }
        } else if (ptr_type == PTR_TO_MAP_VALUE) {
          int map_id = elem.second[idx].map_id;
          // todo: assume one value
          unsigned int map_v_mem_off_start = mem_t::get_mem_off_by_idx_in_map(map_id, 0);
          int map_v_off = ptr_off;
          assert(map_id >= 0);
          assert(map_id < mem_t::maps_number());
          ptr_val = stack_top + map_v_mem_off_start + map_v_off;
        } else if (ptr_type == CONST_PTR_TO_MAP) {
          assert(elem.second[idx].val_flag);
          int map_id = elem.second[idx].val;
          assert(map_id >= 0);
          assert(map_id < mem_t::maps_number());
          ptr_val = map_id;
        } else {
          cout << "ERROR: [gen_random_input_for_win] not support this ptr type: " << ptr_type << endl;
        }
        // write ptr value on stack
        for (int off_i = 0; off_i < ptr_sz; off_i++) {
          uint8_t v = ptr_val >> (off_i * NUM_BYTE_BITS);
          inputs[i].stack[stack_off_s + off_i] = v;
        }
      }
    }
  }

}
