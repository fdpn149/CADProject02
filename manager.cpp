#include "manager.h"

Manager::Manager()
{
	// add begin & end node
	beginNode = new Node(NodeType::BEGIN, "begin");
	endNode = new Node(NodeType::END, "end");
	nodes["begin"] = beginNode;
	nodes["end"] = endNode;

	gateNode.push_back(endNode);

	columnNode.push_back({}); // let index 0 is empty (start from 1)
}

Manager::~Manager()
{
	// delete all nodes
	for (auto i : nodes)
		delete i.second;
}

int Manager::parseInput(char *_mode, char *filename, char *and_limit, char *or_limit, char *not_limit)
{
	char buffer[1000];
	file = fopen(filename, "r"); // open file & point to

	// can not open file
	if (file == NULL)
	{
		printf("file could not open\n");
		return -1;
	}
	else
	{
		// set and, or, not limit as input provide
		gate_limit[AND] = atoi(and_limit);
		gate_limit[OR] = atoi(or_limit);
		gate_limit[NOT] = atoi(not_limit);
		gate_limit[END] = 1;
	}
	int mode;
	if (strcmp(_mode, "-h") == 0)
		mode = 1;
	else if (strcmp(_mode, "-e") == 0)
		mode = 2;
	else
		return -1;

	// read file content
	while (fgets(buffer, 1000, file) != NULL) // getLine until EOF
	{
		char *word = strtok(buffer, " \n"); // get a word
		if (strcmp(word, ".names") == 0)
			getNames();
		else if (strcmp(word, ".model") == 0)
			continue;
		else if (strcmp(word, ".inputs") == 0)
			getInputs();
		else if (strcmp(word, ".outputs") == 0)
			getOutputs();
		else if (strcmp(word, ".end") == 0) // file end
			break;
	}
	fclose(file); // close file
	return mode;
}

void Manager::getInputs()
{
	char *word; // be used to store a word

	// read every input nodes
	while ((word = strtok(NULL, " \n")) != NULL) // read until new line
	{
		// judge "\\n" case
		if (strcmp(word, "\\") == 0)
			toNextLine(word); // catch next line content

		// add new input node & initialize
		Node *newInput = new Node(NodeType::INPUT, word);
		newInput->predecessor.push_back(beginNode);
		beginNode->successor.push_back(newInput);
		nodes[word] = newInput;
		newInput->asap = 0;
		newInput->ready = 1; // set ready number is 1
	}
}

void Manager::getOutputs()
{
	char *word; // be used to store a word

	// read every output nodes
	while ((word = strtok(NULL, " \n")) != NULL) // read until new line
	{
		// judge "\\n" case
		if (strcmp(word, "\\") == 0)
			toNextLine(word); // catch next line content

		// add new output node & initialize
		Node *newOutput = new Node(NodeType::UNKNOWN, word);
		newOutput->successor.push_back(endNode);
		endNode->predecessor.push_back(newOutput);
		nodes[word] = newOutput;
		gateNode.push_back(newOutput); // also push the 'gate' to gateNode vector
	}
}

void Manager::getNames()
{
	char buffer[1000];
	char *word;			  // be used to store a word
	vector<Node *> io;	  // store the gate's input & output nodes
	int literalCount = 0; // record how many pin the gate has

	// read all inputs & output node the gate has
	while ((word = strtok(NULL, " \n")) != NULL) // read until new line
	{
		literalCount++;
		auto matchNode_it = nodes.find(word); // find if the pin node is exist
		if (matchNode_it != nodes.end())
			io.push_back(matchNode_it->second);
		else
		{
			// create a new node
			Node *newNode = new Node(NodeType::UNKNOWN, word);
			nodes[word] = newNode;		 // store the newNode to nodes(map)
			gateNode.push_back(newNode); // store the newNode to gateNode(vector)
			io.push_back(newNode);
		}
	}
	Node *currentNode = *io.rbegin(); // get output node (the last one)

	// if only two pins, it must be a NOT
	if (literalCount == 2)
	{
		currentNode->type = NodeType::NOT;
		logicGates[NOT].push_back(currentNode);
		gate_count_limit[NOT]++;
		fgets(buffer, 1000, file); // eat the redundant line
	}
	else // it may be OR/AND
	{
		fgets(buffer, 1000, file); // eat next line (to check)
		bool isOR = false;		   // record is OR or not

		// peek next character
		char ch = getc(file); // check next char is '.' or not
		ungetc(ch, file);	  // put it back

		// if the content only has two more line, it must be OR
		if (ch != '.')
		{
			isOR = true;

			// eat redundant lines
			for (int i = 0; i < literalCount - 2; i++)
				fgets(buffer, 1000, file);
		}

		if (isOR)
		{
			currentNode->type = NodeType::OR;
			logicGates[OR].push_back(currentNode);
			gate_count_limit[OR]++;
		}
		else
		{
			currentNode->type = NodeType::AND;
			logicGates[AND].push_back(currentNode);
			gate_count_limit[AND]++;
		}
	}

	// set connect input nodes to self node
	for (int i = 0; i < io.size() - 1; i++)
	{
		currentNode->predecessor.push_back(io.at(i));
		io.at(i)->successor.push_back(currentNode);
	}
}

