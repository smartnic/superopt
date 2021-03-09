#include "win_select.h"

using namespace std;
int num_multi_insns = 0;
int num_jmp = 0;
int num_ret = 0;

/* If the insn does not satisfy window constraints, return false.
*/
bool insn_satisfy_general_win_constraints(inst* insn, int num_insns) {
  assert(num_insns >= 1);

  /* opcode that contains multiple contiguous insns (proposal sampling not
     support multiple contiguous insns)
  */
  if (num_insns > 1) {
    num_multi_insns += num_insns;
    return false;
  }

  /* no jmp and return insns */
  int op_type = insn[0].get_opcode_type();
  if ((op_type == OP_UNCOND_JMP) ||
      (op_type == OP_COND_JMP)) {
    num_jmp++;
    return false;
  } else if (op_type == OP_RET) {
    num_ret++;
    return false;
  }

  return true;
}

/* Generate windows according to valid insns and general constraints
   such as in one basic block.
*/
void gen_wins_by_general_constraints(vector<pair<int, int>>& wins,
                                     prog_static_state& pss,
                                     vector<bool>& insns_valid) {
  wins.clear();
  const vector<unsigned int>& dag = pss.dag;
  const graph& g = pss.g;
  for (int i = 0; i < dag.size(); i++) {
    unsigned int block = dag[i];
    unsigned int block_s = g.nodes[block]._start;
    unsigned int block_e = g.nodes[block]._end;
    int win_s = block_s;
    bool set_win_s = false;
    for (int j = block_s; j <= block_e; j++) {
      // update win_s as the first valid insn from j to block_e
      if (! set_win_s) {
        for (int k = j; k <= block_e; k++) {
          if (! insns_valid[k]) continue;
          win_s = k;
          j = k;
          set_win_s = true;
          break;
        }
        // check whether there is win_s in this block,
        // if not, break in order to go to the next block
        if (! set_win_s) break;
      }

      if (insns_valid[j]) continue;
      if ((j - 1) < win_s) continue;
      // push (win_s, j-1) into wins
      wins.push_back(pair<int, int> {win_s, j - 1});
      set_win_s = false;
    }
  }
}

void reset_win_constraints_statistics() {
  reset_isa_win_constraints_statistics();
  num_multi_insns = 0;
  num_jmp = 0;
  num_ret = 0;
}

void print_win_constraints_statistics() {
  print_isa_win_constraints_statistics();
  cout << "# multi_insns: " << num_multi_insns << endl;
  cout << "# jmp: " << num_jmp << endl;
  cout << "# ret: " << num_ret << endl;
}

void gen_wins(vector<pair<int, int>>& wins, inst* pgm, int len, prog_static_state& pss) {
  reset_win_constraints_statistics();

  vector<bool> insns_valid(len);
  for (int i = 0; i < len; i++) insns_valid[i] = true;

  for (int i = 0; i < len; i++) {
    int num_insns = pgm[i].num_insns();
    bool satisfied = insn_satisfy_general_win_constraints(&pgm[i], num_insns);
    if (! satisfied) {
      for (int j = i; j < i + num_insns; j++) {
        insns_valid[j] = false;
      }
    }
    i += num_insns - 1;
  }

  for (int i = 0; i < len; i++) {
    if (! insns_valid[i]) continue;
    bool satisfied = insn_satisfy_isa_win_constraints(pgm[i], pss.static_state[i]);
    if (! satisfied) insns_valid[i] = false;
  }

  gen_wins_by_general_constraints(wins, pss, insns_valid);

  if (logger.is_print_level(LOGGER_DEBUG)) {
    print_win_constraints_statistics();
    cout << "insns not in windows" << ": ";
    for (int i = 0; i < len; i++) {
      if (! insns_valid[i]) cout << i << " ";
    }
    cout << endl;
  }
}
