#include "../../src/utils.h"
#include "../../src/isa/prog.h"
#include "../../src/isa/inst_header.h"

using namespace std;

// a modification in program window [win_start, win_end]
prog* mod_random_inst_operand(const prog &program,
                              int win_start = 0, int win_end = inst::max_prog_len - 1);
prog* mod_random_inst(const prog &program,
                      int win_start = 0, int win_end = inst::max_prog_len - 1);
prog* mod_random_k_cont_insts(const prog &program, unsigned int k,
                              int win_start = 0, int win_end = inst::max_prog_len - 1);
prog* mod_random_cont_insts(const prog &program,
                            int win_start = 0, int win_end = inst::max_prog_len - 1);
prog* mod_random_inst_opcode_width(const prog &program,
                                   int win_start = 0, int win_end = inst::max_prog_len - 1);
prog* mod_random_inst_as_nop(const prog &program,
                             int win_start = 0, int win_end = inst::max_prog_len - 1);
