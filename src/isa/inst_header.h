#if ISA_TOY_ISA
#include "../../src/isa/toy-isa/inst_var.h"
#include "../../src/isa/toy-isa/inst.h"
#elif ISA_EBPF
#include "../../src/isa/ebpf/inst_var.h"
#include "../../src/isa/ebpf/inst.h"
#elif ISA_NETRONOME
#include "../../src/isa/netronome/inst_var.h"
#include "../../src/isa/netronome/inst.h" 
#endif
