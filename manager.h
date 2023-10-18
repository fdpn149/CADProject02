#ifndef MANAGER_H
#include <algorithm>
#include "lib.h"
#include "node.h"
#include "./include/gurobi_c++.h"

class Manager
{
	int gate_limit[4] = {0};	   // gate maximum number
	int gate_count_limit[3] = {0}; // total number of gates from file
	int latency = -1;			   // latency
	int alap_fix = 0;
	FILE *file;									// input file ptr
	Node *beginNode, *endNode;					// begin node & end node
	map<string, Node *> nodes;					// record node name to node self
	vector<Node *> gateNode;					// all gate node (and, or, not, end)
	vector<vector<Node *>> result_heuristic[3]; // heuristic result of gates
	vector<Node *> logicGates[3];
	//glp_prob *lp;						  // glp probe
	int total_row = 0;					  // total glp rows
	vector<pair<Node *, int>> columnNode; // glp column node & can-work time
	map<string, GRBVar> variable;

	GRBEnv* env;
	GRBModel* model;
	

	void getInputs();			  // get file .inputs content
	void getOutputs();			  // get file .outputs content
	void getNames();			  // get file .names content
	void toNextLine(char *&word); // read next line
	int schedule();				  // heuristic way to solve
	void calculate_asap(Node *node);
	void calculate_alap(Node *node);
	void formSlackTable(vector<array<vector<Node *>, 4>> &slackTable); // form slack table
	void formulate(vector<array<vector<Node *>, 4>> &slackTable);	   // formulate functions

	void printSlackTable(const vector<double> &ar, const vector<int> &ia, const vector<int> &ja);

public:
	Manager();	// initialize
	~Manager(); // destroy
	int parseInput(char *_mode, char *filename, char *_and_limit,
				   char *_or_limit, char *_not_limit); // read input and store
	int heuristicSolve();							   // heuristic way to solve
	void printResult();								   // print heuristic result
	void ilpSolve();								   // ILP solve
	void printILPResult();							   // print ILP result
};
#endif
