#include "../../src/utils.h"
#include "prog.h"

void test1() {
  // test r10 (value of frame pointer), r1 won't be modified in canonicalize()
  inst insts1[7] = {inst(STXW, 10, -4, 1),
                    inst(LDXW, 0, 10, -4),
                    inst(EXIT),
                    inst(),
                    inst(),
                    inst(),
                    inst(),
                   };
  prog p11(insts1);
  prog p12(insts1);
  p11.canonicalize();
  print_test_res(p11 == p12, "canonicalize 1");
  return;

  // test input r1 won't be modified
  inst insts21[7] = {inst(MOV64XC, 3, 0x1),
                     inst(MOV64XC, 2, 0x2),
                     inst(MOV64XY, 0, 1),
                     inst(EXIT),
                     inst(),
                     inst(),
                     inst(),
                    };
  inst insts22[7] = {inst(MOV64XC, 0, 0x1),
                     inst(MOV64XC, 2, 0x2),
                     inst(MOV64XY, 3, 1),
                     inst(EXIT),
                     inst(),
                     inst(),
                     inst(),
                    };
  prog p21(insts21);
  prog p22(insts22);
  p21.canonicalize();
  print_test_res(p21 == p22, "canonicalize 2");  

  // test when r1 is not used, r1 can be used in register renaming
  inst insts31[7] = {inst(MOV64XC, 3, 0x1),
                     inst(MOV64XC, 2, 0x2),
                     inst(MOV64XY, 0, 2),
                     inst(EXIT),
                     inst(),
                     inst(),
                     inst(),
                    };
  inst insts32[7] = {inst(MOV64XC, 0, 0x1),
                     inst(MOV64XC, 1, 0x2),
                     inst(MOV64XY, 2, 1),
                     inst(EXIT),
                     inst(),
                     inst(),
                     inst(),
                    };
  prog p31(insts31);
  prog p32(insts32);
  p31.canonicalize();
  print_test_res(p31 == p32, "canonicalize 3"); 
}

int main() {
  test1();
  return 0;
}
