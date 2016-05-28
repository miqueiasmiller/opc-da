#include "opc_client.h"

namespace opc
{
	OPCClient::~OPCClient()
	{
		// releases the Item Managemente Group...
		if (itemMgt.get())
			itemMgt->ptr->Release();

		// releases the OPC Server...
		if (opcServer)
			opcServer->Release();
	}


	OPCClient::OPCClient(wstring const & serverName) : opcServer(nullptr)
	{
		// gets an instance of the IOPCServer and assigns it to the instance variable...
		opcServer = GetOPCServer(serverName);

		// gets an instance of the managemente group...
		itemMgt = AddGroupItemManagement(opcServer, 1000);
	}


	IOPCServer * OPCClient::GetOPCServer(wstring const & serverName)
	{
		HRESULT hr;
		CLSID opcServerId;

		hr = CLSIDFromString(serverName.c_str(), &opcServerId);
		_ASSERT(hr == NOERROR);

		array<MULTI_QI, 1> instances = { { &IID_IOPCServer, NULL, 0 } };

		hr = CoCreateInstanceEx(opcServerId, NULL, CLSCTX_SERVER, NULL, 1, &instances[0]);
		_ASSERT(hr == S_OK);

		return (IOPCServer *)instances[0].pItf;
	}


	unique_ptr<ItemManagementGroup> OPCClient::AddGroupItemManagement(IOPCServer * opcServer, unsigned long updateRate)
	{
		//ItemManagementGroup * group = new ItemManagementGroup();
		unique_ptr<ItemManagementGroup> group(new ItemManagementGroup());

		HRESULT hr = opcServer->AddGroup(
			L"ItemManagementGroup",    // szName
			false, 			               // bActive
			updateRate, 			         // dwRequestedUpdateRate
			0, 											   // hClientGroup
			0, 											   // pTimeBias
			0, 											   // pPercentDeadband
			0,                         // dwLCID
			&group->handle, 				   // phServerGroup
			&group->updateRate, 		   // pRevisedUpdateRate
			IID_IOPCItemMgt, 				   // riid
			(IUnknown **)&group->ptr); // ppUnk

		_ASSERT(hr == S_OK);

		return group;
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