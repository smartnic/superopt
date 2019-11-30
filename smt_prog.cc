#include <iostream>
#include <vector>
#include <string>
#include "smt_prog.h"
#include "inst.h"
#include "cfg.h"
#include "z3++.h"

using namespace z3;

context smt_prog_c;

#define CURSRC sv.get_cur_reg_var(SRCREG(in))
#define CURDST sv.get_cur_reg_var(DSTREG(in))
#define NEWDST sv.update_reg_var(DSTREG(in))
#define IMM2 IMM2VAL(in)

expr string_to_expr(string s) {
  if (s == "true") {
    return smt_prog_c.bool_val(true);
  } else if (s == "false") {
    return smt_prog_c.bool_val(false);
  }
  return smt_prog_c.int_const(s.c_str());
}

ostream& operator<<(ostream& out, vector<expr>& _expr_vec) {
  if (_expr_vec.size() > 0) {
    out << "\n";
  }
  for (size_t i = 0; i < _expr_vec.size(); i++) {
    out << i << ": " << _expr_vec[i] << "\n";
  }
  return out;
}

ostream& operator<<(ostream& out, vector<vector<expr> >& _expr_vec) {
  if (_expr_vec.size() > 0) {
    out << "\n";
  }
  for (size_t i = 0; i < _expr_vec.size(); i++) {
    out << "block" << i << ": " << _expr_vec[i] << "\n";
  }
  return out;
}

bool is_smt_valid(expr smt) {
  solver s(smt_prog_c);
  s.add(!smt);
  switch (s.check()) {
    case unsat: return true;
    case sat: return false;
    case unknown: return false;
  }
  return false;
}

/* class smt_var start */
smt_var::smt_var(unsigned int prog_id, unsigned int node_id) {
  _name = std::to_string(prog_id) + "_" + std::to_string(node_id);
  std::memset(reg_cur_id, 0, NUM_REGS * sizeof(unsigned int));
  std::string name_prefix = "r_" + _name + "_";
  for (size_t i = 0; i < NUM_REGS; i++) {
    std::string name = name_prefix + std::to_string(i) + "_0";
    reg_var.push_back(string_to_expr(name));
  }
}

smt_var::~smt_var() {
}

expr smt_var::update_reg_var(unsigned int reg_id) {
  reg_cur_id[reg_id]++;
  std::string name = "r_" + _name + "_" + std::to_string(reg_id) \
                     + "_" + std::to_string(reg_cur_id[reg_id]);
  reg_var[reg_id] = string_to_expr(name);
  return get_cur_reg_var(reg_id);
}

expr smt_var::get_cur_reg_var(unsigned int reg_id) {
  return reg_var[reg_id];
}

expr smt_var::get_init_reg_var(unsigned int reg_id) {
  std::string name = "r_" + _name + "_" + std::to_string(reg_id) + "_0";
  return string_to_expr(name);
}
/* class smt_var end */

/* class smt_prog start */
smt_prog::smt_prog() {}

smt_prog::~smt_prog() {}

// assume Block has no branch and is an ordered sequence of instructions
void smt_prog::smt_block(expr& smt_b, inst* program, int length, smt_var& sv) {
  inst* inst_lst = program;
  expr p = string_to_expr("true");
  for (size_t i = 0; i < length; i++) {
    if (get_inst_type(inst_lst[i]) != CFG_OTHERS) continue;
    p = p and smt_inst(sv, &inst_lst[i]);
  }
  smt_b = p.simplify();
}

expr smt_prog::smt_inst(smt_var& sv, inst* in) {
  switch (in->_opcode) {
    case ADDXY: return (CURDST + CURSRC == NEWDST);
    case MOVXC: return (IMM2 == NEWDST);
    case MAXC: {
      expr curDst = CURDST;
      expr newDst = NEWDST;
      expr cond1 = (curDst > IMM2) and (newDst == curDst);
      expr cond2 = (curDst <= IMM2) and (newDst == IMM2);
      return (cond1 or cond2);
    }
    case MAXX: {
      expr curDst = CURDST;
      expr curSrc = CURSRC;
      expr newDst = NEWDST;
      expr cond1 = (curDst > curSrc) and (newDst == curDst);
      expr cond2 = (curDst <= curSrc) and (newDst == curSrc);
      return (cond1 or cond2);
    }
    default: return string_to_expr("false");
  }
}

// init variables in class
void smt_prog::init() {
  post_reg_val.clear();
  path_con.clear();
  reg_iv.clear();
  bl.clear();
  post.clear();

  size_t block_num = g.nodes.size();

  post_reg_val.resize(block_num);
  for (size_t i = 0; i < block_num; i++) {
    post_reg_val[i].resize(NUM_REGS, string_to_expr("true"));
  }

  path_con.resize(block_num);
  for (size_t i = 0; i < block_num; i++) {
    // Because of the corresponding relationship between path_con and g.nodes_in,
    // the size of path_con[i] is equal to that of g.nodes_in[i];
    path_con[i].resize(g.nodes_in[i].size(), string_to_expr("true"));
  }

  reg_iv.resize(block_num);
  for (size_t i = 0; i < block_num; i++) {
    reg_iv[i].resize(g.nodes_in[i].size(), string_to_expr("true"));
  }
  bl.resize(block_num, string_to_expr("true"));
  post.resize(block_num);
  for (size_t i = 0; i < block_num; i++) {
    if (g.nodes_out[i].size() > 0) {
      post[i].resize(g.nodes_out[i].size(), string_to_expr("true"));
    } else {
      post[i].resize(1, string_to_expr("true"));
    }
  }
}

