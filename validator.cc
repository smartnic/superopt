#include <iostream>
#include <vector>
#include <string>
#include "validator.h"
#include "inst.h"
#include "z3++.h"

using namespace z3;

context c;

smtVar::smtVar(unsigned int progId, unsigned int versId) {
	_name = std::to_string(progId) + "_" + std::to_string(versId);
	std::memset(regCurId, 0, NUM_REGS * sizeof(unsigned int));
	std::string namePrefix = "r_" + _name + "_";
	for (size_t i = 0; i < NUM_REGS; i++) {
		std::string name = namePrefix + std::to_string(i) + "_0";
		regVar.push_back(stringToExpr(name));
	}
}

smtVar::~smtVar() {
}

expr smtVar::updateRegVar(unsigned int regId) {
	regCurId[regId]++;
	std::string name = "r_" + _name + "_" + std::to_string(regId) \
	                   + "_" + std::to_string(regCurId[regId]);
	regVar[regId] = stringToExpr(name);
	return getCurRegVar(regId);
}

expr smtVar::getCurRegVar(unsigned int regId) {
	return regVar[regId];
}


bool isEqualProg(inst* program1, int lengthP1, inst* program2, int lengthP2) {
	// smt = (pre1^pre2)^(p1^p2) => post1==post2
	smtVar svP1(1, 1);
	smtVar svP2(2, 1);
	expr pre1 = smtPre(&svP1);
	expr pre2 = smtPre(&svP2);
	expr p1 = smtProg(program1, lengthP1, &svP1);
	expr p2 = smtProg(program2, lengthP2, &svP2);
	expr post = smtPost(&svP1, &svP2);
	expr smt = implies(pre1 and pre2 and p1 and p2, post);
	// std::cout << "SMT is:\n" << smt << "\n";
	solver s(c);
	s.add(!smt);
	switch (s.check()) {
	case unsat: return true;
	case sat: return false;
	case unknown: return false;
	}
	return false;
}

// assume program has no branch and is an ordered sequence of instructions
expr smtProg(inst* program, int length, smtVar* sv) {
	// TODO: deal with illegal input
	if (length < 1) {}

	inst* insn = program;
	expr p = smtInst(sv, &insn[0]);
	for (size_t i = 1; i < length; i++) {
		p = p and smtInst(sv, &insn[i]);
	}
	return p;
}

// TODO: needed to be generalized
// assgin all registers different values
expr smtPre(smtVar* sv) {
	expr p = (sv->getCurRegVar(0) == stringToExpr("pre0"));
	for (size_t i = 1; i < NUM_REGS; i++) {
		expr e = stringToExpr("pre" + std::to_string(i));
		p = p and (sv->getCurRegVar(i) == e);
	}
	return p;
}

// check whether all registers in program 1
// have the same value as the same register in program 2
expr smtPost(smtVar* svP1, smtVar* svP2) {
	expr p = (svP1->getCurRegVar(0) == svP2->getCurRegVar(0));
	for (size_t i = 1; i < NUM_REGS; i++) {
		p = p and (svP1->getCurRegVar(i) == svP2->getCurRegVar(i));
	}
	return p;
}

expr smtInst(smtVar* sv, inst* in) {
#define OLDSRC sv->getCurRegVar(SRCREG(in))
#define OLDDST sv->getCurRegVar(DSTREG(in))
#define NEWDST sv->updateRegVar(DSTREG(in))
#define IMM2 IMM2VAL(in)

	switch (in->_opcode) {
	// case NOP: return smtNOP();
	case ADDXY: {
		return (OLDDST + OLDSRC == NEWDST);
	}
	case MOVXC: {
		return (IMM2 == NEWDST);
	}
	case MAXC: {
		expr oldDst = OLDDST;
		expr newDst = NEWDST;
		expr cond1 = (oldDst > IMM2) and (newDst == oldDst);
		expr cond2 = (oldDst <= IMM2) and (newDst == IMM2);
		return (cond1 or cond2);
	}
	case MAXX: {
		expr oldDst = OLDDST;
		expr newDst = NEWDST;
		expr cond1 = (oldDst > OLDSRC) and (newDst == oldDst);
		expr cond2 = (oldDst <= OLDSRC) and (newDst == OLDSRC);
		return (cond1 or cond2);
	}
	default:
		return stringToExpr("unknown_opcode");
	}
}

expr stringToExpr(string s) {
	return c.int_const(s.c_str());
}
