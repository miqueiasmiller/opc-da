#include "targetver.h"
#include "opc_client.h"
#include <memory>
#include <string>
#include <iostream>
#include <vector>

using namespace opc;
using namespace std;

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

void teste(const unsigned int n)
{
	//std::unique_ptr<OPCClient> opcClient (new OPCClient(L"Matrikon.OPC.Simulation.1"));
	vector<unique_ptr<OPCClient>> v;

	for (unsigned int i = 0; i < n; i++)
	{
		v.push_back(unique_ptr<OPCClient>(new OPCClient(L"Matrikon.OPC.Simulation.1")));
	}
}

void log(const string & msg)
{
	if (verboseEnable)
		cout << msg << endl;
}

int main(int argc, char * argv[])
{
	log("Initializing COM.");
	OPCClient::Initialize();

	try
	{
		teste(1);

		log("Uninitializing COM.");
		OPCClient::Uninitialize();
	}
	catch (exception & e)
	{
		log ("An exception occurred: " + string(e.what()));

		log("Uninitializing COM.");
		OPCClient::Uninitialize();
	}

	return 0;
}