// topological sorting by DFS
void smt_prog::topo_sort_dfs(size_t cur_bid, vector<unsigned int>& blocks, vector<bool>& finished) {
  if (finished[cur_bid]) {
    return;
  }
  for (size_t i = 0; i < g.nodes_out[cur_bid].size(); i++) {
    topo_sort_dfs(g.nodes_out[cur_bid][i], blocks, finished);
  }
  finished[cur_bid] = true;
  blocks.push_back(cur_bid);
}

void smt_prog::gen_block_prog_logic(expr& e, smt_var& sv, size_t cur_bid, inst* inst_lst) {
  inst* start = &inst_lst[g.nodes[cur_bid]._start];
  int length = g.nodes[cur_bid]._end - g.nodes[cur_bid]._start + 1;
  e = string_to_expr("true");
  smt_block(e, start, length, sv);
  bl[cur_bid] = e; // store
}

void smt_prog::store_post_reg_val(smt_var& sv, size_t cur_bid) {
  for (size_t i = 0; i < NUM_REGS; i++) {
    post_reg_val[cur_bid][i] = sv.get_cur_reg_var(i);
  }
}

void smt_prog::smt_jmp_inst(smt_var& sv, vector<expr>& c_inst_end, inst& inst_end) {
  inst* in = &inst_end;
  // e is formula for Jmp
  expr e = string_to_expr("true");
  switch (inst_end._opcode) {
    case JMPEQ: e = (CURDST == CURSRC); break;
    case JMPGT: e = (CURDST > CURSRC); break;
    case JMPGE: e = (CURDST >= CURSRC); break;
    case JMPLT: e = (CURDST < CURSRC); break;
    case JMPLE: e = (CURDST <= CURSRC); break;
  }
  // keep order: insert no jmp first
  c_inst_end.push_back(!e); // no jmp
  c_inst_end.push_back(e);  // jmp
}

// update path condition "p_con" generated by cur_bid into the path_con[next_bid]
void smt_prog::add_path_cond(expr p_con, size_t cur_bid, size_t next_bid) {
  for (size_t j = 0; j < path_con[next_bid].size(); j++) {
    if (g.nodes_in[next_bid][j] == cur_bid) {
      path_con[next_bid][j] = p_con;
    }
  }
}

// generate pre path condition formula with ALL incoming edges for basic block
void smt_prog::gen_block_c_in(expr& c_in, size_t cur_bid) {
  if (path_con[cur_bid].size() > 0) { // calculate c_in by parents, that is, path_con[cur_bid]
    c_in = path_con[cur_bid][0];
    for (size_t i = 1; i < path_con[cur_bid].size(); i++) {
      c_in = c_in || path_con[cur_bid][i];
    }
  }
}

// steps: 1. calculate c_in;
// 2. calculate post path condition "c" of current basic block cur_bid;
// 3. use c to update path_con[next_bid]
// three cases: 1. no next blocks 2. one next block 3. two next blocks
void smt_prog::gen_post_path_con(smt_var& sv, size_t cur_bid, inst& inst_end) {
  // case 1: no next blocks
  if (g.nodes_out[cur_bid].size() == 0) {
    return;
  }
  // step 1. calculate c_in;
  // When cur_bid is processed, path_con[cur_bid] already has been
  // updated with the correct value because of topo sort
  // if current block(i.e., block 0) has no incoming edges, set c_in = true.
  expr c_in = string_to_expr("true");
  gen_block_c_in(c_in, cur_bid);
  // case 2: one next block
  // In this case, the post path condition is same as the c_in
  if (g.nodes_out[cur_bid].size() == 1) {
    unsigned int next_bid = g.nodes_out[cur_bid][0];
    // update path condition to the path_con[next_bid]
    add_path_cond(c_in.simplify(), cur_bid, next_bid);
    post[cur_bid][0] = c_in; //store
    return;
  }
  // case 3: two next blocks
  // If keep order: c_inst_end[0]: no jmp path condition; c_inst_end[1]: jmp path condition,
  // then c_inst_end[i] -> g.nodes_out[cur_bid][i];
  // Why: according to the process of jmp path condition in function gen_all_edges_graph in cfg.cc
  // function smt_jmp_inst keep this order
  // case 3 step 2
  vector<expr> c_inst_end;
  smt_jmp_inst(sv, c_inst_end, inst_end);
  // case 3 step 3
  // push the c_inst_end[0] and c_inst_end[1] into next_bids' path_con
  // no jmp
  unsigned int next_bid = g.nodes_out[cur_bid][0];
  expr c_next_bid = (c_in && c_inst_end[0]).simplify();
  add_path_cond(c_next_bid, cur_bid, next_bid);
  post[cur_bid][0] = c_next_bid; // store
  // jmp
  next_bid = g.nodes_out[cur_bid][1];
  c_next_bid = (c_in && c_inst_end[1]).simplify();
  add_path_cond(c_next_bid, cur_bid, next_bid);
  post[cur_bid][1] = c_next_bid; // store
}

