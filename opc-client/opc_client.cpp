#include "opc_client.h"

namespace opc
{
	OPCClient::~OPCClient()
	{
		// releases the Item Management Group...
		if (group.get())
			group->ptr->Release();

		// releases the OPC Server...
		if (opcServer)
			opcServer->Release();
	}

	OPCClient::OPCClient(wstring const & serverName) : opcServer(nullptr), logger([](string const &){})
	{
		// gets an instance of the IOPCServer and assigns it to the instance variable...
		opcServer = GetOPCServer(serverName);

		// gets an instance of the item management group...
		group = AddGroup(opcServer, 1000);
	}

	OPCClient::OPCClient(wstring const & serverName, LogFunction logFunc) : opcServer(nullptr), logger(logFunc)
	{
		// gets an instance of the IOPCServer and assigns it to the instance variable...
		logger(">> Initializing OPCServer.\r\n");
		opcServer = GetOPCServer(serverName);

		// gets an instance of the item management group...
		logger(">> Adding the Item Management Group to the server.\r\n");
		group = AddGroup(opcServer, 1000);
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


	unique_ptr<Group> OPCClient::AddGroup(IOPCServer * opcServer, unsigned long updateRate)
	{
		unique_ptr<Group> group = make_unique<Group>();

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

	HRESULT OPCClient::AddItem(wstring const & itemId, VARENUM type)
	{
		return AddItem(L"", itemId, type);
	}

	HRESULT OPCClient::AddItem(wstring const & accessPath, wstring const & itemId, VARENUM type)
	{
		// checks if an element with this key exists in the map...
		if (items.count(itemId) == 1)
		{
			return S_OK;
		}

		// item add result array.
		OPCITEMRESULT * result = nullptr;

		// item add errors array.
		HRESULT * errors = nullptr;

		OPCITEMDEF item{
			/*szAccessPath*/        const_cast<wchar_t*>(accessPath.c_str()),
			/*szItemID*/            const_cast<wchar_t*>(itemId.c_str()),
			/*bActive*/             true,
			/*hClient*/             1,
			/*dwBlobSize*/          0,
			/*pBlob*/               NULL,
			/*vtRequestedDataType*/ type,
			/*wReserved*/           0
		};

		// adds the items to the group.
		HRESULT hr = group->ptr->AddItems(1, &item, &result, &errors);

		char * msg;

		// if succeeds, adds the item to the map
		if (hr = S_OK)
		{
			// adds the item handle to the map...
			items.emplace(itemId, Item{ result[0].hServer, result[0].vtCanonicalDataType });

			sprintf(msg, ">> Item %s added to the group. Code: %x\r\n", itemId, hr);
			logger(string(msg));
		}
		else
		{
			sprintf(msg, ">> !!! An error occurred when trying to add items to the group. Error code: %x\r\n", hr);
			logger(string(msg));
		}

		// frees the memory allocated by the server.
		CoTaskMemFree(result->pBlob);
		CoTaskMemFree(result);
		result = nullptr;

		CoTaskMemFree(errors);
		errors = nullptr;

		return hr;
	}

	bool OPCClient::ReadItem(wstring const & itemId, VARIANT & output)
	{
		// checks if the itemId is already in the map. I yes, returns OK...
		if (items.count(itemId) == 0)
		{
			return false;
		}

		// gets the item's handle from the map...
		OPCHANDLE item = items[itemId].handle;

		// value of the item:
		OPCITEMSTATE * value = nullptr;

		// to store error code(s)
		HRESULT * errors = nullptr;

		//get a pointer to the IOPCSyncIOInterface:
		IOPCSyncIO * syncIO = nullptr;

		group->ptr->QueryInterface(__uuidof(syncIO), (void**)&syncIO);

		HRESULT hr = syncIO->Read(OPC_DS_DEVICE, 1, const_cast<OPCHANDLE*>(&item), &value, &errors);

		if (hr != S_OK || value == nullptr)
		{
			char * msg;

			sprintf(msg, ">> !! An error occurred when trying to add items to the group. Error code: %x\r\n", hr);
			logger(string(msg));

			return false;
		}

		output = value[0].vDataValue;

		//Release memeory allocated by the OPC server:
		CoTaskMemFree(errors);
		errors = nullptr;

		CoTaskMemFree(value);
		value = nullptr;

		// release the reference to the IOPCSyncIO interface:
		syncIO->Release();
		syncIO = nullptr;

		return true;
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