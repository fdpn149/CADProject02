#include "manager.h"

Manager::Manager()
{
	beginNode = new Node(NodeType::BEGIN, "begin");
	endNode = new Node(NodeType::END, "end");
	nodes["begin"] = beginNode;
	nodes["end"] = endNode;

	glp_term_out(GLP_OFF);
	columnNode.push_back({});
}

Manager::~Manager()
{
	for (auto i : nodes)
		delete i.second;
	glp_delete_prob(lp);
}

int Manager::parseInput(char *_mode, char *filename, char *_and_limit, char *_or_limit, char *_not_limit)
{

	file = fopen(filename, "r");

	if (file == NULL)
	{
		printf("file could not open\n");
		return -1;
	}
	else
	{
		and_limit = atoi(_and_limit);
		or_limit = atoi(_or_limit);
		not_limit = atoi(_not_limit);
	}

	while (fgets(buffer, 1000, file) != NULL) // getLine until EOF
	{
		char *word = strtok(buffer, " \n");
		if (strcmp(word, ".names") == 0)
			getNames();
		else if (strcmp(word, ".model") == 0)
			continue;
		else if (strcmp(word, ".inputs") == 0)
			getInputs();
		else if (strcmp(word, ".outputs") == 0)
			getOutputs();
		else if (strcmp(word, ".end") == 0)
			break;
	}
	fclose(file);
	return 0;
}

int Manager::schedule()
{
	unordered_set<Node *> nextToDo;

	for (auto node : beginNode->successor)
	{
		for (auto child : node->successor)
		{
			child->ready++;
			child->asap = 1;
			nextToDo.insert(child);
		}
	}

	int time = 1;
	int and_count = 0, or_count = 0, not_count = 0;

	while (!(nextToDo.size() == 1 && (*nextToDo.begin())->type == NodeType::END))
	{
		if (time > and_total + or_total + not_total)
			return -1;
		unordered_set<Node *> toDo(nextToDo);
		unordered_set<Node *> toSetReady;
		vector<Node *> and_result_now, or_result_now, not_result_now;
		nextToDo.clear();
		for (auto node : toDo)
		{
			if (node->ready == node->predecessor.size())
			{
				switch (node->type)
				{
				case NodeType::AND:
					if (and_count < and_limit)
					{
						and_result_now.push_back(node);
						and_count++;
						toSetReady.insert(node);
					}
					else
						nextToDo.insert(node);
					break;
				case NodeType::OR:
					if (or_count < or_limit)
					{
						or_result_now.push_back(node);
						or_count++;
						toSetReady.insert(node);
					}
					else
						nextToDo.insert(node);
					break;
				case NodeType::NOT:
					if (not_count < not_limit)
					{
						not_result_now.push_back(node);
						not_count++;
						toSetReady.insert(node);
					}
					else
						nextToDo.insert(node);
					break;
				}
			}
			else
			{
				nextToDo.insert(node);
			}
		}

		for (auto node : toSetReady)
		{
			for (auto child : node->successor)
			{
				child->ready++;
				if (child->asap < node->asap + 1)
					child->asap = node->asap + 1;
				nextToDo.insert(child);
			}
		}
		and_result.push_back(and_result_now);
		or_result.push_back(or_result_now);
		not_result.push_back(not_result_now);
		and_count = or_count = not_count = 0;
		time++;
	}
	latency = time - 1;
	return 0;
}

void Manager::toNextLine(char *&word)
{
	fgets(buffer, 1000, file);
	word = strtok(buffer, " \n");
}

void Manager::getInputs()
{
	char *word;
	while ((word = strtok(NULL, " \n")) != NULL)
	{
		if (strcmp(word, "\\") == 0)
			toNextLine(word);
		Node *newInput = new Node(NodeType::INPUT, word);
		newInput->asap = 0;
		newInput->ready = 1;
		newInput->predecessor.push_back(beginNode);
		beginNode->successor.push_back(newInput);
		nodes[word] = newInput;
	}
}

void Manager::getOutputs()
{
	char *word;
	while ((word = strtok(NULL, " \n")) != NULL)
	{
		if (strcmp(word, "\\") == 0)
			toNextLine(word);
		Node *newOutput = new Node(NodeType::UNKNOWN, word);
		newOutput->successor.push_back(endNode);
		endNode->predecessor.push_back(newOutput);
		nodes[word] = newOutput;
		gateNode.push_back(newOutput);
	}
}

