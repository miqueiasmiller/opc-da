#pragma once

#ifdef OPCCLIENT_EXPORTS
#define OPCCLIENT_API __declspec(dllexport)
#else
#define OPCCLIENT_API __declspec(dllimport)
#endif

#include <string>
#include <array>
#include "opcda.h"

using namespace std;

namespace opc
{
	class OPCCLIENT_API OPCClient {
	protected:
		// a pointer to the IUnknown interface...
		IOPCServer * opcServer;

		// opc-da server name...
		wstring const & opcServerName;

		// retrieves an IUnknown instance of opc-da server...
		IOPCServer * GetOPCServer(wstring const & serverName);

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