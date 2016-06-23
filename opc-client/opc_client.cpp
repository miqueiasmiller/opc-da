#include "opc_client.h"

namespace opc
{
	OPCClient::~OPCClient()
	{
		// removes all added items...
		RemoveAllItems();

		// releases the Item Management Group and tries to remove the group...
		if (group.get())
		{
			group->ptr->Release();
			RemoveGroup(opcServer, *group.get());
		}

		// releases the OPC Server...
		if (opcServer)
		{
			opcServer->Release();
		}
	}


	OPCClient::OPCClient(string const & serverName) : opcServer(nullptr), logger([](string const &){})
	{
		// gets an instance of the IOPCServer and assigns it to the instance variable...
		opcServer = GetOPCServer(serverName);

		// gets an instance of the item management group...
		group = AddGroup(opcServer, 1000);
	}


	OPCClient::OPCClient(string const & serverName, LogFunction logFunc) : opcServer(nullptr), logger(logFunc)
	{
		// gets an instance of the IOPCServer and assigns it to the instance variable...
		logger(">> Initializing OPCServer.\r\n");
		opcServer = GetOPCServer(serverName);

		// gets an instance of the item management group...
		logger(">> Adding the Item Management Group to the server.\r\n");
		group = AddGroup(opcServer, 1000);
	}


	IOPCServer * OPCClient::GetOPCServer(string const & serverName)
	{
		HRESULT hr;
		CLSID opcServerId;

		wchar_t * sn = convertMBSToWCS(serverName.c_str());

		hr = CLSIDFromString(sn, &opcServerId);
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


	HRESULT OPCClient::RemoveGroup(IOPCServer * opcServer, Group const & group)
	{
		// Remove the group...
		HRESULT hr = opcServer->RemoveGroup(group.handle, false);

		if (hr != S_OK)
		{
			ostringstream msg;

			if (hr == OPC_S_INUSE)
			{
				msg << ">> !!! Failed to remove OPC group: object still has references to it." << endl;
			}
			else
			{
				msg << ">> !!! Failed to remove OPC group. Error code: " << hr << endl;
			}

			logger(msg.str());
		}

		return hr;
	}


	HRESULT OPCClient::AddItem(string const & itemId, VARENUM type)
	{
		return AddItem("", itemId, type);
	}


	HRESULT OPCClient::AddItem(string const & accessPath, string const & itemId, VARENUM type)
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

		wchar_t * ap = convertMBSToWCS(accessPath.c_str());
		wchar_t * id = convertMBSToWCS(itemId.c_str());

		OPCITEMDEF item{
			/*szAccessPath*/        ap,
			/*szItemID*/            id,
			/*bActive*/             true,
			/*hClient*/             1,
			/*dwBlobSize*/          0,
			/*pBlob*/               NULL,
			/*vtRequestedDataType*/ type,
			/*wReserved*/           0
		};

		// adds the items to the group.
		HRESULT hr = group->ptr->AddItems(1, &item, &result, &errors);

		ostringstream msg;

		// if succeeds, adds the item to the map
		if (hr == S_OK)
		{
			// adds the item handle to the map...
			items.emplace(itemId, Item{ itemId, result[0].hServer, result[0].vtCanonicalDataType });
			msg << ">> Item " << itemId << " added to the group. Code: " << hr << endl;
			//sprintf(msg, ">> Item %s added to the group. Code: %x\r\n", itemId, hr);
			logger(msg.str());
		}
		else
		{
			msg << ">> !!! An error occurred while trying to add the item '" << itemId << "' to the group. Error code: " << hr << endl;
			logger(msg.str());
		}

		// frees the memory allocated by the server.
		CoTaskMemFree(result->pBlob);
		CoTaskMemFree(result);
		result = nullptr;

		CoTaskMemFree(errors);
		errors = nullptr;

		return hr;
	}


	HRESULT OPCClient::RemoveAllItems()
	{
		HRESULT hr;

		auto it = items.begin();

		while (it != items.end())
		{
			hr = RemoveItem(it->second);

			if (hr == S_OK)
			{
				it = items.erase(it);
			}
			else
			{
				++it;
			}
		}

		return items.empty() ? S_OK : S_FALSE;
	}