void Manager::toNextLine(char *&word)
{
	char buffer[1000];
	fgets(buffer, 1000, file);
	word = strtok(buffer, " \n");
}

void Manager::calculate_asap(Node *node)
{
	for (auto child_node : node->successor)
	{
		if (child_node->asap < node->asap + 1)
		{
			child_node->asap = node->asap + 1;
			if (child_node->type == NodeType::END && latency < child_node->asap)
				latency = child_node->asap;
			calculate_asap(child_node);
		}
	}
}

void Manager::calculate_alap(Node *node)
{
	for (auto parent_node : node->predecessor)
	{
		if (parent_node->alap > node->alap - 1)
		{
			parent_node->alap = node->alap - 1;
			calculate_alap(parent_node);
		}
	}
}

int Manager::heuristicSolve()
{
	for (auto node : beginNode->successor)
		calculate_asap(node);

	endNode->alap = latency;
	latency--;

	calculate_alap(endNode);

	int code = schedule();
	if (code == -1)
		return -1;
	return 0;
}

int Manager::schedule()
{
	for (int type = AND; type <= END; type++)
		if (gate_limit[type] == 0 && gate_count_limit[type] > 0)
			return -1;
	// sort all type nodes
	std::sort(logicGates[AND].begin(), logicGates[AND].end(), Node::sortFunc);
	std::sort(logicGates[OR].begin(), logicGates[OR].end(), Node::sortFunc);
	std::sort(logicGates[NOT].begin(), logicGates[NOT].end(), Node::sortFunc);

	vector<Node *> toSchedule[] = {logicGates[AND], logicGates[OR], logicGates[NOT]};

	set<Node *> nextToDo[3];

	// extract input nodes' successor
	for (const auto &inputNode : beginNode->successor)
		for (const auto &node : inputNode->successor)
		{
			node->ready++;
			nextToDo[node->type].insert(node);
		}
	int time = 1;
	// schedule
	for (; endNode->ready < endNode->predecessor.size(); time++)
	{
		set<Node *> toDo[3]{nextToDo[0], nextToDo[1], nextToDo[2]};
		vector<Node *> toSetReady;
		for (int type = AND; type <= NOT; type++)
			nextToDo[type].clear();

		for (int type = AND; type <= NOT; type++)
		{
			vector<Node *> result_now;
			if (!toDo[type].empty())
			{
				int count = 0;
				auto node_it = toSchedule[type].begin();
				while (node_it != toSchedule[type].end() && time >= (*node_it)->asap && count < gate_limit[type])
				{
					if ((*node_it)->ready == (*node_it)->predecessor.size() && toDo[type].find(*node_it) != toDo[type].end())
					{
						result_now.push_back(*node_it);
						count++;
						toDo[type].erase(*node_it);
						toSetReady.push_back(*node_it);
						// if ((*node_it)->name == "p")
						// 	printf("  ");
						node_it = toSchedule[type].erase(node_it);
					}
					else
					{
						nextToDo[type].insert(*node_it);
						node_it++;
					}
				}
				nextToDo[type].insert(toDo[type].begin(), toDo[type].end());
			}

			result_heuristic[type].push_back(result_now);
		}

		for (auto &node : toSetReady)
		{
			for (auto &child : node->successor)
			{
				child->ready++;
				nextToDo[child->type].insert(child);
			}
		}
	}

	latency = time - 1;
	alap_fix = latency - endNode->alap + 1;
	return 0;
}

void Manager::printResult()
{
	printf("Heuristic Scheduling Result\n");
	for (int i = 0; i < latency; i++)
	{
		printf("%d: {", i + 1);
		if (!result_heuristic[AND].empty() && !result_heuristic[AND].at(i).empty())
		{
			printf("%s", result_heuristic[AND].at(i).at(0)->name.c_str());
			for (int j = 1; j < result_heuristic[AND].at(i).size(); j++)
			{
				printf(" %s", result_heuristic[AND].at(i).at(j)->name.c_str());
			}
		}
		printf("} {");
		if (!result_heuristic[OR].empty() && !result_heuristic[OR].at(i).empty())
		{
			printf("%s", result_heuristic[OR].at(i).at(0)->name.c_str());
			for (int j = 1; j < result_heuristic[OR].at(i).size(); j++)
			{
				printf(" %s", result_heuristic[OR].at(i).at(j)->name.c_str());
			}
		}
		printf("} {");
		if (!result_heuristic[NOT].empty() && !result_heuristic[NOT].at(i).empty())
		{
			printf("%s", result_heuristic[NOT].at(i).at(0)->name.c_str());
			for (int j = 1; j < result_heuristic[NOT].at(i).size(); j++)
			{
				printf(" %s", result_heuristic[NOT].at(i).at(j)->name.c_str());
			}
		}
		printf("}\n");
	}
	printf("LATENCY: %d\nEND\n", latency);
}

