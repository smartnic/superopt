#include "benchmark_ebpf.h"
#include <map>
#include <algorithm> 
#include <cctype>
#include <locale>

using namespace std;

inst bm0[N0] = {inst(MOV64XC, 0, 0x1),  /* mov64 r0, 0x1 */
                inst(ADD64XY, 0, 0),  /* add64 r0, r0 */
                inst(EXIT),  /* exit, return r0 */
                inst(),
                inst(),
                inst(),
                inst(),
               };
inst bm_opti00[N0] = {inst(MOV64XC, 0, 0x2),  /* mov64 r0, 0x2 */
                      inst(),
                      inst(),
                      inst(),
                      inst(),
                      inst(),
                      inst(),
                     };
inst bm_opti01[N0] = {inst(MOV32XC, 0, 0x2),  /* mov32 r0, 0x2 */
                      inst(),
                      inst(),
                      inst(),
                      inst(),
                      inst(),
                      inst(),
                     };
inst bm_opti02[N0] = {inst(ADD64XC, 0, 0x2),  /* add64 r0, 0x2 */
                      inst(),
                      inst(),
                      inst(),
                      inst(),
                      inst(),
                      inst(),
                     };
inst bm_opti03[N0] = {inst(ADD32XC, 0, 0x2),  /* add32 r0, 0x2 */
                      inst(),
                      inst(),
                      inst(),
                      inst(),
                      inst(),
                      inst(),
                     };

// w0 = (w1 >> 16) | ((w1 << 16) & 0xff0000)
inst bm1[N1] = {inst(MOV32XY, 2, 1), // w2 = w1
                inst(RSH32XC, 2, 16), // w2 >>= 16
                inst(LSH32XC, 1, 16), // w1 <<= 16
                inst(AND32XC, 1, 0xff0000), // w1 &= 0xff0000
                inst(OR32XY, 1, 2), // w1 |= w2
                inst(MOV32XY, 0, 1), // w0 = w1
                inst(EXIT),
               };

inst bm_opti10[N1] = {inst(MOV32XY, 0, 1), // w0 = w1
                      inst(RSH32XC, 0, 16), // w0 >>= 16
                      inst(LSH32XC, 1, 16), // w1 <<= 16
                      inst(AND32XC, 1, 0xff0000), // w1 &= 0xff0000
                      inst(OR32XY, 0, 1), // w0 |= w1
                      inst(),
                      inst(),
                     };

inst bm2[N2] = {inst(STXB, 10, -2, 1),    // *(r10-2) = L8(input)
                inst(MOV64XC, 1, 0x01),   // *(r10-1) = 0x01
                inst(STXB, 10, -1, 1),
                inst(MOV64XC, 1, 0),      // r1 = map0
                inst(MOV64XY, 2, 10),     // r2 = r10 - 1
                inst(ADD64XC, 2, -1),
                inst(CALL, BPF_FUNC_map_lookup), // r0 = &map0[0x01]
                inst(JEQXC, 0, 0, 7),   // if r0 == 0, exit else map0[0x01] = L8(input)
                inst(MOV64XC, 1, 0),    // r1 = map0
                inst(MOV64XY, 2, 10),   // r2 = r10 - 1
                inst(ADD64XC, 2, -1),
                inst(MOV64XY, 3, 10),   // r3 = r10 - 2
                inst(ADD64XC, 3, -2),
                inst(CALL, BPF_FUNC_map_update), // r0 = addr_v1
                inst(MOV64XC, 0, 0),
                inst(EXIT),
               };
inst bm_opti20[N2] = {inst(STXB, 10, -2, 1),    // *(r10-2) = L8(input)
                      inst(MOV64XC, 1, 0x01),   // *(r10-1) = 0x01
                      inst(STXB, 10, -1, 1),
                      inst(MOV64XC, 1, 0),      // r1 = map0
                      inst(MOV64XY, 2, 10),     // r2 = r10 - 1
                      inst(ADD64XC, 2, -1),
                      inst(CALL, BPF_FUNC_map_lookup), // r0 = &map0[0x01]
                      inst(JEQXC, 0, 0, 7),   // if r0 == 0, exit
                      inst(LDXB, 1, 10, -2),  // r1 = L8(input)
                      inst(STXB, 0, 0, 1),    // *r0 = r1
                      inst(MOV64XC, 0, 0),
                      inst(),
                      inst(),
                      inst(),
                      inst(),
                      inst(),
                     };