	HRESULT OPCClient::RemoveItem(string const & itemId)
	{
		ostringstream msg;

		// checks if the itemId is already in the map. I yes, returns OK...
		if (items.count(itemId) == 0)
		{
			msg << ">> The item '" << itemId.c_str() << "' wasn't found in the added list." << endl;
			logger(msg.str());

			return S_FALSE;
		}

		Item toRemove = items[itemId];

		if (RemoveItem(toRemove) == S_OK)
		{
			items.erase(itemId);
		}

		msg << ">> The item '" << itemId << "' was removed from the group." << endl;
		logger(msg.str());
	}


	HRESULT OPCClient::RemoveItem(Item const & item)
	{
		HRESULT * errors;

		HRESULT hr = group->ptr->RemoveItems(1, const_cast<OPCHANDLE *>(&item.handle), &errors);

		if (hr != S_OK)
		{
			ostringstream msg;

			msg << ">> !!! An error occurred while trying to remove the item '" << item.id << "'. Error code: " << hr << endl;
			logger(msg.str());
		}

		//release memory allocated by the server...
		CoTaskMemFree(errors);
		errors = nullptr;

		return hr;
	}


	HRESULT OPCClient::Read(string const & itemId, VARIANT & value)
	{
		ostringstream msg;

		// checks if the itemId is already in the map. I yes, returns OK...
		if (items.count(itemId) == 0)
		{
			msg << ">> The item '" << itemId.c_str() << "' wasn't found in the added list." << endl;
			logger(msg.str());

			return S_FALSE;
		}

		// gets the item's handle from the map...
		OPCHANDLE item = items[itemId].handle;

		// value of the item:
		OPCITEMSTATE * readValue = nullptr;

		// to store error code(s)
		HRESULT * errors = nullptr;

		//get a pointer to the IOPCSyncIOInterface:
		IOPCSyncIO * syncIO = nullptr;

		group->ptr->QueryInterface(__uuidof(syncIO), (void**)&syncIO);

		HRESULT hr = syncIO->Read(OPC_DS_DEVICE, 1, const_cast<OPCHANDLE*>(&item), &readValue, &errors);

		if (hr != S_OK || readValue == nullptr)
		{
			msg << ">> !! An error occurred while trying to read the item '" << itemId.c_str() << "'. Error code: " << hr << endl;
			logger(msg.str());

			return hr;
		}

		value = readValue[0].vDataValue;

		//Release memeory allocated by the OPC server:
		CoTaskMemFree(errors);
		errors = nullptr;

		CoTaskMemFree(readValue);
		readValue = nullptr;

		// release the reference to the IOPCSyncIO interface:
		syncIO->Release();
		syncIO = nullptr;

		return hr;
	}


	HRESULT OPCClient::Write(string const & itemId, VARIANT & value)
	{
		ostringstream msg;

		// checks if the itemId is already in the map. I yes, returns OK...
		if (items.count(itemId) == 0)
		{
			msg << ">> The item '" << itemId.c_str() << "' wasn't found in the added list." << endl;
			logger(msg.str());

			return S_FALSE;
		}

		// gets the item's handle from the map...
		OPCHANDLE item = items[itemId].handle;

		// to store error code(s)
		HRESULT * errors = nullptr;

		//get a pointer to the IOPCSyncIOInterface:
		IOPCSyncIO * syncIO = nullptr;

		group->ptr->QueryInterface(__uuidof(syncIO), (void**)&syncIO);

		HRESULT hr = syncIO->Write(1, const_cast<OPCHANDLE *>(&item), &value, &errors);

		if (hr != S_OK || &value == nullptr)
		{
			msg << ">> !! An error occurred while trying to write to the item '" << itemId.c_str() << "'. Error code: " << hr << endl;
			logger(msg.str());

			return hr;
		}

		//Release memeory allocated by the OPC server:
		CoTaskMemFree(errors);
		errors = nullptr;

		// release the reference to the IOPCSyncIO interface:
		syncIO->Release();
		syncIO = nullptr;

		return hr;
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