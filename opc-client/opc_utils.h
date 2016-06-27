#pragma once

#ifdef OPCCLIENT_EXPORTS
#define OPCCLIENT_API __declspec(dllexport)
#else
#define OPCCLIENT_API __declspec(dllimport)
#endif

#include <functional>
#include <string>
#include "opcda.h"

using namespace std;

namespace opc
{
  struct OPCCLIENT_API Group {
    OPCHANDLE handle;
    IOPCItemMgt * ptr;
    unsigned long updateRate;
  };

  struct OPCCLIENT_API ItemValue
  {
    OPCHANDLE handle;
    VARIANT value;
    DWORD quality;
  };

  struct OPCCLIENT_API ItemInfo
  {
    string id;
    OPCHANDLE handle;
    VARENUM dataType;
  };

  typedef function<void(string const &)> LogHandler;

  typedef function<void(vector<unique_ptr<ItemValue>> const &)> DataChangeHandler;

  static wchar_t * convertMBSToWCS(char const * value){
    size_t newSize = strlen(value) + 1;
    size_t convertedChars = 0;

    wchar_t * converted = new wchar_t[newSize];

    mbstowcs_s(&convertedChars, converted, newSize, value, _TRUNCATE);

    return converted;
  };
}