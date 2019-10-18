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
	string toStr();
	friend ostream& operator<<(ostream& out, const node& n);
};

class graph {
private:
	vector<node> nodes;
	vector<vector<unsigned int> > nodesIn;
	vector<vector<unsigned int> > nodesOut;
	size_t getEndInstID(inst* instLst, size_t start, size_t end);
	void genNodeStarts(inst* instLst, int length, set<size_t>& nodeStarts);
	void genNodeEnds(inst* instLst, int length, set<size_t>& nodeStarts, vector<size_t>& nodeEnds);
	void genAllNodesGraph(vector<node>& gNodes, set<size_t>& nodeStarts, vector<size_t>& nodeEnds);
	void genAllEdgesGraph(vector<vector<unsigned int> >& gNodesOut, vector<node>& gNodes, inst* instLst);
	void genIdMap(unsignedMap& idMap, vector<node>& gNodes);
	void addNode(node& nd, unsigned int& added);
	void checkLoopInDfs(size_t curgNodeId, vector<node>& gNodes, vector<unsigned int>& nextgNodeIds, \
	                    vector<bool>& visited, vector<bool>& finished);
	void dfs(size_t curgNodeId, vector<node>& gNodes, vector<vector<unsigned int> >& gNodesOut, \
	         vector<unsigned int>& added, vector<bool>& visited, vector<bool>& finished);
public:
	graph(inst* instLst, int length);
	~graph();
	friend ostream& operator<<(ostream& out, const graph& g);
};
