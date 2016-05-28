#pragma once

#ifdef OPCCLIENT_EXPORTS
#define OPCCLIENT_API __declspec(dllexport)
#else
#define OPCCLIENT_API __declspec(dllimport)
#endif

#include <string>
#include <array>
#include <memory>
#include "opcda.h"

using namespace std;

namespace opc
{
	struct OPCCLIENT_API ItemManagementGroup {
		OPCHANDLE handle;
		IOPCItemMgt * ptr;
		unsigned long updateRate;
	};

	class OPCCLIENT_API OPCClient {
	protected:
		// a pointer to the IUnknown interface...
		IOPCServer * opcServer;

		unique_ptr<ItemManagementGroup> itemMgt;

		// retrieves an IUnknown instance of opc-da server...
		IOPCServer * GetOPCServer(wstring const & serverName);

		// creates a item management group...
		unique_ptr<ItemManagementGroup> AddGroupItemManagement(IOPCServer * opcServer, unsigned long updateRate);

	public:
		// destructor...
		~OPCClient();

		// constructor...
		OPCClient(wstring const & serverName);

		// initializes COM...
		static void Initialize(void);

		// uninitializes COM...
		static void Uninitialize(void);
	};
}