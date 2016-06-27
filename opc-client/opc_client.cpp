#include "opc_client.h"

namespace opc
{
  OPCClient::~OPCClient()
  {
    Disconnect();
  }


  OPCClient::OPCClient() : opcServer(nullptr), logger([](string const &){}), dwCookie(0), connected(false)
  {
  }


  OPCClient::OPCClient(LogHandler logFunc) : opcServer(nullptr), logger(logFunc), dwCookie(0), connected(false)
  {
  }


  void OPCClient::Connect(string const & serverName)
  {
    if (connected)
      return;

    // gets an instance of the IOPCServer and assigns it to the instance variable...
    logger(">> Initializing OPCServer.\r\n");
    opcServer = GetOPCServer(serverName);

    // gets an instance of the item management group...
    logger(">> Adding the Item Management Group to the server.\r\n");
    group = AddGroup(opcServer, 1000);

    connected = true;
  }


  void OPCClient::Disconnect()
  {
    if (!connected)
      return;

    UnsetDataCallback();

    dwCookie = 0;

    // releases the connection pointer...
    if (connPoint)
      connPoint->Release();

    // releases the data callback object...
    if (dataCallback)
      dataCallback->Release();

    //TODO - Verificar a contagem de referencias do dataCallback

    // removes all added items...
    RemoveAllItems();

    // releases the Item Management Group and tries to remove the group...
    if (group)
    {
      group->ptr->Release();
      RemoveGroup(opcServer, *group);
    }

    // releases the OPC Server...
    if (opcServer)
    {
      opcServer->Release();
    }

    connected = false;
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


  HRESULT OPCClient::AddItem(string const & itemId, VARENUM type, ItemInfo & addedItem)
  {
    return AddItem("", itemId, type, addedItem);
  }


  HRESULT OPCClient::AddItem(string const & accessPath, string const & itemId, VARENUM type, ItemInfo & addedInfo)
  {
    // checks if an element with this key exists in the map...
    if (itemsById.count(itemId) == 1)
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
      // creates the item info...
      addedInfo.id = itemId;
      addedInfo.handle = result[0].hServer;
      addedInfo.dataType = (VARENUM)result[0].vtCanonicalDataType;

      // adds the item handle to the map...
      itemsById.emplace(itemId, addedInfo);

      msg << ">> Item " << itemId << " added to the group. Handler: " << addedInfo.handle << endl;
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


  HRESULT OPCClient::GetItemInfo(string const & itemId, ItemInfo & addedInfo)
  {
    if (itemsById.count(itemId) == 1)
    {
      addedInfo = itemsById[itemId];
      return S_OK;
    }

    return S_FALSE;
  }


  HRESULT OPCClient::RemoveAllItems()
  {
    HRESULT hr;
    ostringstream msg;

    auto it = itemsById.begin();

    for (auto it = itemsById.begin(); it != itemsById.end();)
    {
      hr = InternalRemoveItem(it->second.handle);

      if (hr == S_OK)
      {
        msg << ">> Item '" << it->second.id << "' was removed from the group. Handle: " << it->second.handle << endl;
        logger(msg.str());

        it = itemsById.erase(it);
      }
      else
      {
        msg << ">> !!! An error occurred while trying to remove the item '" << it->second.id << "'. Error code: " << hr << endl;
        logger(msg.str());

        ++it;
      }

      msg.clear();
      msg.str("");
    }

    return itemsById.empty() ? S_OK : S_FALSE;
  }


  HRESULT OPCClient::RemoveItem(ItemInfo const & item)
  {
    HRESULT hr;
    ostringstream msg;

    // checks if the itemId is already in the map. I yes, returns OK...
    if (itemsById.count(item.id) == 0)
    {
      msg << ">> The item '" << item.id << "' wasn't found in the added list." << endl;
      logger(msg.str());

      return S_FALSE;
    }

    ItemInfo toRemove = itemsById[item.id];

    if (toRemove.handle != item.handle)
    {
      msg << ">> The item's handle [" << item.handle << "] does no match the handle in the internal dictionary." << endl;
      logger(msg.str());

      return S_FALSE;
    }

    if (hr = InternalRemoveItem(toRemove.handle) == S_OK)
    {
      itemsById.erase(item.id);
    }
    else
    {
      msg << ">> !!! An error occurred while trying to remove the item '" << toRemove.id << "'. Error code: " << hr << endl;
      logger(msg.str());
    }

    msg << ">> Item '" << toRemove.id << "' was removed from the group. Handle: " << toRemove.handle << endl;
    logger(msg.str());
  }


  HRESULT OPCClient::InternalRemoveItem(OPCHANDLE const & handle)
  {
    HRESULT * errors;

    HRESULT hr = group->ptr->RemoveItems(1, const_cast<OPCHANDLE *>(&handle), &errors);

    //release memory allocated by the server...
    CoTaskMemFree(errors);
    errors = nullptr;

    return hr;
  }


  HRESULT OPCClient::Read(ItemInfo const & item, ItemValue & value)
  {
    ostringstream msg;

    // checks if the itemId is already in the map. I yes, returns OK...
    if (itemsById.count(item.id) == 0)
    {
      msg << ">> The item '" << item.id << "' wasn't found in the added list." << endl;
      logger(msg.str());

      return S_FALSE;
    }

    // gets the item in the internal dictionary...
    ItemInfo toRead = itemsById[item.id];

    if (toRead.handle != item.handle)
    {
      msg << ">> The item's handle [" << item.handle << "] doesn't match the handle of the item in the internal dictionary." << endl;
      logger(msg.str());

      return S_FALSE;
    }

    // value of the item:
    OPCITEMSTATE * readValue = nullptr;

    // to store error code(s)
    HRESULT * errors = nullptr;

    //get a pointer to the IOPCSyncIOInterface:
    IOPCSyncIO * syncIO = nullptr;

    group->ptr->QueryInterface(__uuidof(syncIO), (void**)&syncIO);

    HRESULT hr = syncIO->Read(OPC_DS_DEVICE, 1, const_cast<OPCHANDLE*>(&toRead.handle), &readValue, &errors);

    if (hr != S_OK || readValue == nullptr)
    {
      msg << ">> !! An error occurred while trying to read the item '" << item.id << "'. Error code: " << hr << endl;
      logger(msg.str());

      return hr;
    }

    value.handle = readValue[0].hClient;
    value.value = readValue[0].vDataValue;
    value.quality = readValue[0].wQuality;

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


  HRESULT OPCClient::Write(ItemInfo const & item, VARIANT & value)
  {
    ostringstream msg;

    // checks if the itemId is already in the map. I yes, returns OK...
    if (itemsById.count(item.id) == 0)
    {
      msg << ">> The item '" << item.id << "' wasn't found in the added list." << endl;
      logger(msg.str());

      return S_FALSE;
    }

    ItemInfo toWrite = itemsById[item.id];

    if (toWrite.handle != item.handle)
    {
      msg << ">> The item's handle [" << item.handle << "] doesn't match the handle of the item in the internal dictionary." << endl;
      logger(msg.str());

      return S_FALSE;
    }

    // to store error code(s)
    HRESULT * errors = nullptr;

    //get a pointer to the IOPCSyncIOInterface:
    IOPCSyncIO * syncIO = nullptr;

    group->ptr->QueryInterface(__uuidof(syncIO), (void**)&syncIO);

    HRESULT hr = syncIO->Write(1, const_cast<OPCHANDLE *>(&toWrite.handle), &value, &errors);

    if (hr != S_OK || &value == nullptr)
    {
      msg << ">> !! An error occurred while trying to write to the item '" << toWrite.id << "'. Error code: " << hr << endl;
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


  void OPCClient::OnDataChanged(vector<unique_ptr<ItemValue>> const & changedItems)
  {
    ostringstream msg;

    msg << "GOT IT!!! Callback called. Items changed: " << changedItems.size() << endl;
    logger(msg.str());
  }


  HRESULT OPCClient::SetDataCallback()
  {
    ostringstream msg;
    HRESULT hr;

    IConnectionPointContainer * cpc = nullptr;

    //Get a pointer to the IConnectionPointContainer interface:
    hr = group->ptr->QueryInterface(__uuidof(cpc), (void**)&cpc);
    if (hr != S_OK)
    {
      msg << ">> !!! Could not obtain a pointer to IConnectionPointContainer. Error: " << hr << endl;
      logger(msg.str());

      return hr;
    }


    // Call the IConnectionPointContainer::FindConnectionPoint method on the
    // group object to obtain a Connection Point
    IConnectionPoint * cp = connPoint.get();
    hr = cpc->FindConnectionPoint(IID_IOPCDataCallback, &cp);
    if (hr != S_OK)
    {
      msg << ">> !!! Failed call to FindConnectionPoint. Error: " << hr << endl;
      logger(msg.str());

      return hr;
    }


    // Now set up the Connection Point.
    dataCallback = make_unique<OPCDataCallback>(logger, bind(&OPCClient::OnDataChanged, this, placeholders::_1));
    dataCallback->AddRef();
    hr = connPoint->Advise(dataCallback.get(), &dwCookie);
    if (hr != S_OK)
    {
      msg << ">> !!! Failed call to IConnectionPoint::Advise.Error: " << hr << endl;
      logger(msg.str());

      return hr;
    }


    // From this point on we do not need anymore the pointer to the
    // IConnectionPointContainer interface, so release it
    cpc->Release();
    cpc = nullptr;
  }


  HRESULT OPCClient::UnsetDataCallback()
  {
    if (connPoint)
    {
      HRESULT hr;
      ostringstream msg;

      //call the IDataObject::DUnAdvise server method for cancelling the callback
      hr = connPoint->Unadvise(dwCookie);
      if (hr != S_OK)
      {
        msg << ">> !!! Failed call to IDataObject::UnAdvise. Error: " << hr << endl;
        logger(msg.str());

        //dwCookie = 0;
      }

      return hr;
    }

    return S_OK;
  }


  HRESULT OPCClient::SetGroupState(bool active)
  {
    HRESULT hr;
    ostringstream msg;
    IOPCGroupStateMgt * groupStateMgr;
    DWORD revisedUpdateRate;
    BOOL activeFlag = active;

    // Get a pointer to the IOPCGroupStateMgt interface:
    hr = group->ptr->QueryInterface(__uuidof(groupStateMgr), (void**)&groupStateMgr);
    if (hr != S_OK)
    {
      msg << ">> !!! Could not obtain a pointer to IOPCGroupStateMgt. Error: " << hr << endl;
      logger(msg.str());
      return hr;
    }

    // Set the state to Active. Since the other group properties are to remain
    // unchanged we pass NULL pointers to them as suggested by the OPC DA Spec.
    hr = groupStateMgr->SetState(
      NULL,                // *pRequestedUpdateRate
      &revisedUpdateRate,  // *pRevisedUpdateRate - can't be NULL
      &activeFlag,		     // *pActive
      NULL,				         // *pTimeBias
      NULL,				         // *pPercentDeadband
      NULL,				         // *pLCID
      NULL);				       // *phClientGroup

    if (hr != S_OK)
    {
      msg << ">> !!! Failed call to IOPCGroupMgt::SetState. Error: " << hr << endl;
      logger(msg.str());
    }

    // Free the pointer since we will not use it anymore.
    groupStateMgr->Release();
    groupStateMgr = nullptr;

    return hr;
  }


  void OPCClient::Initialize(void)
  {
    // Initializes Microsoft COM library...

    // Forces STA (single thread apartment)
    //CoInitialize(NULL);

    // Forces MTA (multi thread apartment)
    CoInitializeEx(NULL, COINIT_MULTITHREADED);
  }


  void OPCClient::Uninitialize(void)
  {
    // Closes Microsoft COM library...
    CoUninitialize();
  }
}