#pragma once

#include <iostream>
#include <vector>
#include <set>
#include <unordered_map>
#include "inst.h"

using namespace std;

typedef unordered_map<unsigned int, unsigned int> unsignedMap;

class node {
private:
public:
	unsigned int _start = 0; // start intruction ID
	unsigned int _end = 0;   // end instruction ID
	node(unsigned int start, unsigned int end);
	~node();
	friend ostream& operator<<(ostream& out, const node& n);
};

class graph {
private:
	vector<node> nodes;
	vector<vector<unsigned int> > nodesIn;
	vector<vector<unsigned int> > nodesOut;
	void genInstStarts(inst* insn, int length, set<size_t>& instStarts);
	void genIdMap(unsignedMap& idMap, vector<node>& nodesLst);
	void genNodes(set<size_t>& instStarts, int length, vector<node>& nodesLst);
	void addNode(node nd);
	void dfs(unsigned int curNodeId, inst* insn, vector<node>& nodesLst, unsignedMap& inId2NdId, \
                vector<unsigned int>& added, vector<bool>& finished, vector<bool>& visited);
public:
	graph(inst* insn, int length);
	~graph();
	friend ostream& operator<<(ostream& out, const graph& g);
};
