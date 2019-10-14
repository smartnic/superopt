#include <iostream>
#include "cfg.h"
#include "inst.h"

using namespace std;

int main () {
	inst instructions1[6] = {inst(MOVXC, 1, 4),     // mov r1, 4
	                         inst(ADDXY, 0, 1),     // add r0, r1
	                         inst(MOVXC, 2, 15),    // mov r2, 15
	                         inst(JMPGT, 0, 2, 1),  // if r0 <= r2:
	                         inst(RETX, 2),         // ret r2
	                         inst(RETX, 0),         // else ret r0
	                        };
	graph g1(instructions1, 6);
	cout << "graph1 is" << g1 << endl;

	inst instructions2[6] = {inst(MOVXC, 1, 4),     // mov r0, 4
	                         inst(JMPGT, 0, 2, 3),  // if r0 <= r2:
	                         inst(MOVXC, 2, 15),    // mov r2, 15
	                         inst(JMPGT, 0, 2, 1),  // if r0 <= r2:
	                         inst(RETX, 2),         // ret r2
	                         inst(RETX, 0),         // else ret r0
	                        };
	graph g2(instructions2, 6);
	cout << "graph2 is" << g2 << endl;

	inst instructions3[7] = {inst(MOVXC, 1, 4),     // mov r1, 4
	                         inst(JMPGT, 0, 2, 3),  // if r0 <= r3:
	                         inst(RETX, 0),         // else ret r0
	                         inst(MOVXC, 2, 15),    // mov r2, 15
	                         inst(JMPGT, 0, 2, 1),  // if r0 <= r2:
	                         inst(RETX, 2),         // ret r2
	                         inst(RETX, 0),         // else ret r0
	                        };
	graph g3(instructions3, 7);
	cout << "graph3 is" << g3 << endl;

	// test illegal input with loop 
	cout << "graph4 is" << endl;
	inst instructions4[6] = {inst(MOVXC, 1, 4),     // 0 mov r0, 4
	                         inst(JMPGT, 0, 2, 3),  // 1 if r0 <= r2:
	                         inst(MOVXC, 2, 15),    // 2 mov r2, 15
	                         inst(JMPGT, 0, 2, -4), // 3 loop from here to instruction 0
	                         inst(RETX, 2),         // 4 ret r2
	                         inst(RETX, 0),         // 5 else ret r0
	                        };
	try {
		graph g4(instructions4, 6);
	}
	catch (const string errMsg) {
		cerr << errMsg << endl;
	}
	return 0;
}
