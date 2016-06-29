#pragma once

#ifdef OPCCLIENT_EXPORTS
#define OPCCLIENT_API __declspec(dllexport)
#else
#define OPCCLIENT_API __declspec(dllimport)
#endif

#include <array>
#include <functional>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>
#include "opcda.h"
#include "opcerror.h"
#include "opc_utils.h"
#include "opc_data_callback.h"

using namespace std;

namespace opc
{
  // controls if the COM was initialized...
  bool comInitialized;

  class OPCCLIENT_API OPCClient {
  private:

    // a pointer to the IUnknown interface...
    IOPCServer * opcServer;

    // logger function...
    LogHandler logger;

    // data change function...
    DataChangeHandler dataChangeFunc;

    // only allows one group and this is the reference to it...
    unique_ptr<Group> group;

    // the items collection. All added items will have its handle stored here...
    // must update to use a multi indexed container like boost::multi_index
    unordered_map<string, ItemInfo> itemsById;
    vector<ItemInfo> itemsVector;

    // connection poiter used when monitoring changes on the items...
    unique_ptr<IConnectionPoint> connPoint;

    // dwCookie...
    DWORD dwCookie;

    // controls if the OPC client is connected to the OPC server...
    bool connected;

    // data callback class...
    unique_ptr<OPCDataCallback> dataCallback;
    //OPCDataCallback dataCallback;

    // retrieves an IUnknown instance of opc-da server...
    IOPCServer * GetOPCServer(string const & serverName);

    // creates a group...
    unique_ptr<Group> AddGroup(IOPCServer * opcServer, unsigned long updateRate);

    // removes a group...
    HRESULT RemoveGroup(IOPCServer * opcServer, Group const & group);

    // adds an item to the group...
    HRESULT AddItem(string const & accessPath, string const & itemId, VARENUM type, ItemInfo & addedInfo);

    // removes all added items from the group...
    HRESULT RemoveAllItems();

    // removes an item...
    HRESULT OPCClient::InternalRemoveItem(OPCHANDLE const & handle);

    // this functions is called every time when one or more items's values are changed... 
    //void OPCClient::OnDataChanged(vector<unique_ptr<ItemValue>> const & changedItems);

  public:
    // destructor...
    ~OPCClient();

    // constructor...
    OPCClient::OPCClient(LogHandler logFunc, DataChangeHandler dataChangeFunc);

    // constructor...
    OPCClient(LogHandler logger);

    // constructor...
    OPCClient();

    // connects to an OPCServer...
    void Connect(string const & serverName);

    // disconnects from the OPCServer...
    void Disconnect();

    // adds an item to the group...
    HRESULT AddItem(string const & itemId, VARENUM type, ItemInfo & addedItem);

    // removes an item from the group...
    HRESULT RemoveItem(ItemInfo const & item);

    // gets the item info...
    HRESULT GetItemInfo(string const & itemId, ItemInfo & addedInfo);

    // reads the value of an item...
    HRESULT Read(ItemInfo const & item, ItemValue & value);

    // writes the value of an item...
    HRESULT Write(ItemInfo const & item, VARIANT & value);

    // starts monitoring data changes...
    HRESULT SetDataCallback();

    // stops monitoring data changes...
    HRESULT UnsetDataCallback();

    // sets the group activated...
    HRESULT SetGroupState(bool active);

    // gets the item info based on index...
    ItemInfo GetItemInfoByIndex(size_t index);

    // initializes COM...
    static void Initialize(void);

    // uninitializes COM...
    static void Uninitialize(void);
  };
}