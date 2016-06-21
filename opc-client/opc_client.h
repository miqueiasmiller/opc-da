#pragma once

#ifdef OPCCLIENT_EXPORTS
#define OPCCLIENT_API __declspec(dllexport)
#else
#define OPCCLIENT_API __declspec(dllimport)
#endif

#include <string>
#include <array>
#include <memory>
#include <functional>
#include <vector>
#include <unordered_map>
#include "opcda.h"

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
		OPCHANDLE handle;
		VARTYPE dataType;
	};

	class OPCCLIENT_API OPCClient {
	private:
		// since this class doesn't support accessPath, this method is private...
		// adds an item to the group...
		HRESULT AddItem(wstring const & accessPath, wstring const & itemId, VARENUM type);

	protected:
		// a pointer to the IUnknown interface...
		IOPCServer * opcServer;

		// logger function...
		LogFunction logger;

		// only allows one group and this is the reference to it...
		unique_ptr<Group> group;

		// the items collection. All added items will have its handle stored here...
		unordered_map<wstring, Item> items;

		// retrieves an IUnknown instance of opc-da server...
		IOPCServer * GetOPCServer(wstring const & serverName);

		// creates a item management group...
		unique_ptr<Group> AddGroup(IOPCServer * opcServer, unsigned long updateRate);

	public:
		// destructor...
		~OPCClient();

		// constructor...
		OPCClient(wstring const & serverName, LogFunction logger);

		// constructor...
		OPCClient(wstring const & serverName);

		// adds an item to the group...
		HRESULT AddItem(wstring const & itemId, VARENUM type);

		// reads the value of an item...
		bool ReadItem(wstring const & itemId, VARIANT & output);

		// initializes COM...
		static void Initialize(void);

		// uninitializes COM...
		static void Uninitialize(void);
	};
}