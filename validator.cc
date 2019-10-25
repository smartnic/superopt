#include <iostream>
#include <vector>
#include <string>
#include "validator.h"
#include "inst.h"
#include "cfg.h"
#include "z3++.h"

using namespace z3;

context c;

#define CURSRC sv->getCurRegVar(SRCREG(in))
#define CURDST sv->getCurRegVar(DSTREG(in))
#define NEWDST sv->updateRegVar(DSTREG(in))
#define IMM2 IMM2VAL(in)

expr stringToExpr(string s) {
	if (s == "true") {
		return c.bool_val(true);
	}
	else if (s == "false") {
		return c.bool_val(false);
	}
	return c.int_const(s.c_str());
}

ostream& operator<<(ostream& out, vector<expr>& _exprVec) {
	if (_exprVec.size() > 0) {
		out << "\n";
	}
	for (size_t i = 0; i < _exprVec.size(); i++) {
		out << i << ": " << _exprVec[i] << "\n";
	}
	return out;
}

ostream& operator<<(ostream& out, vector<vector<expr> >& _exprVec) {
	if (_exprVec.size() > 0) {
		out << "\n";
	}
	for (size_t i = 0; i < _exprVec.size(); i++) {
		out << "block" << i << ": " << _exprVec[i] << "\n";
	}
	return out;
}

bool isSMTValid(expr smt) {
	solver s(c);
	s.add(!smt);
	switch (s.check()) {
	case unsat: return true;
	case sat: return false;
	case unknown: return false;
	}
	return false;
}