inst bm3[N3] = {inst(191, 1, 6, 0, 0),
                inst(183, 0, 1, 0, 0),
                inst(97, 6, 2, 36, 0),
                inst(86, 0, 2, 4, 6),
                inst(0, 0, 0, 0, 0), // call 7 modified as nop
                inst(188, 0, 1, 0, 0),
                inst(103, 0, 1, 0, 32),
                inst(119, 0, 1, 0, 32),
                inst(123, 1, 10, -40, 0),
                inst(97, 6, 1, 4, 0),
                inst(99, 1, 10, -32, 0),
                inst(97, 6, 1, 24, 0),
                inst(99, 1, 10, -16, 0),
                inst(180, 0, 8, 0, 0),
                inst(107, 8, 10, -26, 0),
                inst(97, 10, 1, -16, 0),
                inst(107, 1, 10, -28, 0),
                inst(191, 10, 2, 0, 0),
                inst(7, 0, 2, 0, -40),
                inst(183, 0, 1, 0, 0),
                inst(0, 0, 0, 0),
                inst(133, 0, 0, 0, 1),
                inst(191, 0, 7, 0, 0),
                inst(21, 0, 7, 65, 0),
                inst(97, 7, 1, 0, 0),
                inst(99, 1, 10, -56, 0),
                inst(105, 7, 1, 4, 0),
                inst(107, 8, 10, -48, 0),
                inst(107, 8, 10, -50, 0),
                inst(107, 8, 10, -46, 0),
                inst(107, 1, 10, -52, 0),
                inst(22, 0, 1, 10, 0),
                inst(191, 10, 2, 0, 0),
                inst(7, 0, 2, 0, -56),
                inst(183, 0, 1, 0, 1),
                inst(0, 0, 0, 0),
                inst(133, 0, 0, 0, 1),
                inst(21, 0, 0, 2, 0),
                inst(105, 0, 1, 4, 0),
                inst(86, 0, 1, 10, 0),
                inst(180, 0, 1, 0, 0),
                inst(107, 1, 10, -52, 0),
                inst(191, 10, 2, 0, 0),
                inst(7, 0, 2, 0, -56),
                inst(183, 0, 1, 0, 1),
                inst(0, 0, 0, 0),
                inst(133, 0, 0, 0, 1),
                inst(21, 0, 0, 5, 0),
                inst(105, 0, 1, 4, 0),
                inst(22, 0, 1, 3, 0),
                inst(105, 7, 1, 6, 0),
                inst(105, 0, 2, 6, 0),
                inst(30, 1, 2, 20, 0),
                inst(191, 10, 2, 0, 0),
                inst(7, 0, 2, 0, -40),
                inst(183, 0, 1, 0, 0),
                inst(0, 0, 0, 0),
                inst(133, 0, 0, 0, 3),
                inst(183, 0, 6, 0, 0),
                inst(123, 6, 10, -8, 0),
                inst(123, 6, 10, -16, 0),
                inst(183, 0, 1, 0, 264),
                inst(123, 1, 10, -24, 0),
                inst(191, 10, 2, 0, 0),
                inst(7, 0, 2, 0, -24),
                inst(183, 0, 1, 0, 2),
                inst(0, 0, 0, 0),
                inst(133, 0, 0, 0, 1),
                inst(21, 0, 0, 9, 0),
                inst(121, 0, 1, 0, 0),
                inst(7, 0, 1, 0, 1),
                inst(123, 1, 0, 0, 0),
                inst(5, 0, 0, 16, 0),
                inst(97, 7, 1, 0, 0),
                inst(99, 1, 6, 4, 0),
                inst(105, 7, 1, 4, 0),
                inst(99, 1, 6, 24, 0),
                inst(5, 0, 0, 11, 0),
                inst(123, 6, 10, -8, 0),
                inst(183, 0, 1, 0, 1),
                inst(123, 1, 10, -16, 0),
                inst(191, 10, 2, 0, 0),
                inst(7, 0, 2, 0, -24),
                inst(191, 10, 3, 0, 0),
                inst(7, 0, 3, 0, -16),
                inst(183, 0, 1, 0, 2),
                inst(0, 0, 0, 0),
                inst(180, 0, 4, 0, 0),
                inst(133, 0, 0, 0, 2),
                inst(180, 0, 0, 0, 1),
                inst(149, 0, 0, 0, 0),
               };

// Struct needs to be defined because the loader writes to
// the .ins file using bpf_insn which has a different size
// than insn
struct bpf_insn {

    uint8_t opcode;
    uint8_t dst_reg:4;
    uint8_t src_reg:4;
    short off; 
    int imm; 
};


// trim from start (in place)
static inline void ltrim(std::string &s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int ch) {
        return !std::isspace(ch);
    }));
}

