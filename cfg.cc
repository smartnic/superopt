#include <iostream>
#include "cfg.h"

using namespace std;

#define CFG_OTHERS 0
#define CFG_CONDJMP 1
#define CFG_END 2       // RETC & RETX regarded as the end of program

int getInstType(inst& ins) {
	switch (ins._opcode) {
	case RETX: return CFG_END;
	case RETC: return CFG_END;
	case JMPEQ: return CFG_CONDJMP;
	case JMPGT: return CFG_CONDJMP;
	case JMPGE: return CFG_CONDJMP;
	case JMPLT: return CFG_CONDJMP;
	case JMPLE: return CFG_CONDJMP;
	default: return CFG_OTHERS;
	}
}

/* class node start */
node::node(unsigned int start, unsigned int end) {
	_start = start;
	_end = end;
}

node::~node() {}

string node::toStr( ) {
	string s = "[" + to_string(_start) + ":" + to_string(_end) + "]";
	return s;
}

ostream& operator<<(ostream& out, const node& n) {
	out << "[" << n._start << ":" << n._end << "]";
	return out;
}

void graph::genNodeStarts(inst* instLst, int length, set<size_t>& nodeStarts) {
	nodeStarts.insert(0);
	for (size_t i = 0; i < length; i++) {
		if (getInstType(instLst[i]) == CFG_CONDJMP) {
			if ((i + 1) < length) {
				nodeStarts.insert(i + 1);
			}
			if ((i + 1 + instLst[i]._args[2]) < length) {
				nodeStarts.insert(i + 1 + instLst[i]._args[2]);
			}
		}
	}
}

// return end instruction ID in [start: end]
size_t graph::getEndInstID(inst* instLst, size_t start, size_t end) {
	for (size_t i = start; i < end; i++) {
		if (getInstType(instLst[i]) == CFG_END) {
			return i;
		}
	}
	return end;
}

void graph::genNodeEnds(inst* instLst, int length, set<size_t>& nodeStarts, vector<size_t>& nodeEnds) {
	/* Traverse all starts in nodeStarts, find an end for each start
	   The end for all starts except the last one is the CFG_END instruction OR the ${next start - 1} instruction
	   The end for the last start is the CFG_END instruction OR the last instruction.
	*/
	// ends for all starts except the last start
	set<size_t>::iterator i = nodeStarts.begin();
	set<size_t>::iterator i2 = nodeStarts.begin();
	for (i2++; i2 != nodeStarts.end(); i++, i2++) {
		// Traverse all instructions in between [start_i: start_i+1)
		size_t end = getEndInstID(instLst, *i, *i2 - 1);
		nodeEnds.push_back(end);
	}
	// end for the last start
	size_t end = getEndInstID(instLst, *i, length - 1);
	nodeEnds.push_back(end);
}

void graph::genAllNodesGraph(vector<node>& gNodes, set<size_t>& nodeStarts, vector<size_t>& nodeEnds) {
	set<size_t>::iterator s = nodeStarts.begin();
	size_t e = 0;
	for (; s != nodeStarts.end(); s++, e++) {
		node nd(*s, nodeEnds[e]);
		gNodes.push_back(nd);
	}
}

// key: node._start (the start instruction ID) -> value: nodeId in gNodes
void graph::genIdMap(unsignedMap& idMap, vector<node>& gNodes) {
	for (size_t i = 0; i < gNodes.size(); i++) {
		idMap[gNodes[i]._start] = i;
	}
}

void graph::genAllEdgesGraph(vector<vector<unsigned int> >& gNodesOut, vector<node>& gNodes, inst* instLst) {
	for (size_t i = 0; i < gNodes.size(); i++) {
		gNodesOut.push_back(vector<unsigned int> {});
	}

	unsignedMap inId2GNdId;
	genIdMap(inId2GNdId, gNodes);

	for (size_t i = 0; i < gNodes.size(); i++) {
		size_t endInstId = gNodes[i]._end;
		vector <unsigned int> nextInstIds;
		if (getInstType(instLst[endInstId]) == CFG_OTHERS) {
			nextInstIds.push_back(endInstId + 1);
		}
		else if (getInstType(instLst[endInstId]) == CFG_CONDJMP) {
			nextInstIds.push_back(endInstId + 1);
			nextInstIds.push_back(endInstId + 1 + instLst[endInstId]._args[2]);
		}

		for (size_t j = 0; j < nextInstIds.size(); j++) {
			unsignedMap::iterator it = inId2GNdId.find(nextInstIds[j]);
			if (it != inId2GNdId.end()) {
				gNodesOut[i].push_back(it->second);
			}
		}
	}
}

