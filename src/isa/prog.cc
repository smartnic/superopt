#include "prog.h"

using namespace std;

// TODO: find canonical way to invoke one constructor from another
prog::prog(const prog& other) {
  inst_list = new inst[inst::max_prog_len];
  for (int i = 0; i < inst::max_prog_len; i++) {
    inst_list[i] = other.inst_list[i];
  }
  freq_count = other.freq_count;
  _error_cost = other._error_cost;
  _perf_cost = other._perf_cost;
}

prog::prog(inst* instructions) {
  inst_list = new inst[inst::max_prog_len];
  for (int i = 0; i < inst::max_prog_len; i++) {
    inst_list[i] = instructions[i];
  }
  freq_count = 0;
  _error_cost = -1;
  _perf_cost = -1;
}

void prog::reset_vals() {
  freq_count = 0;
  _error_cost = -1;
  _perf_cost = -1;
}

prog::prog() {
  freq_count = 0;
  _error_cost = -1;
  _perf_cost = -1;
}

prog::~prog() {
  delete []inst_list;
}

void prog::print() const {
  for (int i = 0; i < inst::max_prog_len; i++) {
    cout << i << ": ";
    inst_list[i].print();
  }
  cout << endl;
}

bool prog::operator==(const prog &x) const {
  for (int i = 0; i < inst::max_prog_len; i++) {
    if (! (inst_list[i] == x.inst_list[i])) return false;
  }
  return true;
}

void prog::set_vals(const prog &x) {
  freq_count = x.freq_count;
  _error_cost = x._error_cost;
  _perf_cost = x._perf_cost;
}

void prog::set_error_cost(double cost) {
  _error_cost = cost;
}

void prog::set_perf_cost(double cost) {
  _perf_cost = cost;
}

int prog::to_rel_bv(const prog &p) const {
  int bv = 0;
  for (int i = 0; i < inst::max_prog_len; i++) {
    if (inst_list[i] == p.inst_list[i]) {
      bv |= 1 << (inst::max_prog_len - 1 - i);
    }
  }
  return bv;
}

int prog::to_rel_bv(const vector<prog> &ps) const {
  int best = 0;
  int count = 0;
  for (int i = 0; i < ps.size(); i++) {
    int bv = to_rel_bv(ps[i]);
    int bv_count = pop_count_asm(bv);
    if (bv_count > count) {
      count = bv_count;
      best = bv;
    }
  }
  return best;
}

void prog::to_abs_bv(vector<op_t>& bv) const {
  for (int i = 0; i < inst::max_prog_len; i++) {
    inst_list[i].to_abs_bv(bv);
  }
}

bool prog::if_ret_exists(int start, int end) const {
  for (int i = start; i < end; i++) {
    if (inst_list[i].get_opcode_type() == OP_RET) {
      return true;
    }
  }
  return false;
}

// if reg 0 is NOT used but implicit RETX 0 instruction is needed, reg 0 cannot be used
// case 1: no RETs instruction(test6 insts41)
// case 2: has RETs instruction, but JMP makes implicit RETX 0 instruction needed(test6 insts42)
void prog::update_map_if_implicit_ret_r0_needed(unordered_map<int, int> &map_before_after) const {
  bool can_use_reg0 = true;
  // check whether there is RETs
  bool ret_exists = if_ret_exists(0, inst::max_prog_len);
  // step 1: check whether reg0 can be used
  if (! ret_exists) {
    // no RETs instruction
    can_use_reg0 = false;
  } else {
    // has RETs instruction, check jmp distance
    int start_index_chk_ret = 0;
    for (int i = 0; i < inst::max_prog_len; i++) {
      if ((inst_list[i].get_opcode_type() == OP_COND_JMP) &&
          ((i + 1 + inst_list[i].get_jmp_dis()) > start_index_chk_ret)) {
        start_index_chk_ret = i + 1 + inst_list[i].get_jmp_dis();
      }
    }
    ret_exists = if_ret_exists(start_index_chk_ret, inst::max_prog_len);
    if (! ret_exists) {
      can_use_reg0 = false;
    }
  }

  // step 2: if reg0 cannot be used, update the map_before_after
  if (! can_use_reg0) {
    for (auto it = map_before_after.begin(); it != map_before_after.end(); it++) {
      it->second++;
    }
  }
}

void prog::canonicalize() {
  unordered_map<int, int> map_before_after; // key: reg_id before, val: reg_id after
  vector<int> reg_list;
  // traverse all instructions and once there is a new reg_id(before), assign it a reg_id(after)
  // store reg_id(before) and reg_id(after) into map
  vector<int> available_reg_list = inst::get_isa_canonical_reg_list();
  int count = 0;
  for (int i = 0; i < inst::max_prog_len; i++) {
    reg_list = inst_list[i].get_canonical_reg_list();
    for (size_t j = 0; j < reg_list.size(); j++) {
      int cur_reg = reg_list[j];
      if (map_before_after.find(cur_reg) == map_before_after.end()) {
        assert(count < available_reg_list.size());
        map_before_after[cur_reg] = available_reg_list[count];
        count++;
      }
    }
  }
  if (map_before_after.size() == 0) return;

  // replace reg_ids(before) with reg_ids(after) for all instructions
  for (int i = 0; i < inst::max_prog_len; i++) {
    for (int j = 0; j < MAX_OP_LEN; j++) {
      if (inst_list[i].is_reg(j)) {
        auto it = map_before_after.find(inst_list[i].get_operand(j));
        if (it != map_before_after.end())
          inst_list[i].set_operand(j, it->second);
      }
    }
  }
}

int prog::num_real_instructions() const {
  int count = 0;
  for (int i = 0; i < inst::max_prog_len; i++) {
    count += inst_list[i].is_real_inst();
  }
  return count;
}

void prog::interpret(inout_t& output, prog_state &ps, const inout_t& input) const {
  return ::interpret(output, inst_list, inst::max_prog_len, ps, input);
}

size_t progHash::operator()(const prog &x) const {
  size_t hval = 0;
  for (int i = 0; i < inst::max_prog_len; i++) {
    hval = hval ^ (instHash()(x.inst_list[i]) << (i % 4));
  }
  return hval;
}