void Manager::getNames()
{
	char *word;
	vector<Node *> io;
	int literalCount = 0;
	while ((word = strtok(NULL, " \n")) != NULL) // read all inputs & output
	{
		literalCount++;
		if (nodes.find(word) != nodes.end())
			io.push_back(nodes[word]);
		else
		{
			Node *newNode = new Node(NodeType::UNKNOWN, word);
			nodes[word] = newNode;
			gateNode.push_back(newNode);
			io.push_back(newNode);
		}
	}

	Node *currentNode = *io.rbegin(); // get output

	if (literalCount == 2) // NOT
	{
		currentNode->type = NodeType::NOT;
		// nots.push_back(currentNode);
		not_total++;
		fgets(buffer, 1000, file); // eat next line
	}
	else
	{
		fgets(buffer, 1000, file); // eat next line
		bool isOR = false;
		char ch = getc(file); // check next char is '.' or not
		ungetc(ch, file);
		if (ch != '.')
		{
			isOR = true;
			for (int i = 0; i < literalCount - 2; i++) // eat lines
				fgets(buffer, 1000, file);
		}
		if (isOR)
		{
			currentNode->type = NodeType::OR;
			// ors.push_back(currentNode);
			or_total++;
		}
		else
		{
			currentNode->type = NodeType::AND;
			// ands.push_back(currentNode);
			and_total++;
		}
	}

	for (int i = 0; i < io.size() - 1; i++) // set input nodes
	{
		currentNode->predecessor.push_back(io.at(i));
		io.at(i)->successor.push_back(currentNode);
	}
}