// trim from end (in place)
static inline void rtrim(std::string &s) {
    s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch) {
        return !std::isspace(ch);
    }).base(), s.end());
}

// trim from both ends (in place)
static inline void trim(std::string &s) {
    ltrim(s);
    rtrim(s);
}

map_attr extract_attrs_from_map(std::string curr_map) {
  
  int open_brace_idx = curr_map.find("{");
  int close_brace_idx = curr_map.find("}");
  std::string attr_str(curr_map.substr(open_brace_idx + 2, close_brace_idx - 2));  
  size_t comma_pos = 0;
  std::map <std::string, int> attr_map;

  while ((comma_pos = attr_str.find(',')) != std::string::npos) {

    std::string attr_assignment = attr_str.substr(0, comma_pos);
    size_t assign_pos = 0;
    if ((assign_pos = attr_assignment.find('=')) != std::string::npos) {
      std::string attr_name = attr_assignment.substr(0, assign_pos); 
      std::string attr_value_str = attr_assignment.substr(assign_pos + 1, attr_assignment.size() - 1);
      trim(attr_value_str);
      int attr_value = atoi(&attr_value_str[0]);
      trim(attr_name);
      attr_map[attr_name] = attr_value;
    }
    attr_str.erase(0, comma_pos + 1);
  }
 
  return map_attr(attr_map["key_size"], 
        attr_map["value_size"], 
        attr_map["max_entries"]);
}

void read_maps(char* map_file) {

  FILE* fp;

  fp = fopen(map_file, "r"); 
  char* line = NULL;
  size_t len = 0;
  ssize_t read;

  if (fp == NULL) {
    std::cerr<<"Error: BPF maps file could not be opened" << std::endl;
    exit(1);
  }
  while (read = getline (&line, &len, fp) != -1){

      std::string curr_map(line); 
      mem_t::add_map(extract_attrs_from_map(curr_map)); 
  }
  fclose(fp);
}
void read_insns(inst** bm, char* insn_file) {

    FILE *fp;
    fp = fopen(insn_file, "r"); 
    if (fp == NULL ) {
      std::cerr<<"Error: BPF insns file could not be opened" << std::endl;
      exit(1);
    }
    inst* insns;
    vector<inst> insn_vec;
    bpf_insn input;
    // read file contents till end of file 
    while(fread(&input, sizeof(bpf_insn), 1, fp)) {

      printf("opcode - %02x, src reg - %01x, dst reg - %01x, off - %04x, imm - %08x\t\n", 
        input.opcode, input.dst_reg, input.src_reg, input.off, input.imm); 

        inst curr_inst((int)input.opcode, 
                (int32_t)input.src_reg, 
                (int32_t)input.dst_reg, 
                (int32_t)input.off, 
                (int16_t)input.imm);
    
        curr_inst.print();
        cout << "Bytecode string: " << curr_inst.get_bytecode_str() << std::endl; 
        insn_vec.push_back(curr_inst);
        cout << std::endl;
        
    }
    *bm = &insn_vec[0];
    fclose(fp); 
  
}

void read_input(inst** bm, char* insn_file, char* map_file) {

  read_maps(map_file);
  read_insns(bm, insn_file);

}

void init_benchmarks(inst** bm, vector<inst*> &bm_optis_orig, int bm_id) {
  switch (bm_id) {
    case 0:
      inst::max_prog_len = N0;
      *bm = bm0;
      bm_optis_orig.push_back(bm_opti00);
      bm_optis_orig.push_back(bm_opti01);
      bm_optis_orig.push_back(bm_opti02);
      bm_optis_orig.push_back(bm_opti03);
      return;
    case 1:
      inst::max_prog_len = N1;
      inst::add_sample_imm(vector<int32_t> {0xff0000});
      mem_t::add_map(map_attr(8, 8, N1));
      *bm = bm1;
      bm_optis_orig.push_back(bm_opti10);
      return;
    case 2:
      inst::max_prog_len = N2;
      *bm = bm2;
      bm_optis_orig.push_back(bm_opti20);
      return;
    case 3:
      inst::max_prog_len = N3;
      inst::add_sample_imm(vector<int32_t> {264});
      mem_t::set_pkt_sz(128);
      mem_t::add_map(map_attr(128, 64, N3)); // 8 items
      mem_t::add_map(map_attr(96, 96, N3));  // 12 items
      mem_t::add_map(map_attr(64, 128, N3)); // 16 items  => 36 items
      *bm = bm3;
      return;
    default:
      cout << "ERROR: ebpf bm_id " + to_string(bm_id) + " is out of range {0, 1, 2}" << endl;
      return;
  }
}