void Manager::formSlackTable(vector<array<vector<Node *>, 4>> &slackTable)
{
	slackTable.resize(latency + 2);
	for (int i = 0; i < latency; i++)
	{
		for (int type = AND; type <= NOT; type++)
		{
			for (auto node : result_heuristic[type][i])
			{
				for (int j = node->alap + alap_fix; j >= node->asap; j--)
				{
					slackTable[j][type].push_back(node);
					columnNode.push_back(make_pair(node, j));
					node->canWorkColNodeIndex[j] = columnNode.size() - 1;
				}
			}
		}
	}
	for (int j = latency + 1; j >= endNode->asap; j--)
	{
		slackTable[j][END].push_back(endNode);
		columnNode.push_back(make_pair(endNode, j));
		endNode->canWorkColNodeIndex[j] = columnNode.size() - 1;
	}
}

int Manager::formulate(vector<array<vector<Node *>, 4>> &slackTable)
{
	vector<double> ar(1, 0.0);
	vector<int> ia(1, 0), ja(1, 0);
	vector<pair<int, int>> row_val(1, pair<int, int>()); //== => 0, >= => 1, <= => -1
	int same = 0, order = 0, level = 0;

	file = fopen("converted.lp", "w");
	if (file == NULL)
	{
		printf("file could not be written\n");
		return -1;
	}

	fprintf(file, "Min\n");
	for (const auto &nodeIndex : endNode->canWorkColNodeIndex)
	{
		if (nodeIndex != *endNode->canWorkColNodeIndex.begin())
			fprintf(file, " + ");
		fprintf(file, "end_%d", nodeIndex.first);
		// glp_set_obj_coef(lp, nodeIndex.second, nodeIndex.first);
	}

	fprintf(file, "\nSubject To\n");
	for (const auto &node : gateNode)
	{
		// x2 + x1 = 1
		if (!node->canWorkColNodeIndex.empty())
		{
			for (const auto &canWorkColIndex : node->canWorkColNodeIndex)
			{
				if (canWorkColIndex != *node->canWorkColNodeIndex.begin())
					fprintf(file, " + ");
				fprintf(file, "%s_%d", node->name.c_str(), canWorkColIndex.first);
			}
			// total_row++;
			// row_val.push_back(make_pair(0, 1));
			fprintf(file, " = 1\n");
		}

		// 3x3 + 2x2 - 2y2 - 1y1 >= 1
		for (auto parentNode : node->successor)
		{
			// if (!parentNode->canWorkColNodeIndex.empty() && !node->canWorkColNodeIndex.empty())
			// {
			if (parentNode->canWorkColNodeIndex.begin()->first <= node->canWorkColNodeIndex.rbegin()->first)
			{
				for (const auto &parentCanWorkColIndex : parentNode->canWorkColNodeIndex)
				{
					if (parentCanWorkColIndex != *parentNode->canWorkColNodeIndex.begin())
						fprintf(file, " + ");
					fprintf(file, "%d %s_%d", columnNode[parentCanWorkColIndex.second].second, parentNode->name.c_str(), parentCanWorkColIndex.first);
					// ia.push_back(total_row);
					// ja.push_back(parentCanWorkColIndex.second);
					// ar.push_back(columnNode[parentCanWorkColIndex.second].second);
				}
				for (const auto &canWorkColIndex : node->canWorkColNodeIndex)
				{
					// ia.push_back(total_row);
					// ja.push_back(canWorkColIndex.second);
					// ar.push_back(-columnNode[canWorkColIndex.second].second);

					fprintf(file, " - %d %s_%d", columnNode[canWorkColIndex.second].second, node->name.c_str(), canWorkColIndex.first);
				}
				// total_row++;
				// row_val.push_back(make_pair(1, 1));
				fprintf(file, " >= 1\n");
			}
			// }
		}
	}

	// x2 + k2 + b2 <= 2
	for (int i = 1; i <= latency + 1; i++)
	{
		for (int type = AND; type <= NOT; type++)
		{
			if (!slackTable[i][type].empty())
			{
				for (const auto &node : slackTable[i][type])
				{
					if (node != *slackTable[i][type].begin())
						fprintf(file, " + ");
					fprintf(file, "%s_%d", node->name.c_str(), i);
					// ia.push_back(total_row);
					// ja.push_back(node->canWorkColNodeIndex[i]);
					// ar.push_back(1.0);
				}
				// total_row++;
				// row_val.push_back(make_pair(-1, gate_limit[type]));
				fprintf(file, " <= %d\n", gate_limit[type]);
			}
		}
	}
	fprintf(file, "Binary\n");
	for (auto node_it = columnNode.begin() + 1; node_it != columnNode.end(); node_it++)
	{
		fprintf(file, "%s_%d\n", node_it->first->name.c_str(), node_it->second);
	}

	fprintf(file, "End\n");

	fclose(file);
	// writeFile(ar, ia, ja, row_val);

	// printf(" ");
	return 0;
}

