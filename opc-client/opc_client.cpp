#include "opc_client.h"

namespace opc
{
	OPCClient::~OPCClient()
	{
		// Releases the OPC Server...
		if (this->opcServer)
			this->opcServer->Release();
	}


	OPCClient::OPCClient(wstring const & serverName) : opcServerName(serverName), opcServer(nullptr)
	{
		// Gets an instance of the IOPCServer and assigns it to the instance variable...
		this->opcServer = this->GetOPCServer(this->opcServerName);
	}


	IOPCServer * OPCClient::GetOPCServer(wstring const & serverName)
	{
		HRESULT hr;
		CLSID opcServerId;

		hr = CLSIDFromString(serverName.c_str(), &opcServerId);
		_ASSERT(hr == NOERROR);

		array<MULTI_QI, 1> instances = {{ &IID_IOPCServer, NULL, 0 }};

		hr = CoCreateInstanceEx(opcServerId, NULL, CLSCTX_SERVER, NULL, 1, &instances[0]);
		_ASSERT(hr == S_OK);

		//this->opcServer.reset(std::move((IOPCServer *)instances[0].pItf));
		return (IOPCServer *)instances[0].pItf;
	}


	void OPCClient::Initialize(void)
	{
		// Initializes Microsoft COM library...
		CoInitialize(NULL);
	}


	void OPCClient::Uninitialize(void)
	{
		// Closes Microsoft COM library...
		CoUninitialize();
	}
}