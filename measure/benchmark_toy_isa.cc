#include "benchmark_toy_isa.h"

#include <iostream>
#include <fstream>
#include <stdint.h>

using namespace std;

ostream& operator<<(ostream& out, vector<int>& v) {
  for (size_t i = 0; i < v.size(); i++) {
    out << v[i] << " ";
  }
  return out;
}

ostream& operator<<(ostream& out, vector<vector<int> >& v) {
  for (size_t i = 0; i < v.size(); i++) {
    out << i << ": " << v[i] << endl;
  }
  return out;
}
// output = max(input+4, 15)
// perf_cost = 3 + 1 = 4
inst bm0[N] = {inst(MOVXC, 2, 4),  /* mov r2, 4  */
               inst(ADDXY, 0, 2),  /* add r0, r2 */
               inst(MOVXC, 3, 15),  /* mov r3, 15  */
               inst(JMPGT, 0, 3, 1),  /* if r0 <= r3: */
               inst(RETX, 3),      /* ret r3 */
               inst(RETX, 0),      /* else ret r0 */
               inst(),  /* control never reaches here */
              };
// f(x) = max(2*x, x+4)
// perf_cost = 3 + 1 = 4
inst bm1[N] = {inst(ADDXY, 1, 0),
               inst(MOVXC, 2, 4),
               inst(ADDXY, 1, 2), // r1 = r0+4
               inst(ADDXY, 0, 0), // r0 += r0
               inst(MAXX, 1, 0),  // r1 = max(r1, r0)
               inst(RETX, 1),
               inst(),
              };
// f(x) = 6*x
// perf_cost = 4 + 0 = 4
inst bm2[N] = {inst(MOVXC, 1, 0),
               inst(ADDXY, 1, 0), // r1 = 2*r0
               inst(ADDXY, 0, 1),
               inst(ADDXY, 0, 1),
               inst(ADDXY, 0, 1),
               inst(ADDXY, 0, 1),
               inst(ADDXY, 0, 1),
              };
inst bm_opti00[N] = {inst(MOVXC, 1, 4),
                     inst(ADDXY, 0, 1),
                     inst(MAXC, 0, 15),
                     inst(),
                     inst(),
                     inst(),
                     inst(),
                    };
inst bm_opti01[N] = {inst(MAXC, 1, 4),
                     inst(ADDXY, 0, 1),
                     inst(MAXC, 0, 15),
                     inst(),
                     inst(),
                     inst(),
                     inst(),
                    };
inst bm_opti02[N] = {inst(MAXC, 1, 4),
                     inst(MAXC, 0, 11),
                     inst(ADDXY, 0, 1),
                     inst(),
                     inst(),
                     inst(),
                     inst(),
                    };
inst bm_opti03[N] = {inst(MOVXC, 1, 4),
                     inst(MAXC, 0, 11),
                     inst(ADDXY, 0, 1),
                     inst(),
                     inst(),
                     inst(),
                     inst(),
                    };
inst bm_opti04[N] = {inst(MAXC, 0, 11),
                     inst(MOVXC, 1, 4),
                     inst(ADDXY, 0, 1),
                     inst(),
                     inst(),
                     inst(),
                     inst(),
                    };
inst bm_opti05[N] = {inst(MAXC, 0, 11),
                     inst(MAXC, 1, 4),
                     inst(ADDXY, 0, 1),
                     inst(),
                     inst(),
                     inst(),
                     inst(),
                    };
inst bm_opti10[N] = {inst(MOVXC, 1, 4),
                     inst(MAXX, 1, 0),
                     inst(ADDXY, 0, 1),
                     inst(),
                     inst(),
                     inst(),
                     inst(),
                    };
inst bm_opti11[N] = {inst(MAXC, 1, 4),
                     inst(MAXX, 1, 0),
                     inst(ADDXY, 0, 1),
                     inst(),
                     inst(),
                     inst(),
                     inst(),
                    };
inst bm_opti12[N] = {inst(ADDXY, 1, 0),
                     inst(MAXC, 0, 4),
                     inst(ADDXY, 0, 1),
                     inst(),
                     inst(),
                     inst(),
                     inst(),
                    };
inst bm_opti13[N] = {inst(ADDXY, 1, 0),
                     inst(MAXC, 1, 4),
                     inst(ADDXY, 0, 1),
                     inst(),
                     inst(),
                     inst(),
                     inst(),
                    };
inst bm_opti20[N] = {inst(ADDXY, 0, 0),
                     inst(ADDXY, 1, 0),
                     inst(ADDXY, 0, 1),
                     inst(ADDXY, 0, 1),
                     inst(),
                     inst(),
                     inst(),
                    };