void Manager::ilpSolve()
{
	vector<array<vector<Node *>, 4>> slackTable;
	formSlackTable(slackTable);
	formulate(slackTable);

	// glp_iocp param;
	// glp_init_iocp(&param);
	// param.presolve = GLP_ON;
	// glp_intopt(lp, &param);
}

void Manager::printILPResult()
{
	vector<array<vector<string>, 3>> result;
	// int total_columns = glp_get_num_cols(lp);
	// for (int i = 1; i <= total_columns; i++)
	// {
	// 	if (glp_mip_col_val(lp, i) == 1.0)
	// 	{
	// 		auto nodeTime = columnNode[i];
	// 		if (nodeTime.first->type != NodeType::END && result.size() < nodeTime.second + 1)
	// 			result.resize(nodeTime.second + 1);

	// 		if (nodeTime.first->type != NodeType::END)
	// 			result[nodeTime.second][nodeTime.first->type].push_back(nodeTime.first->name);
	// 	}
	// }

	latency = result.size() - 1;
	printf("ILP-based Scheduling Result\n");
	for (int i = 1; i <= latency; i++)
	{
		printf("%d: {", i);
		if (!result[i][0].empty())
		{
			printf("%s", result[i][0].at(0).c_str());
			for (int j = 1; j < result[i][0].size(); j++)
			{
				printf(" %s", result[i][0].at(j).c_str());
			}
		}
		printf("} {");
		if (!result[i][1].empty())
		{
			printf("%s", result[i][1].at(0).c_str());
			for (int j = 1; j < result[i][1].size(); j++)
			{
				printf(" %s", result[i][1].at(j).c_str());
			}
		}
		printf("} {");
		if (!result[i][2].empty())
		{
			printf("%s", result[i][2].at(0).c_str());
			for (int j = 1; j < result[i][2].size(); j++)
			{
				printf(" %s", result[i][2].at(j).c_str());
			}
		}
		printf("}\n");
	}
	printf("LATENCY: %d\nEND\n", latency);
}

int Manager::writeFile(const vector<double> &ar, const vector<int> &ia, const vector<int> &ja, const vector<pair<int, int>> &row_val)
{

	// for (int i = 1; i < ia.size(); i++)
	//	printf("ia[%d] ja[%d] ar[%d] =	%d	%s%d	%lf\n", i, i, i, ia[i], columnNode[ja[i]].first->name.c_str(), columnNode[ja[i]].second, ar[i]);

	vector<map<string, int, std::greater<string>>> result;
	result.resize(*ia.rbegin() + 1);

	for (int i = 1; i < ia.size(); i++)
	{
		result[ia[i]].insert(make_pair(columnNode[ja[i]].first->name + "_" + std::to_string(columnNode[ja[i]].second), ar[i]));
	}

	fprintf(file, "\nSubject To\n");
	for (int i = 1; i < result.size(); i++)
	{
		auto &element = *(result.begin() + i);
		for (const auto &item : element)
		{
			if (item.first != element.begin()->first)
			{
				if (item.second < 0)
					fprintf(file, " - ");
				else
					fprintf(file, " + ");
			}
			fprintf(file, "%d %s", item.second < 0 ? item.second * -1 : item.second, item.first.c_str());
		}
		switch (row_val[i].first)
		{
		case 0:
			fprintf(file, " = ");
			break;
		case 1:
			fprintf(file, " >= ");
			break;
		case -1:
			fprintf(file, " <= ");
		}
		fprintf(file, "%d", row_val[i].second);
		fprintf(file, "\n");
	}

	fprintf(file, "Binary\n");
	for (auto node_it = columnNode.begin() + 1; node_it != columnNode.end(); node_it++)
	{
		fprintf(file, "%s_%d\n", node_it->first->name.c_str(), node_it->second);
	}

	fprintf(file, "End\n");

	fclose(file);
	return 0;
}
