#ifndef MANAGER_H
#include <algorithm>
#include "lib.h"
#include "node.h"
#include "glpk.h"
#include <map>

class Manager
{
	//const int AND = 0, OR = 1, NOT = 2, END = 3;	 // define represent value of and, or, not
	int gate_limit[4] = {0};						 // gate maximum number
	int gate_count_limit[3] = {0};					 // total number of gates from file
	int latency;									 // latency
	FILE *file;										 // input file ptr
	Node *beginNode, *endNode;						 // begin node & end node
	unordered_map<string, Node *> nodes;			 // record node name to node self
	vector<Node *> gateNode;						 // all gate node (and, or, not, end)
	vector<vector<Node *>> gate_result_heuristic[3]; // heuristic result of gates
	// vector<Node *> ands, ors, nots;
	glp_prob *lp;						  // glp probe
	int total_row = 0;					  // total glp rows
	vector<pair<Node *, int>> columnNode; // glp column node & can-work time

	void getInputs();												   // get file .inputs content
	void getOutputs();												   // get file .outputs content
	void getNames();												   // get file .names content
	void toNextLine(char *&word);									   // read next line
	int schedule();													   // heuristic way to solve
	void formSlackTable(vector<array<vector<Node *>, 4>> &slackTable); // form slack table
	void formulate(vector<array<vector<Node *>, 4>> &slackTable);	   // formulate functions

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
