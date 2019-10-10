#pragma once

#include <vector>
#include "inst.h"
#include "z3++.h"

using namespace z3;

// SMT Variable format
// [type]_[progId]_[versId]_[regId/memId]_[curId]
// [type]: r means register; m means memory
class smtVar {
private:
	string _name;
	// store the curId
	unsigned int regCurId[NUM_REGS];
	std::vector<expr> regVar;
public:
	// 1. Convert progId and versId into _name, that is string([progId]_[versId])
	// 2. Initialize regVal[i] = r_[_name]_0, i = 0, ..., NUM_REGS
	smtVar(unsigned int progId, unsigned int versId);
	~smtVar();
	// inital value for [curId] is 0, and increases when updated
	expr updateRegVar(unsigned int regId);
	expr getCurRegVar(unsigned int regId);
};

// return true if program1 == program2, otherwise false
bool isEqualProg(inst* program1, int lengthP1, inst* program2, int lengthP2);
// return the SMT for the given program (SMT = smtPre + program logic)
expr smtProg(inst* program, int length, smtVar* sv);
// return the SMT for the pre condition
expr smtPre(smtVar* sv);
// return the SMT for the post condition check
expr smtPost(smtVar* svP1, smtVar* svP2);
// return SMT for the given instruction
expr smtInst(smtVar* sv, inst in);
// return SMT for the instruction ADDXY
// expr smtNOP();
expr smtADDXY(smtVar* sv, int dst, int src);
expr smtMOVXC(smtVar* sv, int dst, int imm2);
expr smtMAXC(smtVar* sv, int dst, int imm2);
expr smtMAXX(smtVar* sv, int dst, int src);
// convert string s into expr e, the type of e is int_const
expr stringToExpr(string s);
