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


void connectCommand(OPCClient & opc)
{
  string serverName;

  cout << "Enter the OPC Server name: ";
  cin >> serverName;

  opc.Connect(serverName);
}


void disconnectCommand(OPCClient & opc)
{
  string response;

  cout << "Enter (y) if you really wants to disconnect. ";
  cin >> response;

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


void readItem(OPCClient & opc, vector<string> const & tokens)
{
  if (tokens.size() < 2)
  {
    cout << "Invalid write command." << endl;
    return;
  }

  ItemInfo item;
  string id = tokens[1];

  HRESULT hr = opc.GetItemInfo(id, item);

  if (hr == S_OK)
  {
    ItemValue value;
    VariantInit(&value.value);

    hr = opc.Read(item, value);

    if (hr == S_OK)
    {
      cout << "Fazer o parser do valor lido." << endl;
    }
  }
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
  ItemInfo item;

  HRESULT hr = opc.GetItemInfo(itemId, item);

  if (hr == S_OK)
  {
    VARIANT v;
    v.intVal = stoi(value);

    hr = opc.Write(item, v);
  }
}


void openSocket(OPCClient const & opc, vector<string> const & tokens)
{
  int maxThreads = 1;

  if (tokens.size() == 2)
    maxThreads = stoi(tokens[1]);
}


void commandLoop(OPCClient & opc)
{
  string cmd;
  vector<string> tokens;
  do
  {
    cout << "Enter a command: ";
    cin >> cmd;

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
    else if (tokens[0] == "open_socket")
      openSocket(opc, tokens);


    tokens.clear();
  } while (cmd != "quit");
}


int main(int argc, char * argv[])
{
  setupOptions(argc, argv);

  gatewayLog("Initializing COM.\r\n");
  OPCClient::Initialize();

  try
  {
    unique_ptr<OPCClient> opc = make_unique<OPCClient>(gatewayLog);

    // criar ponto de conexão TCP e disparar uma nova thread a cada conexão

    commandLoop(*opc);

    // desconectar todos os clientes TCP

    // matar as threads filhas

    // finalizar

    /*{
      //teste(3);


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
      }*/

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