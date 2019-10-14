#include <iostream>
#include "cfg.h"

using namespace std;

#define Others 0
#define CONDJMP 1
#define UNCONDJMP 2  // have not implemented UNCONDJMP
#define CALLEND 3    // have not implemented CALLEND
#define END 4        // RETC & RETX regarded as the end of program

int getInstType(inst* ins) {
	switch (ins->_opcode) {
	case RETX: return END;
	case RETC: return END;
	case JMPEQ: return CONDJMP;
	case JMPGT: return CONDJMP;
	case JMPGE: return CONDJMP;
	case JMPLT: return CONDJMP;
	case JMPLE: return CONDJMP;
	default: return Others;
	}
}

/* class node start */
node::node(unsigned int start, unsigned int end) {
	_start = start;
	_end = end;
}

node::~node() {}

ostream& operator<<(ostream& out, const node& n) {
	out << "[" << n._start << ":" << n._end << "]";
	return out;
}

void graph::genInstStarts(inst* insn, int length, set<size_t>& instStarts) {
	if (getInstType(&insn[0]) != CONDJMP) {
		instStarts.insert(0);
	}
	for (size_t i = 0; i < length; i++) {
		int instType = getInstType(&insn[i]);
		if (instType == END) {
			if (i + 1 < length) {
				instStarts.insert(i + 1);
			}
		}
		else if (instType == CONDJMP) {
			instStarts.insert(i + 1);
			instStarts.insert(i + 1 + insn[i]._args[2]);
		}
	}
}

void graph::genNodes(set<size_t>& instStarts, int length, vector<node>& nodesLst) {
	set<size_t>::iterator it = instStarts.begin();
	set<size_t>::iterator it2 = instStarts.begin();
	for (it2++; it2 != instStarts.end(); it++, it2++) {
		node nd(*it, *it2 - 1);
		nodesLst.push_back(nd);
	}
	node nd(*it, length - 1);
	nodesLst.push_back(nd);
}

// key: node._start, value: nodeId
void graph::genIdMap(unsignedMap& idMap, vector<node>& nodesLst) {
	for (size_t i = 0; i < nodesLst.size(); i++) {
		idMap[nodesLst[i]._start] = i;
	}
}

void graph::addNode(node nd) {
	nodes.push_back(nd);
	nodesIn.push_back(vector<unsigned int> {});
	nodesOut.push_back(vector<unsigned int> {});
}

void graph::dfs(unsigned int curNodeId, inst* insn, vector<node>& nodesLst, unsignedMap& inId2NdId, \
                vector<unsigned int>& added, vector<bool>& finished, vector<bool>& visited) {
	if (finished[curNodeId]) {
		return;
	}

	// 0 set curNodeId visited
	visited[curNodeId] = true;

	// 1 get next nodeIds of nodesLst
	unsigned int instId = nodes[curNodeId]._end;
	int instType = getInstType(&insn[instId]);
	vector<unsigned int> nextNodeIds;
	if (instType == END) {
		finished[curNodeId] = true;
		return;
	}
	else if (instType == Others) {
		nextNodeIds.push_back(inId2NdId[instId + 1]);
	}
	else if (instType == CONDJMP) {
		nextNodeIds.push_back(inId2NdId[instId + 1]);
		nextNodeIds.push_back(inId2NdId[instId + 1 + insn[instId]._args[2]]);
	}
	// 2 check loop. loop exists if the visited[i] && !fininshed[i]
	for (size_t i = 0; i < nextNodeIds.size(); i++) {
		if (visited[nextNodeIds[i]] && (!finished[nextNodeIds[i]])) {
			string errMsg  = "illegal input: loop from instruction " + \
			              to_string(nodes[curNodeId]._end) + \
			              " to instruction " + \
			              to_string(nextNodeIds[i]);
			throw (errMsg);
		}
	}

	// 3 for each next node, update the graph
	for (size_t i = 0; i < nextNodeIds.size(); i++) {
		unsigned int nextNodeId;
		// add node into the graph
		if (added[nextNodeIds[i]] == -1) {
			addNode(nodesLst[nextNodeIds[i]]);
			nextNodeId = nodes.size() - 1;
			added[nextNodeIds[i]] = nextNodeId;
		}
		else {
			nextNodeId = added[nextNodeIds[i]];
		}
		// add edge into the graph
		nodesIn[nextNodeId].push_back(curNodeId);
		nodesOut[curNodeId].push_back(nextNodeId);
		// dfs nextNode
		try {
			dfs(nextNodeId, insn, nodesLst, inId2NdId, added, finished, visited);
		}
		catch (const string errMsg) {
			throw errMsg;
		}
	}

	// 4 set curNodeId finished
	finished[curNodeId] = true;
	return;
}

graph::graph(inst* insn, int length) {
	// 1 generate nodesLst, which contains all the nodes
	set<size_t> instStarts;
	genInstStarts(insn, length, instStarts);
	vector<node> nodesLst;
	genNodes(instStarts, length, nodesLst);

	// 2 generate nodes/edges in graph
	// 2.1 generate the Map inId2NdId: instruction Id in insn to node id in nodeLst
	unsignedMap inId2NdId;
	genIdMap(inId2NdId, nodesLst);
	// 2.2 add nodes/edges into the graph by dfs
	// added[i] means the nodesLst[i] has been added into the graph, and nodesLst[i] = nodes[added[i]]
	vector<unsigned int> added(nodesLst.size(), -1);
	// visited[i] = true means the node has been visited
	// used to check loop. loop exists if the visited[i] && !finished[i]
	vector<bool> visited(nodesLst.size(), 0);
	// finished[i] = true means the node in nodesLst has finished DFS of the subtree(s) of nodesLst[i]
	vector<bool> finished(nodesLst.size(), 0);
	addNode(nodesLst[0]); // add the root
	try {
		dfs(0, insn, nodesLst, inId2NdId, added, finished, visited);
	}
	catch (const string errMsg) {
		throw errMsg;
	}
}

graph::~graph() {}

ostream& operator<<(ostream& out, const graph& g) {
	cout << endl << "nodes:" << endl << " ";
	for (size_t i = 0; i < g.nodes.size(); i++) {
		cout << i << g.nodes[i] << " ";
	}
	cout << endl << "edges:" << endl;
	for (size_t i = 0; i < g.nodes.size(); i++) {
		out << " " << i << " in:";
		for (size_t j = 0; j < g.nodesIn[i].size(); j++) {
			out << g.nodesIn[i][j] << " ";
		}
		out << " out:";
		for (size_t j = 0; j < g.nodesOut[i].size(); j++) {
			out << g.nodesOut[i][j] << " ";
		}
		out << endl;
	}
	return out;
}
