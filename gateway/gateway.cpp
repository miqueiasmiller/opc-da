#include "targetver.h"
#include "opc-client.h"
#include <memory>
#include <iostream>
//#include <stdio.h>

bool verboseEnable = false;

void setupOptions(int argc, char * argv[])
{
	if (argc > 0)
	{
		for (int i = 0; i < argc; i++)
		{
			if (argv[i] == "v" || argv[i] == "V")
				verboseEnable = true;
		}
	}
}

int main(int argc, char * argv[])
{
	std::unique_ptr<OPCClient> opcClient (new OPCClient(L"Matrikon.OPC.Simulation.1"));

	std::cout << "Teste";
	int a;
	std::cin >> a;

	return 0;
}