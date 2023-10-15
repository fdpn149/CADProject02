#include "node.h"

Node::Node(NodeType type, string name) : type(type), name(name)
{
}

bool Node::sortFunc(Node* a,  Node *b)
{
    if (a->asap < b->asap)
        return true;
    if (a->asap == b->asap && a->alap < b->alap)
        return true;
    return false;
}