inst bm_opti21[N] = {inst(ADDXY, 1, 0),
                     inst(ADDXY, 1, 1),
                     inst(ADDXY, 0, 1),
                     inst(ADDXY, 0, 0),
                     inst(),
                     inst(),
                     inst(),
                    };
inst bm_opti22[N] = {inst(ADDXY, 1, 0),
                     inst(ADDXY, 0, 1),
                     inst(ADDXY, 0, 1),
                     inst(ADDXY, 0, 0),
                     inst(),
                     inst(),
                     inst(),
                    };
inst bm_opti23[N] = {inst(ADDXY, 1, 0),
                     inst(ADDXY, 1, 0),
                     inst(ADDXY, 0, 1),
                     inst(ADDXY, 0, 0),
                     inst(),
                     inst(),
                     inst(),
                    };
inst bm_opti24[N] = {inst(ADDXY, 1, 0),
                     inst(ADDXY, 0, 0),
                     inst(ADDXY, 0, 1),
                     inst(ADDXY, 0, 0),
                     inst(),
                     inst(),
                     inst(),
                    };
inst bm_opti25[N] = {inst(ADDXY, 0, 0),
                     inst(ADDXY, 1, 0),
                     inst(ADDXY, 1, 0),
                     inst(ADDXY, 0, 1),
                     inst(),
                     inst(),
                     inst(),
                    };
inst bm_opti26[N] = {inst(ADDXY, 0, 0),
                     inst(ADDXY, 1, 0),
                     inst(ADDXY, 0, 0),
                     inst(ADDXY, 0, 1),
                     inst(),
                     inst(),
                     inst(),
                    };
inst bm_opti27[N] = {inst(ADDXY, 0, 0),
                     inst(ADDXY, 1, 0),
                     inst(ADDXY, 1, 1),
                     inst(ADDXY, 0, 1),
                     inst(),
                     inst(),
                     inst(),
                    };
struct bpf_insn {

  uint8_t opcode;
  uint8_t dst_reg: 4;
  uint8_t src_reg: 4;
  short off;
  int imm;
};

void read_maps() {

  FILE* fp;

  fp = fopen("/home/mikewong/Documents/smartnic/superopt/inputs/socket1.maps", "r");
  char * line = NULL;
  size_t len = 0;
  ssize_t read;

  if (fp == NULL) {

    std::cerr << "Error: BPF maps file could not be opened" << std::endl;
    exit(1);
  }
  while (read = getline (&line, &len, fp) != -1) {
    printf("%s\n", line);
  }
  fclose(fp);
}
void read_insns() {


  FILE *fp;
  fp = fopen("/home/mikewong/Documents/smartnic/superopt/inputs/socket1.ins", "r");
  if (fp == NULL ) {
    std::cerr << "Error: BPF insns file could not be opened" << std::endl;
    exit(1);
  }

  inst* insns;
  vector<inst> vec;

  bpf_insn input;
  // read file contents till end of file
  while (fread(&input, sizeof(bpf_insn), 1, fp)) {
    printf("op - %02x, src reg - %01x, dst reg - %01x, off - %04x, imm - %08x\t\n",
           input.opcode, input.dst_reg, input.src_reg, input.off, input.imm);
  }
  fclose(fp);

}
void init_benchmarks(inst** bm, vector<inst*> &bm_optis_orig, int bm_id) {

  read_maps();
  read_insns();
  inst::max_prog_len = N;
  switch (bm_id) {
    case 0:
      *bm = bm0;
      bm_optis_orig.push_back(bm_opti00);
      bm_optis_orig.push_back(bm_opti01);
      bm_optis_orig.push_back(bm_opti02);
      bm_optis_orig.push_back(bm_opti03);
      bm_optis_orig.push_back(bm_opti04);
      bm_optis_orig.push_back(bm_opti05);
      return;
    case 1:
      *bm = bm1;
      bm_optis_orig.push_back(bm_opti10);
      bm_optis_orig.push_back(bm_opti11);
      bm_optis_orig.push_back(bm_opti12);
      bm_optis_orig.push_back(bm_opti13);
      return;
    case 2:
      *bm = bm2;
      bm_optis_orig.push_back(bm_opti20);
      bm_optis_orig.push_back(bm_opti21);
      bm_optis_orig.push_back(bm_opti22);
      bm_optis_orig.push_back(bm_opti23);
      bm_optis_orig.push_back(bm_opti24);
      bm_optis_orig.push_back(bm_opti25);
      bm_optis_orig.push_back(bm_opti26);
      bm_optis_orig.push_back(bm_opti27);
      return;
    default:
      cout << "ERROR: toy-isa bm_id " + to_string(bm_id) + " is out of range {0, 1, 2}" << endl;
      return;
  }
}
