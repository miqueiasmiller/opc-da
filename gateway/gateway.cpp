#include <boost/thread.hpp>
#include "proxy-server.h"
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "targetver.h"
#include "opc_client.h"
#include "opc_utils.h"

using namespace opc;
using namespace std;

bool verboseEnable = false;

vector<unique_ptr<ItemValue>> actualValues;

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


void connectCommand(OPCClient & opc)
{
  string serverName;

  cout << "Enter the OPC Server name: ";
  getline(cin, serverName);

  opc.Connect(serverName);
}


void disconnectCommand(OPCClient & opc)
{
  string response;

  cout << "Enter (y) if you really wants to disconnect. ";
  getline(cin, response);

  if (response == "y")
    opc.Disconnect();
}


void addServer211Items(OPCClient & opc)
{
  ItemInfo info;
  string id;

  for (size_t i = 0; i < 10; i++)
  {
    id = "TAG" + to_string(i);
    opc.AddItem(id, VARENUM::VT_I4, info);
  }
}


VARIANT readItem(OPCClient & opc, string const & itemId)
{
  ItemInfo item;

  HRESULT hr = opc.GetItemInfo(itemId, item);

  if (hr == S_OK)
  {
    ItemValue value;
    VariantInit(&value.value);

    hr = opc.Read(item, value);

    if (hr == S_OK)
    {
      return value.value;
    }
  }
}


void readItem(OPCClient & opc, vector<string> const & tokens)
{
  if (tokens.size() < 2)
  {
    cout << "Invalid write command." << endl;
    return;
  }

  string id = tokens[1];

  VARIANT v = readItem(opc, id);

  string result = fromVARIANT(v);

  cout << result << endl;
}

boost::mutex readMtx;
string readItemProxyFn(OPCClient & opc, string const & itemId)
{
  boost::unique_lock<boost::mutex> lock(readMtx);

  VARIANT v = readItem(opc, itemId);

  return fromVARIANT(v);
}


HRESULT writeItem(OPCClient & opc, string const & itemId, VARIANT & value)
{
  ItemInfo item;

  HRESULT hr = opc.GetItemInfo(itemId, item);

  if (hr == S_OK)
  {
    hr = opc.Write(item, value);
  }

  return hr;
}


HRESULT writeItem(OPCClient & opc, vector<string> const & tokens)
{
  if (tokens.size() < 3)
  {
    cout << "Invalid write command." << endl;
    return S_FALSE;
  }

  string itemId = tokens[1];
  string value = tokens[2];

  VARIANT v;
  VariantInit(&v);
  //v.vt = VT_I4;
  //v.intVal = stoi(value);
  toVariant(value, v);

  HRESULT hr = writeItem(opc, itemId, v);

  if (hr == S_OK)
    cout << "Success" << endl;
  else
    cout << "Fail (" << hr << ")" << endl;
}


boost::mutex writeMtx;
bool writeItemProxyFn(OPCClient & opc, string const & itemId, string value)
{
  boost::unique_lock<boost::mutex> lock(writeMtx);

  VARIANT v;
  VariantInit(&v);
  toVariant(value, v);

  HRESULT hr = writeItem(opc, itemId, v);

  return hr == S_OK;
}


void openSocket(OPCClient & opc, vector<string> const & tokens)
{
  int maxThreads = 1;

  if (tokens.size() == 2)
    maxThreads = stoi(tokens[1]);
}


HRESULT groupManager(OPCClient & opc, vector<string> const & tokens)
{
  if (tokens.size() == 2)
  {
    string option = tokens[1];

    if (option == "activate")
      return opc.SetGroupState(true);

    if (option == "deactivate")
      return opc.SetGroupState(false);
  }

  return S_FALSE;
}


void commandLoop(OPCClient & opc)
{
  string cmd;
  vector<string> tokens;
  do
  {
    cout << "Enter a command: " << endl;
    getline(cin, cmd);

    istringstream iss(cmd);
    copy(istream_iterator<string>(iss), istream_iterator<string>(), back_inserter(tokens));

    if (tokens[0] == "connect")
      connectCommand(opc);
    else if (tokens[0] == "disconnect")
      disconnectCommand(opc);
    else if (tokens[0] == "add_211")
      addServer211Items(opc);
    else if (tokens[0] == "read")
      readItem(opc, tokens);
    else if (tokens[0] == "write")
      writeItem(opc, tokens);
    else if (tokens[0] == "group")
      groupManager(opc, tokens);
    else if (tokens[0] == "open_socket")
      openSocket(opc, tokens);


    tokens.clear();
  } while (cmd != "quit");
}


void dataChangeCallback(vector<unique_ptr<ItemValue>> const & data)
{
  vector<unique_ptr<ItemValue>> changed;

  for (auto d = data.begin(); d != data.end(); ++d)
  {
    //auto a = find(actualValues.begin(), actualValues.end(), *d);
    auto a = find_if(actualValues.begin(), actualValues.end(), [&](unique_ptr<ItemValue> const & obj) {
      return  obj->handle == (*d)->handle;
    });


    // if found in the actual values
    if (a != actualValues.end())
    {
      ULONG res = VarCmp(&(*a)->value, &(*d)->value, 0);

      if (res == VARCMP_GT || res == VARCMP_LT)
      {
        (*a)->value = (*d)->value;
      }
    }
    else
    {
      // its a new item
      changed.push_back(make_unique<ItemValue>(ItemValue{ (*d)->handle, (*d)->value, (*d)->quality }));
      actualValues.push_back(make_unique<ItemValue>(ItemValue{ (*d)->handle, (*d)->value, (*d)->quality }));
    }
  }
}


void initProxyAsync(function<string(string const & itemId)> readFn, function<bool(string const & itemId, string value)> writeFn)
{
  ProxyServer proxy(9002, readFn, writeFn);
  proxy.start();
}


//function<string(string const & itemId)> readFnHandler;

int main(int argc, char * argv[])
{
  setupOptions(argc, argv);

  gatewayLog("Initializing COM.\r\n");
  OPCClient::Initialize();

  try
  {
    {
      unique_ptr<OPCClient> opc = make_unique<OPCClient>(gatewayLog, dataChangeCallback);

      function<string(string const & itemId)> readFnHandler = [&](string const & itemId) -> string { return readItemProxyFn(*opc, itemId); };

      function<bool(string const & itemId, string value)> writeFnHandler = [&](string const & itemId, string value) -> bool { return writeItemProxyFn(*opc, itemId, value); };

      boost::thread proxyThread(&initProxyAsync, readFnHandler, writeFnHandler);

      commandLoop(*opc);

      proxyThread.interrupt();
    }

    gatewayLog("Uninitializing COM.\r\n");
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