#include "manager.h"

int main(int argc, char *argv[])
{
	Manager manager;
	int code = manager.parseInput(argv[1], argv[2], argv[3], argv[4], argv[5]);
	if (code == -1)
	{
		printf("ERROR\n");
		return -1;
	}
	code = manager.heuristicSolve();
	if (code == -1)
	{
		printf("No Solution\n");
		return -1;
	}
	manager.printResult();
	// manager.ilpSolve();
	// manager.printILPResult();
	return 0;
}