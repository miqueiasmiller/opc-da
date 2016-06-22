#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "targetver.h"
#include "opc_client.h"

using namespace opc;
using namespace std;

bool verboseEnable = false;

void setupOptions(int argc, char * argv[])
{
	if (argc > 0)
	{
		for (int i = 1; i < argc; i++)
		{
			if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "-V") == 0)
				verboseEnable = true;
		}
	}
}

void teste(const unsigned int n)
{
	//unique_ptr<OPCClient> opcClient (new OPCClient(L"Matrikon.OPC.Simulation.1"));
	vector<unique_ptr<OPCClient>> v;

	for (unsigned int i = 0; i < n; i++)
	{
		//v.push_back(unique_ptr<OPCClient>(new OPCClient(L"Matrikon.OPC.Simulation.1")));
		v.push_back(unique_ptr<OPCClient>(new OPCClient(L"ECA.OPCDAServer211")));
	}
}

void gatewayLog(const string & msg)
{
	if (verboseEnable)
		cout << msg;
}

int main(int argc, char * argv[])
{
	setupOptions(argc, argv);

	gatewayLog("Initializing COM.\r\n");
	OPCClient::Initialize();

	try
	{
		//teste(3);
		unique_ptr<OPCClient> opc = make_unique<OPCClient>(L"ECA.OPCDAServer211", gatewayLog);

		HRESULT hr = opc->AddItem(L"TAG0", VARENUM::VT_I8);

		_ASSERT(!hr);

		VARIANT result;
		VariantInit(&result);

		opc->ReadItem(L"TAG0", result);

		long a = result.lVal;

		gatewayLog("Uninitializing COM.");
		OPCClient::Uninitialize();
	}
	catch (exception & e)
	{
		gatewayLog("An exception occurred: " + string(e.what()));

		gatewayLog("Uninitializing COM.\r\n");
		OPCClient::Uninitialize();
	}

	return 0;
}