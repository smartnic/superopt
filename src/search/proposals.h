#include "../../src/utils.h"
#include "../../src/isa/prog.h"
#if ISA_TOY_ISA
#include "../../src/isa/toy-isa/inst.h"
#elif ISA_EBPF
#include "../../src/isa/ebpf/inst.h"
#endif

using namespace std;

prog* mod_random_inst_operand(const prog &program);
prog* mod_random_inst(const prog &program);
prog* mod_random_k_cont_insts(const prog &program, unsigned int k);
prog* mod_random_cont_insts(const prog &program);
