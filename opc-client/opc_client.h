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

using namespace std;

namespace opc
{
	typedef function<void(string const &)> LogFunction;

	struct OPCCLIENT_API Group {
		OPCHANDLE handle;
		IOPCItemMgt * ptr;
		unsigned long updateRate;
	};

	struct OPCCLIENT_API Item {
		string id;
		OPCHANDLE handle;
		VARTYPE dataType;
	};

	class OPCCLIENT_API OPCClient {
	private:
		// since this class doesn't support accessPath, this method is private...
		// adds an item to the group...
		HRESULT AddItem(string const & accessPath, string const & itemId, VARENUM type);

		HRESULT OPCClient::RemoveItem(Item const & item);

	protected:
		// a pointer to the IUnknown interface...
		IOPCServer * opcServer;

		// logger function...
		LogFunction logger;

		// only allows one group and this is the reference to it...
		unique_ptr<Group> group;

		// the items collection. All added items will have its handle stored here...
		unordered_map<string, Item> items;

		// retrieves an IUnknown instance of opc-da server...
		IOPCServer * GetOPCServer(string const & serverName);

		// creates a group...
		unique_ptr<Group> AddGroup(IOPCServer * opcServer, unsigned long updateRate);

		// removes a group...
		HRESULT RemoveGroup(IOPCServer * opcServer, Group const & group);

		// removes all added items from the group...
		HRESULT RemoveAllItems();

	public:
		// destructor...
		~OPCClient();

		// constructor...
		OPCClient(string const & serverName, LogFunction logger);

		// constructor...
		OPCClient(string const & serverName);

		// adds an item to the group...
		HRESULT AddItem(string const & itemId, VARENUM type);

		// removes an item from the group...
		HRESULT RemoveItem(string const & itemId);

		// reads the value of an item...
		HRESULT Read(string const & itemId, VARIANT & value);

		// writes the value of an item...
		HRESULT Write(string const & itemId, VARIANT & value);

		// initializes COM...
		static void Initialize(void);

		// uninitializes COM...
		static void Uninitialize(void);
	};
}