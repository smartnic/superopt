#if ISA_TOY_ISA
#include "benchmark_toy_isa.h"
#elif ISA_EBPF && ISA_BPF_BENCH
#include "bpf_bench_linux.h"
#elif ISA_EBPF
#include "benchmark_ebpf.h"
#endif
