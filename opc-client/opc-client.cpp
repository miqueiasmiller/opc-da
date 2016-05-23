#include "opc-client.h"

OPCClient::~OPCClient()
{
	// Closes Microsoft COM library...
	CoUninitialize();
}

OPCClient::OPCClient(std::wstring const & serverName) : opcServerName(serverName), opcServer(nullptr)
{
	// The OPC DA Spec requires that some constants be registered in order to use
	// them. The one below refers to the OPC DA 1.0 IDataObject interface.
	this->opcDateTime = RegisterClipboardFormat(L"OPCSTMFORMATDATATIME");

	// Initializes Microsoft COM library...
	CoInitialize(NULL);

	// Gets an instance of the IOPCServer and assigns it to the instance variable...
	this->opcServer.reset(std::move(this->GetOPCServer(this->opcServerName)));
}

IOPCServer * OPCClient::GetOPCServer(std::wstring const & serverName)
{
	HRESULT hr;
	CLSID opcServerId;

	hr = CLSIDFromString(serverName.c_str(), &opcServerId);
	_ASSERT(!FAILED(hr));

	std::array<MULTI_QI, 1> instances = {{ &IID_IOPCServer, NULL, 0 }};

	hr = CoCreateInstanceEx(opcServerId, NULL, CLSCTX_SERVER, NULL, 1, &instances[0]);
	_ASSERT(!hr);

	//this->opcServer.reset(std::move((IOPCServer *)instances[0].pItf));
	return (IOPCServer *)instances[0].pItf;
}