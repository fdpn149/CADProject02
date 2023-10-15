#ifndef NODE_H
#include "lib.h"
#include <limits>

enum NodeType
{
	AND,
	OR,
	NOT,
	END,
	BEGIN,
	INPUT,
	UNKNOWN
};

class Node
{
public:
	string name;
	int asap = -1, alap = 2147483647;
	NodeType type;
	vector<Node *> predecessor;
	vector<Node *> successor;
	int ready = 0;
	map<int, int> canWorkColNodeIndex; // timeLevel / index columnNode[canWorkColNodeIndex]
	Node(NodeType type, string name);
	static bool sortFunc( Node* a,  Node* b);
};

#endif