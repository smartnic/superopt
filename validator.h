#pragma once

#include <vector>
#include "inst.h"
#include "cfg.h"
#include "z3++.h"

using namespace z3;

// convert string s into expr e, the type of e is int_const
expr stringToExpr(string s);
ostream& operator<< (ostream& out, vector<expr>& _exprVec);
ostream& operator<< (ostream& out, vector<vector<expr> >& _exprVec);
bool isSMTValid(expr smt);

// SMT Variable format
// [type]_[progId]_[nodeId]_[regId/memId]_[versionId]
// [type]: r means register; m means memory
class smtVar {
private:
	string _name;
	// store the curId
	unsigned int regCurId[NUM_REGS];
	std::vector<expr> regVar;
public:
	// 1. Convert progId and nodeId into _name, that is string([progId]_[versId])
	// 2. Initialize regVal[i] = r_[_name]_0, i = 0, ..., NUM_REGS
	smtVar(unsigned int progId, unsigned int nodeId);
	~smtVar();
	// inital value for [versionId] is 0, and increases when updated
	expr updateRegVar(unsigned int regId);
	expr getCurRegVar(unsigned int regId);
	expr getInitRegVar(unsigned int regId);
};

class progSmt {
private:
	// f[i] is basic block logic FOL formula F of block logic i
	vector<expr> f;
	// program FOL formula; smt = f[0] && ... && f[n]
	expr smt = stringToExpr("true");
	// program output FOL formula; rename all output register values to the same name
	expr smtOutput = stringToExpr("true");
	// postRegVal[i] is post register values of basic block i,
	// which are initial values for NEXT basic blocks
	vector<vector<expr> > postRegVal;
	// return the SMT for the given program
	expr smtProg(inst* program, int length, smtVar* sv);
	// return SMT for the given instruction
	expr smtInst(smtVar* sv, inst* in);
	void initVariables();
	void topoSortDFS(size_t curBId, vector<unsigned int>& blocks, vector<bool>& finished);
	expr genBlockProgLogic(smtVar* sv, size_t curBId, inst* instLst);
	void storePostRegVal(smtVar* sv, size_t curBId);
	void smtJmpInst(smtVar* sv, vector<expr>& cInstEnd, inst& instEnd);
	void addPathCond(expr pCon, size_t curBId, size_t nextBId);
	void genPostPathCon(smtVar* sv, size_t curBId, inst& instEnd);
	expr getInitVal(smtVar* sv, size_t inBId);
	expr smtRetInst(size_t curBId, inst* instEnd, unsigned int progId);
	void processOutput(inst* instLst, unsigned int progId);
public:
	// store pathCon, regIV, bL, post, g
	// 1. pathCon[i] stores pre path condition formulas of basic block i
	// There is a corresponding relationship between pathCon and g.nodesIn
	// more specifically, pathCon[i][j] stores the pre path condition formula from basic block g.nodesIn[i][j] to i
	vector<vector<expr> > pathCon;
	// 2. regIV[i][j] stores pre register initial value formula
	// that values from the last node(g.nodes[i][j]) are fed to the node(i)
	vector<vector<expr> > regIV;
	// 3. bl[i] stores block logic formula of basic block i
	// more specifically, bL[i] = instLogic_i_0 && instLogic_i_1 && ... && instLogic_i_n
	vector<expr> bL;
	// 4. post[i][j] store post logic formula for basic block i
	// If block i is the end block, post[i][j] stores output formula
	// otherwise, post[i][j] stores post path condition formula.
	vector<vector<expr> > post;
	//control flow graph
	graph g;
	progSmt();
	~progSmt();
	expr genSmt(unsigned int progId, inst* instLst, int length);
};

class validator {
private:
	// return the SMT for the pre condition
	expr smtPre(unsigned int progId);// for program
	expr smtPre(expr e);
	// return the SMT for the post condition check
	expr smtPost(unsigned int progId1, unsigned int progId2);
	expr smtPost(unsigned int progId, expr e);
public:
	// store
	vector<expr> pre;
	vector<expr> p;
	expr post = stringToExpr("false");
	// f = pre^pre2^p1^p2 -> post
	expr f = stringToExpr("false");
	vector<progSmt> ps;

	validator();
	~validator();
	bool equalCheck(inst* instLst1, int len1, inst* instLst2, int len2);
	bool equalCheck(inst* instLst1, int len1, expr fx, expr input, expr output);
};
