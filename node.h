#ifndef NODE_H
#include "lib.h"
#include <limits>
enum class NodeType
{
	BEGIN,
	INPUT,
	AND,
	OR,
	NOT,
	END,
	UNKNOWN
};

class Node
{
public:
	string name;
	int asap = -1;
	NodeType type;
	vector<Node *> predecessor;
	vector<Node *> successor;
	int ready = 0;
	map<int,int> canWorkColNodeIndex;	// timeLevel / index columnNode[canWorkColNodeIndex]
	Node(NodeType type, string name);
};

#endif