/* class smtVar start */
smtVar::smtVar(unsigned int progId, unsigned int nodeId) {
	_name = std::to_string(progId) + "_" + std::to_string(nodeId);
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

expr smtVar::getInitRegVar(unsigned int regId) {
	std::string name = "r_" + _name + "_" + std::to_string(regId) + "_0";
	return stringToExpr(name);
}
/* class smtVar end */

/* class progSmt start */
progSmt::progSmt() {}

progSmt::~progSmt() {}

// assume program has no branch and is an ordered sequence of instructions
expr progSmt::smtProg(inst* program, int length, smtVar* sv) {
	inst* instLst = program;
	// length = 1
	if (length == 1) {
		return smtInst(sv, &instLst[0]);
	}
	int instLength = length;
	// length > 1, end with END or JMP instruction
	if (getInstType(instLst[length - 1]) == CFG_END) {
		instLength = length - 1;
	}
	expr p = smtInst(sv, &instLst[0]);
	for (size_t i = 1; i < instLength; i++) {
		p = p and smtInst(sv, &instLst[i]);
	}
	return p;
}

expr progSmt::smtInst(smtVar* sv, inst* in) {
	switch (in->_opcode) {
	case NOP: {
		return stringToExpr("true");
	}
	case ADDXY: {
		return (CURDST + CURSRC == NEWDST);
	}
	case MOVXC: {
		return (IMM2 == NEWDST);
	}
	case MAXC: {
		expr oldDst = CURDST;
		expr newDst = NEWDST;
		expr cond1 = (oldDst > IMM2) and (newDst == oldDst);
		expr cond2 = (oldDst <= IMM2) and (newDst == IMM2);
		return (cond1 or cond2);
	}
	case MAXX: {
		expr oldDst = CURDST;
		expr newDst = NEWDST;
		expr cond1 = (oldDst > CURSRC) and (newDst == oldDst);
		expr cond2 = (oldDst <= CURSRC) and (newDst == CURSRC);
		return (cond1 or cond2);
	}
	case RETX: {
		return stringToExpr("true");
	}
	case RETC: {
		return stringToExpr("true");
	}
	case JMPEQ: {
		return stringToExpr("true");
	}
	case JMPGT: {
		return stringToExpr("true");
	}
	case JMPGE: {
		return stringToExpr("true");
	}
	case JMPLT: {
		return stringToExpr("true");
	}
	case JMPLE: {
		return stringToExpr("true");
	}
	default: {
		return stringToExpr("false");
	}
	}
}

// init f, postRegVal, postPathCon
void progSmt::initVariables() {
	f.clear();
	postRegVal.clear();
	pathCon.clear();
	regIV.clear();
	bL.clear();
	post.clear();

	size_t blockNum = g.nodes.size();
	f.resize(blockNum, stringToExpr("true"));

	postRegVal.resize(blockNum);
	for (size_t i = 0; i < blockNum; i++) {
		postRegVal[i].resize(NUM_REGS, stringToExpr("true"));
	}

	pathCon.resize(blockNum);
	for (size_t i = 0; i < blockNum; i++) {
		// Because of the corresponding relationship between pathCon and g.nodesIn,
		// the size of pathCon[i] is equal to that of g.nodesIn[i];
		pathCon[i].resize(g.nodesIn[i].size(), stringToExpr("true"));
	}

	regIV.resize(blockNum);
	for (size_t i = 0; i < blockNum; i++) {
		regIV[i].resize(g.nodesIn[i].size(), stringToExpr("true"));
	}
	bL.resize(blockNum, stringToExpr("false"));
	post.resize(blockNum);
	for (size_t i = 0; i < blockNum; i++) {
		if (g.nodesOut[i].size() > 0) {
			post[i].resize(g.nodesOut[i].size(), stringToExpr("false"));
		}
		else {
			post[i].resize(1, stringToExpr("false"));
		}
	}
}

// topological sorting by DFS
void progSmt::topoSortDFS(size_t curBId, vector<unsigned int>& blocks, vector<bool>& finished) {
	if (finished[curBId]) {
		return;
	}
	for (size_t i = 0; i < g.nodesOut[curBId].size(); i++) {
		topoSortDFS(g.nodesOut[curBId][i], blocks, finished);
	}
	finished[curBId] = true;
	blocks.push_back(curBId);
}

expr progSmt::genBlockProgLogic(smtVar* sv, size_t curBId, inst* instLst) {
	inst* start = &instLst[g.nodes[curBId]._start];
	int length = g.nodes[curBId]._end - g.nodes[curBId]._start + 1;
	expr e = smtProg(start, length, sv);
	bL[curBId] = e; // store
	return e;
}

void progSmt::storePostRegVal(smtVar* sv, size_t curBId) {
	for (size_t i = 0; i < NUM_REGS; i++) {
		postRegVal[curBId][i] = sv->getCurRegVar(i);
	}
}

void progSmt::smtJmpInst(smtVar* sv, vector<expr>& cInstEnd, inst& instEnd) {
	inst* in = &instEnd;
	// e is formula for Jmp
	expr e = stringToExpr("true");
	switch (instEnd._opcode) {
	case JMPEQ: {e = (CURDST == CURSRC); break;}
	case JMPGT: {e = (CURDST > CURSRC); break;}
	case JMPGE: {e = (CURDST >= CURSRC); break;}
	case JMPLT: {e = (CURDST < CURSRC); break;}
	case JMPLE: {e = (CURDST <= CURSRC); break;}
	}
	// keep order: insert no jmp first
	cInstEnd.push_back(!e); // no jmp
	cInstEnd.push_back(e);  // jmp
}

// update path condition "pCon" generated by curBId into the pathCon[nextBId]
void progSmt::addPathCond(expr pCon, size_t curBId, size_t nextBId) {
	for (size_t j = 0; j < g.nodesIn[nextBId].size(); j++) {
		if (g.nodesIn[nextBId][j] == curBId) {
			pathCon[nextBId][j] = pCon;
		}
	}
}

// steps: 1. calculate c_in;
// 2. calculate post path condition "c" of current basic block curBId;
// 3. use c to update pathCon[nextBId]
// three cases: 1. no next blocks 2. one next block 3. two next blocks
void progSmt::genPostPathCon(smtVar* sv, size_t curBId, inst& instEnd) {
	// case 1: no next blocks
	if (g.nodesOut[curBId].size() == 0) {
		return;
	}
	// step 1. calculate c_in;
	// When curBId is processed, pathCon[curBId] already has been
	// updated with the correct value because of topo sort
	// if current block(i.e., block 0) has no incoming edges, set c_in = true.
	expr cIn = stringToExpr("true");
	if (pathCon[curBId].size() > 0) { // calculate c_in by parents, that is, pathCon[curBId]
		cIn = pathCon[curBId][0];
		for (size_t i = 1; i < g.nodesIn[curBId].size(); i++) {
			// cIn = (cIn || pathCon[curBId][i]).simplify(); //TODO: imporve later
			cIn = (cIn || pathCon[curBId][i]);
		}
	}
	// case 2: one next block
	// In this case, the post path condition is same as the c_in
	if (g.nodesOut[curBId].size() == 1) {
		unsigned int nextBId = g.nodesOut[curBId][0];
		// update path condition to the pathCon[nextBId]
		addPathCond(cIn, curBId, nextBId);
		post[curBId][0] = cIn; //store
		return;
	}
	// case 3: two next blocks
	// If keep order: cInstEnd[0]: no jmp path condition; cInstEnd[1]: jmp path condition,
	// then cInstEnd[i] -> g.nodesOut[curBId][i];
	// Why: according to the process of jmp path condition in function genAllEdgesGraph in cfg.cc
	// function smtJmpInst keep this order
	// case 3 step 2
	vector<expr> cInstEnd;
	smtJmpInst(sv, cInstEnd, instEnd);
	// case 3 step 3
	// push the cInstEnd[0] and cInstEnd[1] into nextBIds' pathCon
	// no jmp
	unsigned int nextBId = g.nodesOut[curBId][0];
	expr cNextBId = cIn && cInstEnd[0];
	addPathCond(cNextBId, curBId, nextBId);
	post[curBId][0] = cNextBId;
	// jmp
	nextBId = g.nodesOut[curBId][1];
	cNextBId = cIn && cInstEnd[1];
	addPathCond(cNextBId, curBId, nextBId);
	post[curBId][1] = cNextBId;
}

expr progSmt::getInitVal(smtVar* sv, size_t inBId) {
	expr e = (postRegVal[inBId][0] == sv->getInitRegVar(0));
	for (size_t i = 1; i < NUM_REGS; i++) {
		e = e && (postRegVal[inBId][i] == sv->getInitRegVar(i));
	}
	return e;
}

// TODO: needed to be generalized
// for each return value v, smt: v == output[progId]
expr progSmt::smtRetInst(size_t curBId, inst* instEnd, unsigned int progId) {
	switch (instEnd->_opcode) {
	case RETX:
		return (postRegVal[curBId][DSTREG(instEnd)] == stringToExpr("output" + to_string(progId)));
	case RETC:
		return (IMM1VAL(instEnd) == stringToExpr("output" + to_string(progId)));
	default: // if no RET, return r0
		return (postRegVal[curBId][0] == stringToExpr("output" + to_string(progId)));
	}
}

void progSmt::processOutput(inst* instLst, unsigned int progId) {
	expr e = stringToExpr("true");
	// search all basic blocks for the basic blocks without outgoing edges
	for (size_t i = 0; i < g.nodes.size(); i++) {
		if (g.nodesOut[i].size() != 0) continue;
		// process END instruction
		expr cIn = stringToExpr("true");
		// if current block has incoming edges
		if (g.nodesIn[i].size() > 0) {
			cIn = pathCon[i][0];
			for (size_t j = 1; j < g.nodesIn[i].size(); j++) {
				// cIn = (cIn || pathCon[i][j]).simplify();
				cIn = (cIn || pathCon[i][j]);
			}
		}
		expr e1 = smtRetInst(i, &instLst[g.nodes[i]._end], progId);
		expr e2 = implies(cIn, e1);
		e = e && e2;
		post[i][0] = e2; // store
	}
	smtOutput = e;
}

expr progSmt::genSmt(unsigned int progId, inst* instLst, int length) {
	try {
		g.genGraph(instLst, length);
	}
	catch (const string errMsg) {
		cerr << errMsg << endl;
	}
	// init class variables
	initVariables();

	// blocks stores the block IDs in order after topological sorting
	vector<unsigned int> blocks;
	vector<bool> finished(g.nodes.size(), false);
	topoSortDFS(0, blocks, finished);
	std::reverse(blocks.begin(), blocks.end());

	// process each basic block in order
	for (size_t i = 0; i < blocks.size(); i++) {
		unsigned int b = blocks[i];
		smtVar sv(progId, b);
		// generate f_pl: the block program logic
		expr fPL = genBlockProgLogic(&sv, b, instLst);
		if (b == 0) {
			f[0] = fPL;
		}
		else {
			for (size_t j = 0; j < g.nodesIn[b].size(); j++) {
				// generate f_iv: the logic that the initial values are fed by the last basic block
				expr fIV = getInitVal(&sv, g.nodesIn[b][j]);
				regIV[b][j] = fIV; // store
				f[b] = f[b] && implies(pathCon[b][j], fPL && fIV);
			}
		}
		// store post iv for current basic block b
		storePostRegVal(&sv, b);
		// update post path condtions "pathCon" created by current basic block b
		genPostPathCon(&sv, b, instLst[g.nodes[b]._end]);
	}

	for (size_t i = 0; i < f.size(); i++) {
		smt = smt && f[i];
	}

	processOutput(instLst, progId);
	return (smt && smtOutput).simplify();
}
/* class progSmt end */

/* class validator start */
validator::validator() {
}

validator::~validator() {}

// assgin input r0 "input", other registers 0
expr validator::smtPre(unsigned int progId) {
	smtVar sv(progId, 0);
	expr p = (sv.getCurRegVar(0) == stringToExpr("input"));
	for (size_t i = 1; i < NUM_REGS; i++) {
		p = p and (sv.getCurRegVar(i) == 0);
	}
	return p;
}

expr validator::smtPre(expr e) {
	return (e == stringToExpr("input"));
}

expr validator::smtPost(unsigned int progId1, unsigned int progId2) {
	return (stringToExpr("output" + to_string(progId1)) == \
	        stringToExpr("output" + to_string(progId2)));
}

expr validator::smtPost(unsigned int progId, expr e) {
	return (stringToExpr("output" + to_string(progId)) == e);
}

void validator::init() {
	pre.clear();
	p.clear();
	ps.clear();
}

bool validator::equalCheck(inst* instLst1, int len1, inst* instLst2, int len2) {
	init();
	vector<unsigned int> progId;
	progId.push_back(1);
	progId.push_back(2);
	expr pre1 = smtPre(progId[0]);
	expr pre2 = smtPre(progId[1]);
	progSmt ps1, ps2;
	expr p1 = ps1.genSmt(progId[0], instLst1, len1);
	expr p2 = ps2.genSmt(progId[1], instLst2, len2);
	expr pst = smtPost(progId[0], progId[1]);
	expr smt = implies(pre1 && pre2 && p1 && p2, pst);
	// store
	pre.push_back(pre1);
	pre.push_back(pre2);
	ps.push_back(ps1);
	ps.push_back(ps2);
	p.push_back(p1);
	p.push_back(p2);
	post = pst;
	f = smt;

	return isSMTValid(smt);
}

bool validator::equalCheck(inst* instLst1, int len1, expr fx, expr input, expr output) {
	init();
	vector<unsigned int> progId;
	progId.push_back(1);
	progId.push_back(2);
	expr pre1 = smtPre(progId[0]);
	expr pre2 = smtPre(input);
	progSmt ps1;
	expr p1 = ps1.genSmt(progId[0], instLst1, len1);
	expr p2 = fx;
	expr pst = smtPost(progId[0], output);
	expr smt = implies(pre1 && pre2 && p1 && p2, pst);
	// store
	pre.push_back(pre1);
	pre.push_back(pre2);
	ps.push_back(ps1);
	p.push_back(p1);
	p.push_back(p2);
	post = pst;
	f = smt;

	return isSMTValid(smt);
}
/* class validator end */
