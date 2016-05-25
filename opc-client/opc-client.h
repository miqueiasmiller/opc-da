#pragma once

#ifdef OPCCLIENT_EXPORTS
#define OPCCLIENT_API __declspec(dllexport)
#else
#define OPCCLIENT_API __declspec(dllimport)
#endif

#include <string>
#include <memory>
#include <array>
#include "opcda.h"

// This class is exported from the opc-client.dll
class OPCCLIENT_API OPCClient {
protected:
	std::unique_ptr<IOPCServer> opcServer;
	std::wstring const & opcServerName;
	unsigned int opcDateTime;

	IOPCServer * GetOPCServer(std::wstring const & serverName);

public:
	// destructor...
	~OPCClient();

	// constructor...
	OPCClient(std::wstring const & serverName);
};