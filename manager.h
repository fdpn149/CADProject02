#ifndef MANAGER_H
#include <algorithm>
#include "lib.h"
#include "node.h"
#include "glpk.h"
#include <map>

class Manager
{
	int and_limit, or_limit, not_limit;
	int and_total = 0, or_total = 0, not_total = 0;
	int latency;
	char buffer[1000];
	int total_row = 0;
	FILE *file;
	glp_prob *lp;
	Node *endNode;
	unordered_map<string, Node *> nodes;
	vector<Node *> gateNode;
	// vector<Node *> ands, ors, nots;
	vector<vector<Node *>> and_result, or_result, not_result;
	vector<pair<Node*, int>> columnNode;

	void toNextLine(char *&);
	void getInputs();
	void getOutputs();
	void getNames();
	void formSlackTable(vector<array<vector<Node *>,4>> &);
	void formulate(vector<array<vector<Node *>,4>> &);

public:
	Node *beginNode;
	Manager();
	~Manager();
	int parseInput(char *, char *, char *, char *);
	int schedule();
	void printResult();
	void glp();
	void printILPResult();
};
#endif
