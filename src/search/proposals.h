#include "../../src/isa/prog.h"
#include "../../src/isa/inst.h"

using namespace std;

prog* mod_random_inst_operand(const prog &program);
prog* mod_random_inst(const prog &program);
prog* mod_random_k_cont_insts(const prog &program, unsigned int k);
prog* mod_random_cont_insts(const prog &program);
