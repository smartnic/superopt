#include "benchmark_ebpf.h"

using namespace std;

ebpf_inst ebpf_bm0[N] = {ebpf_inst(ebpf::MOV64XC, 0, 0x1),  /* mov64 r0, 0x1 */
                         ebpf_inst(ebpf::ADD64XY, 0, 0),  /* add64 r0, r0 */
                         ebpf_inst(ebpf::EXIT),  /* exit, return r0 */
                         ebpf_inst(),
                         ebpf_inst(),
                         ebpf_inst(),
                         ebpf_inst(),
                        };

ebpf_inst ebpf_bm_opti00[N] = {ebpf_inst(ebpf::MOV64XC, 0, 0x2),  /* mov64 r0, 0x2 */
                               ebpf_inst(ebpf::EXIT),  /* exit, return r0 */
                               ebpf_inst(),
                               ebpf_inst(),
                               ebpf_inst(),
                               ebpf_inst(),
                               ebpf_inst(),
                              };
