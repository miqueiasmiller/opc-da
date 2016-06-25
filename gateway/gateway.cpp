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
		{
			//teste(3);
			unique_ptr<OPCClient> opc = make_unique<OPCClient>("ECA.OPCDAServer211", gatewayLog);

			VARIANT result;
			HRESULT hr;

			hr = opc->AddItem("TAG9", VARENUM::VT_I4);

			hr = opc->AddItem("TAG0", VARENUM::VT_I8);
			_ASSERT(!hr);
			VariantInit(&result);
			opc->Read("TAG0", result);

			hr = opc->Write("TAG9", result);

			VariantClear(&result);

			hr = opc->AddItem("TAG1", VARENUM::VT_I8);
			_ASSERT(!hr);
			VariantInit(&result);
			opc->Read("TAG1", result);

			hr = opc->Write("TAG9", result);

			VariantClear(&result);
			
			hr = opc->RemoveItem("TAG0");
			_ASSERT(!hr);
		}

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