void smt_prog::get_init_val(expr& f_iv, smt_var& sv, size_t in_bid) {
  expr e = (sv.get_init_reg_var(0) == post_reg_val[in_bid][0]);
  for (size_t i = 1; i < NUM_REGS; i++) {
    e = e && (sv.get_init_reg_var(i) == post_reg_val[in_bid][i]);
  }
  f_iv = e;
}

// TODO: needed to be generalized
// for each return value v, smt: v == output[prog_id]
expr smt_prog::smt_end_block_inst(size_t cur_bid, inst* inst_end, unsigned int prog_id) {
  switch (inst_end->_opcode) {
    case RETX:
      return (string_to_expr("output" + to_string(prog_id)) == post_reg_val[cur_bid][DSTREG(inst_end)]);
    case RETC:
      return (string_to_expr("output" + to_string(prog_id)) == IMM1VAL(inst_end));
    default: // if no RET, return r0
      return (string_to_expr("output" + to_string(prog_id)) == post_reg_val[cur_bid][0]);
  }
}

// Set f_p_output to capture the output of the program (from return instructions/default register)
// in the variable output[prog_id]
void smt_prog::process_output(expr& f_p_output, inst* inst_lst, unsigned int prog_id) {
  expr e = string_to_expr("true");
  // search all basic blocks for the basic blocks without outgoing edges
  for (size_t i = 0; i < g.nodes.size(); i++) {
    if (g.nodes_out[i].size() != 0) continue;
    // process END instruction
    expr c_in = string_to_expr("true");
    gen_block_c_in(c_in, i);
    expr e1 = smt_end_block_inst(i, &inst_lst[g.nodes[i]._end], prog_id);
    expr e2 = implies(c_in.simplify(), e1);
    e = e && e2;
    post[i][0] = e2; // store
  }
  f_p_output = e;
}

expr smt_prog::gen_smt(unsigned int prog_id, inst* inst_lst, int length) {
  try {
    // generate a cfg
    // illegal input would be detected: 1. program with loop
    // 2. program that goes to the invalid instruction
    g.gen_graph(inst_lst, length);
  } catch (const string err_msg) {
    throw (err_msg);
  }
  // init class variables
  init();

  // blocks stores the block IDs in order after topological sorting
  vector<unsigned int> blocks;
  vector<bool> finished(g.nodes.size(), false);
  // cfg here is without loop, loop detect: function dfs in class graph
  topo_sort_dfs(0, blocks, finished);
  std::reverse(blocks.begin(), blocks.end());

  // basic block FOL formula; f_block[b] = implies(path_con[b], f_iv[b] && fpl[b])
  vector<expr> f_block;
  f_block.resize(g.nodes.size(), string_to_expr("true"));
  // process each basic block in order
  for (size_t i = 0; i < blocks.size(); i++) {
    unsigned int b = blocks[i];
    smt_var sv(prog_id, b);
    // generate f_bl: the block program logic
    expr f_bl = string_to_expr("true");
    gen_block_prog_logic(f_bl, sv, b, inst_lst);
    if (b == 0) {
      // basic block 0 does not have pre path condition
      // and its f_iv is the whole program's pre condition which is stored in variable pre of class validator
      f_block[0] = f_bl;
    } else {
      for (size_t j = 0; j < g.nodes_in[b].size(); j++) {
        // generate f_iv: the logic that the initial values are fed by the last basic block
        expr f_iv = string_to_expr("true");
        get_init_val(f_iv, sv, g.nodes_in[b][j]);
        reg_iv[b][j] = f_iv; // store
        f_block[b] = f_block[b] && implies(path_con[b][j], f_iv && f_bl);
      }
    }
    // store post iv for current basic block b
    store_post_reg_val(sv, b);
    // update post path condtions "path_con" created by current basic block b
    gen_post_path_con(sv, b, inst_lst[g.nodes[b]._end]);
  }

  // program FOL formula; f_prog = f_block[0] && ... && f_block[n]
  expr f_prog = f_block[0];
  for (size_t i = 1; i < f_block.size(); i++) {
    f_prog = f_prog && f_block[i];
  }
  // program output FOL formula; rename all output register values to the same name
  expr f_p_output = string_to_expr("true");
  process_output(f_p_output, inst_lst, prog_id);
  pl = (f_prog && f_p_output).simplify();
  return pl;
}
/* class smt_prog end */