void Manager::printResult()
{
	printf("Heuristic Scheduling Result\n");
	for (int i = 0; i < latency; i++)
	{
		printf("%d: {", i + 1);
		if (!and_result.empty() && !and_result.at(i).empty())
		{
			printf("%s", and_result.at(i).at(0)->name.c_str());
			for (int j = 1; j < and_result.at(i).size(); j++)
			{
				printf(" %s", and_result.at(i).at(j)->name.c_str());
			}
		}
		printf("} {");
		if (!or_result.empty() && !or_result.at(i).empty())
		{
			printf("%s", or_result.at(i).at(0)->name.c_str());
			for (int j = 1; j < or_result.at(i).size(); j++)
			{
				printf(" %s", or_result.at(i).at(j)->name.c_str());
			}
		}
		printf("} {");
		if (!not_result.empty() && !not_result.at(i).empty())
		{
			printf("%s", not_result.at(i).at(0)->name.c_str());
			for (int j = 1; j < not_result.at(i).size(); j++)
			{
				printf(" %s", not_result.at(i).at(j)->name.c_str());
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
		for (auto node_and : and_result[i])
		{
			for (int j = i + 1; j >= node_and->asap; j--)
			{
				slackTable[j][0].push_back(node_and);
				columnNode.push_back(make_pair(node_and, j));
				node_and->canWorkColNodeIndex[j] = columnNode.size() - 1;
				glp_add_cols(lp, 1);
				glp_set_col_kind(lp, columnNode.size() - 1, GLP_BV);
			}
		}
		for (auto node_or : or_result[i])
		{
			for (int j = i + 1; j >= node_or->asap; j--)
			{
				slackTable[j][1].push_back(node_or);
				columnNode.push_back(make_pair(node_or, j));
				node_or->canWorkColNodeIndex[j] = columnNode.size() - 1;
				glp_add_cols(lp, 1);
				glp_set_col_kind(lp, columnNode.size() - 1, GLP_BV);
			}
		}
		for (auto node_not : not_result[i])
		{
			for (int j = i + 1; j >= node_not->asap; j--)
			{
				slackTable[j][2].push_back(node_not);
				columnNode.push_back(make_pair(node_not, j));
				node_not->canWorkColNodeIndex[j] = columnNode.size() - 1;
				glp_add_cols(lp, 1);
				glp_set_col_kind(lp, columnNode.size() - 1, GLP_BV);
			}
		}
	}
	for (int j = latency + 1; j >= endNode->asap; j--)
	{
		slackTable[j][3].push_back(endNode);
		columnNode.push_back(make_pair(endNode, j));
		endNode->canWorkColNodeIndex[j] = columnNode.size() - 1;
		glp_add_cols(lp, 1);
		glp_set_col_kind(lp, columnNode.size() - 1, GLP_BV);
	}
}

void Manager::formulate(vector<array<vector<Node *>, 4>> &slackTable)
{
	glp_set_obj_dir(lp, GLP_MIN);

	vector<double> ar(1, 0.0);
	vector<int> ia(1, 0), ja(1, 0);

	vector<double> debug_row;
	vector<pair<string, int>> debug_col;

	int same = 0, order = 0, level = 0;
	for (const auto &node : gateNode)
	{
		// x2 + x1 = 1
		if (!node->canWorkColNodeIndex.empty())
		{
			total_row++;
			glp_add_rows(lp, 1);
			glp_set_row_bnds(lp, total_row, GLP_FX, 1.0, 1.0);
			debug_row.push_back(1.0);

			for (const auto &canWorkColIndex : node->canWorkColNodeIndex)
			{
				ia.push_back(total_row);
				ja.push_back(canWorkColIndex.second);
				ar.push_back(1.0);
			}
		}

		// 3x3 + 2x2 - 2y2 - 1y1 >= 1
		for (auto parentNode : node->successor)
		{
			if (!parentNode->canWorkColNodeIndex.empty() && !node->canWorkColNodeIndex.empty())
			{
				if (parentNode->canWorkColNodeIndex.begin()->first <= node->canWorkColNodeIndex.rbegin()->first)
				{
					total_row++;
					glp_add_rows(lp, 1);
					glp_set_row_bnds(lp, total_row, GLP_LO, 1.0, 0.0);
					debug_row.push_back(1.0);
					for (const auto &parentCanWorkColIndex : parentNode->canWorkColNodeIndex)
					{
						ia.push_back(total_row);
						ja.push_back(parentCanWorkColIndex.second);
						ar.push_back(columnNode[parentCanWorkColIndex.second].second);
					}
					for (const auto &canWorkColIndex : node->canWorkColNodeIndex)
					{
						ia.push_back(total_row);
						ja.push_back(canWorkColIndex.second);
						ar.push_back(-columnNode[canWorkColIndex.second].second);
					}
				}
			}
		}
	}

	// x2 + k2 + b2 <= 2
	for (int i = 1; i <= latency + 1; i++)
	{
		if (!slackTable[i][0].empty())
		{
			total_row++;
			glp_add_rows(lp, 1);
			glp_set_row_bnds(lp, total_row, GLP_UP, 0.0, (double)and_limit);
			debug_row.push_back((double)and_limit);
			for (const auto &node : slackTable[i][0])
			{
				ia.push_back(total_row);
				ja.push_back(node->canWorkColNodeIndex[i]);
				ar.push_back(1.0);
			}
		}

		if (!slackTable[i][1].empty())
		{
			total_row++;
			glp_add_rows(lp, 1);
			glp_set_row_bnds(lp, total_row, GLP_UP, 0.0, (double)or_limit);
			debug_row.push_back((double)or_limit);
			for (const auto &node : slackTable[i][1])
			{
				ia.push_back(total_row);
				ja.push_back(node->canWorkColNodeIndex[i]);
				ar.push_back(1.0);
			}
		}

		if (!slackTable[i][2].empty())
		{
			total_row++;
			glp_add_rows(lp, 1);
			glp_set_row_bnds(lp, total_row, GLP_UP, 0.0, (double)not_limit);
			debug_row.push_back((double)not_limit);
			for (const auto &node : slackTable[i][2])
			{
				ia.push_back(total_row);
				ja.push_back(node->canWorkColNodeIndex[i]);
				ar.push_back(1.0);
			}
		}

		if (!slackTable[i][3].empty())
		{
			total_row++;
			glp_add_rows(lp, 1);
			glp_set_row_bnds(lp, total_row, GLP_UP, 0.0, 1.0);
			debug_row.push_back(3);
			for (const auto &node : slackTable[i][3])
			{
				ia.push_back(total_row);
				ja.push_back(node->canWorkColNodeIndex[i]);
				ar.push_back(1.0);
			}
		}
	}

	// for (int i = 1; i < ia.size(); i++)
	// 	printf("ia[%d] ja[%d] ar[%d] =	%d	%s%d	%lf\n", i, i, i, ia[i], columnNode[ja[i]].first->name.c_str(), columnNode[ja[i]].second, ar[i]);

	for (const auto &nodeIndex : endNode->canWorkColNodeIndex)
	{
		glp_set_obj_coef(lp, nodeIndex.second, nodeIndex.first);
	}

	glp_load_matrix(lp, ar.size() - 1, &ia[0], &ja[0], &ar[0]);
}

void Manager::glp()
{
	lp = glp_create_prob();
	vector<array<vector<Node *>, 4>> slackTable;
	formSlackTable(slackTable);
	formulate(slackTable);
	glp_simplex(lp, NULL);
	glp_intopt(lp, NULL);

	// for (int i = 1; i <= 16; i++)
	// {
	// 	printf("%s%d \t %lf\n", columnNode[i].first->name.c_str(), columnNode[i].second, glp_mip_col_val(lp, i));
	// }
}

void Manager::printILPResult()
{
	vector<array<vector<string>, 3>> result;
	int total_columns = glp_get_num_cols(lp);
	for (int i = 1; i <= total_columns; i++)
	{
		if (glp_mip_col_val(lp, i) == 1.0)
		{
			auto nodeTime = columnNode[i];
			if (nodeTime.first->type != NodeType::END && result.size() < nodeTime.second + 1)
			{
				result.resize(nodeTime.second + 1);
			}
			switch (nodeTime.first->type)
			{
			case NodeType::AND:
				result[nodeTime.second][0].push_back(nodeTime.first->name);
				break;
			case NodeType::OR:
				result[nodeTime.second][1].push_back(nodeTime.first->name);
				break;
			case NodeType::NOT:
				result[nodeTime.second][2].push_back(nodeTime.first->name);
				break;
			}
		}
	}

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
