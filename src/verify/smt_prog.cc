#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include "smt_prog.h"

using namespace z3;

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
  solver s(smt_c);
  s.add(!smt);
  switch (s.check()) {
    case unsat: return true;
    case sat: return false;
    case unknown: return false;
  }
  return false;
}

/* class smt_prog start */
smt_prog::smt_prog() {}

smt_prog::~smt_prog() {}

// assume Block has no branch and is an ordered sequence of instructions
void smt_prog::smt_block(expr& smt_b, inst* program, int start, int end,
                         smt_var& sv, smt_mem_layout& m_layout) {
  expr p = string_to_expr("true");
  for (size_t i = start; i <= end; i++) {
    int op_type = program[i].get_opcode_type();
    if ((op_type != OP_OTHERS) && (op_type != OP_LD) && (op_type != OP_ST)) continue;
    p = p and program[i].smt_inst(sv, m_layout);
  }
  smt_b = p.simplify();
}

// init variables in class
void smt_prog::init(unsigned int num_regs) {
  post_reg_val.clear();
  path_con.clear();
  reg_iv.clear();
  bl.clear();
  post.clear();
  post_sv.clear();

  size_t block_num = g.nodes.size();

  // Assume the parent of the root basic block is -1
  g.nodes_in[0].push_back(-1);

  post_reg_val.resize(block_num);
  for (size_t i = 0; i < block_num; i++) {
    post_reg_val[i].resize(num_regs, string_to_expr("true"));
  }

  post_sv.resize(block_num);

  path_con.resize(block_num);
  for (size_t i = 0; i < block_num; i++) {
    // Because of the corresponding relationship between path_con and g.nodes_in,
    // the size of path_con[i] is equal to that of g.nodes_in[i];
    path_con[i].resize(g.nodes_in[i].size());
  }
  // Assume the path condition from the basic block -1 to the root basic block 0 is 'true'
  path_con[0][0].push_back(string_to_expr("true"));

  reg_iv.resize(block_num);
  for (size_t i = 0; i < block_num; i++) {
    reg_iv[i].resize(g.nodes_in[i].size(), string_to_expr("true"));
  }
  bl.resize(block_num, string_to_expr("true"));
  post.resize(block_num, string_to_expr("true"));
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

// may need to modify
void smt_prog::gen_block_prog_logic(expr& e, smt_var& sv, smt_mem_layout& m_layout,
                                    size_t cur_bid, inst* inst_lst) {
  e = string_to_expr("true");
  smt_block(e, inst_lst, g.nodes[cur_bid]._start, g.nodes[cur_bid]._end, sv, m_layout);
  bl[cur_bid] = e; // store
}

void smt_prog::store_post_reg_val(smt_var& sv, size_t cur_bid, unsigned int num_regs) {
  for (size_t i = 0; i < num_regs; i++) {
    post_reg_val[cur_bid][i] = sv.get_cur_reg_var(i);
  }
}

// "p_con" is generated by the current basic block
// 1. extend the path_con[cur_bid] with "p_con", and add the extended path condtion into path_con[next_bid]
// 2. update all path_con[next_bid][i] where g.nodes_in[next_bid][i] == cur_bid
// Why all: jmp distance is 0, for the next basic block, there may be two same in basic block
void smt_prog::add_path_cond(expr p_con, size_t cur_bid, size_t next_bid) {
  // keep the one-to-one correspondence between g.nodes_in[next_bid][idx] and path_con[next_bid][idx]
  for (size_t i = 0; i < path_con[next_bid].size(); i++) {
    if (g.nodes_in[next_bid][i] != cur_bid) continue;
    // extend all path conditions in path_con[cur_bid] and add it into path_con[next_bid][idx]
    for (size_t j = 0; j < path_con[cur_bid].size(); j++) {
      for (size_t k = 0; k < path_con[cur_bid][j].size(); k++) {
        path_con[next_bid][i].push_back(path_con[cur_bid][j][k] && p_con);
      }
    }
    // cannot add a "break" or "return" here, since all
    // g.nodes_in[next_bid][i] == cur_bid cases shoud be processed
  }
}

// generate pre path condition formula with ALL incoming edges for basic block
void smt_prog::gen_block_c_in(expr& c_in, size_t cur_bid) {
  if (path_con[cur_bid].size() > 0) { // calculate c_in by parents, that is, path_con[cur_bid]
    c_in = string_to_expr("false"); // if c_in is "ture", the "||" makes the final c_in always "true"
    for (size_t i = 0; i < path_con[cur_bid].size(); i++) {
      for (size_t j = 0; j < path_con[cur_bid][i].size(); j++)
        c_in = c_in || path_con[cur_bid][i][j];
    }
  }
}

// steps:
// 1. calculate post path condition "c" of current basic block cur_bid;
// 2. use c to update path_con[next_bid]
// three cases: 1. no next blocks 2. one next block 3. two next blocks
void smt_prog::gen_post_path_con(smt_var& sv, size_t cur_bid, inst& inst_end) {
  // case 1: no next blocks
  if (g.nodes_out[cur_bid].size() == 0) {
    return;
  }

  // When cur_bid is processed, path_con[cur_bid] already has been
  // updated with the correct value because of topo sort
  // case 2: one next block
  // In this case, the post path condition is same as that of parents
  if (g.nodes_out[cur_bid].size() == 1) {
    unsigned int next_bid = g.nodes_out[cur_bid][0];
    // update path condition to the path_con[next_bid]
    add_path_cond(string_to_expr("true"), cur_bid, next_bid);
    return;
  }
  // case 3: two next blocks
  // Why: according to the process of jmp path condition in function gen_all_edges_graph in cfg.cc
  // case 3.1 that no jmp and jmp have the same next block id.
  if (g.nodes_out[cur_bid][0] == g.nodes_out[cur_bid][1]) {
    unsigned int next_bid = g.nodes_out[cur_bid][0];
    add_path_cond(string_to_expr("true"), cur_bid, next_bid);
    return;
  }
  // case3.2 no jmp and jmp have two different next blocks
  // If keep order: c_inst_end[0]: no jmp path condition; c_inst_end[1]: jmp path condition,
  // then c_inst_end[i] -> g.nodes_out[cur_bid][i];
  expr e = inst_end.smt_inst_jmp(sv); // !e: no jmp, e: jmp
  // keep order: insert no jmp first
  // no jmp
  unsigned int next_bid = g.nodes_out[cur_bid][0];
  add_path_cond((!e), cur_bid, next_bid);
  // jmp
  next_bid = g.nodes_out[cur_bid][1];
  add_path_cond(e, cur_bid, next_bid);
}

void smt_prog::get_init_val(expr& f_iv, smt_var& sv, size_t in_bid, unsigned int num_regs) {
  expr e = (sv.get_init_reg_var(0) == post_reg_val[in_bid][0]);
  for (size_t i = 1; i < num_regs; i++) {
    e = e && (sv.get_init_reg_var(i) == post_reg_val[in_bid][i]);
  }
  f_iv = e;
}

// TODO: needed to be generalized
// for each return value v, smt: v == output[prog_id]
expr smt_prog::smt_end_block_inst(size_t cur_bid, inst& inst_end, unsigned int prog_id) {
  switch (inst_end.inst_output_opcode_type()) {
    case RET_X:
      return (string_to_expr("output" + to_string(prog_id)) == post_reg_val[cur_bid][inst_end.inst_output()]);
    case RET_C:
      return (string_to_expr("output" + to_string(prog_id)) == inst_end.inst_output());
    default:
      return string_to_expr("false");
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
    expr e1 = smt_end_block_inst(i, inst_lst[g.nodes[i]._end], prog_id);
    expr e2 = implies(c_in.simplify(), e1);
    e = e && e2;
    post[i] = e2; // store
  }
  f_p_output = e;
}

expr smt_prog::gen_smt(unsigned int prog_id, inst* inst_lst, int length, smt_mem_layout& m_layout) {
  try {
    // generate a cfg
    // illegal input would be detected: 1. program with loop
    // 2. program that goes to the invalid instruction
    g.gen_graph(inst_lst, length);
  } catch (const string err_msg) {
    throw (err_msg);
  }
  // init class variables
  const unsigned int num_regs = NUM_REGS;
  init(num_regs);

  // blocks stores the block IDs in order after topological sorting
  vector<unsigned int> blocks;
  vector<bool> finished(g.nodes.size(), false);
  // cfg here is without loop, loop detect: function dfs in class graph
  topo_sort_dfs(0, blocks, finished);
  std::reverse(blocks.begin(), blocks.end());

  // basic block FOL formula;
  // f_block[b] = f_block[b](in_path_1) ^ f_block[b](in_path_2) ^ ...
  // f_block[b](in_path) = implies(path_con[b](in_path), f_iv[b](in_b) && f_bl[b](in_path))
  vector<expr> f_block;
  f_block.resize(g.nodes.size(), string_to_expr("true"));
  // process each basic block in order
  for (size_t i = 0; i < blocks.size(); i++) {
    unsigned int b = blocks[i];
    smt_var sv(prog_id, b, num_regs);
    sv.mem_var.init_addrs_map_v_next(m_layout);
    if (b == 0) {
      // generate f_bl: the block program logic
      expr f_bl = string_to_expr("true");
      gen_block_prog_logic(f_bl, sv, m_layout, b, inst_lst);
      // basic block 0 does not have pre path condition
      // and its f_iv is the whole program's pre condition which is stored in variable pre of class validator
      f_block[0] = f_bl;
      // store the memory write table of basic block 0 into post_sv
      post_sv[0].push_back(sv);
    } else {
      for (size_t j = 0; j < g.nodes_in[b].size(); j++) {
        // generate f_iv: the logic that the initial values are fed by the last basic block
        expr f_iv = string_to_expr("true");
        get_init_val(f_iv, sv, g.nodes_in[b][j], num_regs);
        reg_iv[b][j] = f_iv; // store
        for (size_t k = 0; k < path_con[b][j].size(); k++) {
          // clear() is to make sure when/after computing the fol of this basic block, the register versions
          // are the same for different initial path conditions of this block
          sv.clear();
          // update sv with the memory write table from the previous basic block
          sv.mem_var = post_sv[g.nodes_in[b][j]][k].mem_var;
          // generate f_bl: the block program logic
          expr f_bl = string_to_expr("true");
          gen_block_prog_logic(f_bl, sv, m_layout, b, inst_lst);
          f_block[b] = f_block[b] && implies(path_con[b][j][k], f_iv && f_bl);
          // store the current memory write table into post_sv
          post_sv[b].push_back(sv);
        }
      }
    }
    // store post iv for current basic block b
    store_post_reg_val(sv, b, num_regs);
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

void smt_prog::get_output_pc_mem(vector<expr>& pc, vector<smt_var>& mv) {
  pc.clear();
  mv.clear();
  for (size_t i = 0; i < g.nodes.size(); i++) {
    if (g.nodes_out[i].size() != 0) continue;

    // copy the path condtions and memory values into "pc" and "mv"
    for (size_t j = 0; j < path_con[i].size(); j++)
      for (size_t k = 0; k < path_con[i][j].size(); k++)
        pc.push_back(path_con[i][j][k]);
    for (size_t j = 0; j < post_sv[i].size(); j++)
      mv.push_back(post_sv[i][j]);
  }
}
/* class smt_prog end */