// if next node has not been added into the graph, add this node
void graph::addNode(node& nd, unsigned int& added) {
	if (added == -1) {
		nodes.push_back(nd);
		nodesIn.push_back(vector<unsigned int> {});
		nodesOut.push_back(vector<unsigned int> {});
		added = nodes.size() - 1;
	}
}


void graph::dfs(size_t curgNodeId, vector<node>& gNodes, vector<vector<unsigned int> >& gNodesOut, \
                vector<unsigned int>& added, vector<bool>& visited, vector<bool>& finished) {
	if (finished[curgNodeId]) {
		return;
	}

	// 1 set curNodeId as visited
	visited[curgNodeId] = true;

	// 2 for each next node, check loop. loop exists if the visited[i] && !finished[i]
	for (size_t i = 0; i < gNodesOut[curgNodeId].size(); i++) {
		unsigned int nextgNodeId = gNodesOut[curgNodeId][i];
		if (visited[nextgNodeId] && (!finished[nextgNodeId])) {
			string errMsg  = "illegal input: loop from node " + \
			                 to_string(curgNodeId) + gNodes[curgNodeId].toStr() + \
			                 " to node " + \
			                 to_string(nextgNodeId) + gNodes[nextgNodeId].toStr();
			throw (errMsg);
		}
	}

	// 3 for each next node, update the graph G'
	unsigned int curNodeId = added[curgNodeId];
	for (size_t i = 0; i < gNodesOut[curgNodeId].size(); i++) {
		unsigned int nextgNodeId = gNodesOut[curgNodeId][i];
		// if next node has not been added into the graph, add this node
		addNode(gNodes[nextgNodeId], added[nextgNodeId]);
		// add edge into the graph
		unsigned int nextNodeId = added[nextgNodeId];
		nodesIn[nextNodeId].push_back(curNodeId);
		nodesOut[curNodeId].push_back(nextNodeId);
		// dfs nextNode
		try {
			dfs(nextgNodeId, gNodes, gNodesOut, added, visited, finished);
		}
		catch (const string errMsg) {
			throw errMsg;
		}
	}

	// 4 set curNodeId finished
	finished[curgNodeId] = true;
}

graph::graph(inst* instLst, int length) {
	// 1 generate node starts
	// set: keep order and ensure no repeated items
	set<size_t> nodeStarts;
	genNodeStarts(instLst, length, nodeStarts);

	// 2 generate node ends for each start in nodeStarts
	vector<size_t> nodeEnds;
	genNodeEnds(instLst, length, nodeStarts, nodeEnds);

	// 3 generate graph G containg all nodes and all edges
	vector<node> gNodes;
	vector<vector<unsigned int> > gNodesOut;
	// 3.1 add all nodes in G, that is gNodes
	genAllNodesGraph(gNodes, nodeStarts, nodeEnds);
	// 3.2 add all edges in G, that is gNodesOut;
	genAllEdgesGraph(gNodesOut, gNodes, instLst);

	// 4. generate connected sub-graph G' containing the first instruction from G by DFS
	// added[i] means the gNodes[i] has been added into the graph, and nodesId = added[gNodesId]
	vector<unsigned int> added(gNodes.size(), -1);
	// visited[i] = true means the node has been visited
	// used to check loop. loop exists if the visited[i] && !finished[i]
	vector<bool> visited(gNodes.size(), 0);
	// finished[i] = true means the node in nodesLst has finished DFS of gNodes[i]
	vector<bool> finished(gNodes.size(), 0);
	// add the root into the G'
	addNode(gNodes[0], added[0]);
	try {
		dfs(0, gNodes, gNodesOut, added, visited, finished);
	}
	catch (const string errMsg) {
		throw errMsg;
	}
}

graph::~graph() {}

ostream& operator<<(ostream& out, const graph& g) {
	out << endl << "nodes:" << endl << " ";
	for (size_t i = 0; i < g.nodes.size(); i++) {
		out << i << g.nodes[i] << " ";
	}
	out << endl << "edges:" << endl;
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