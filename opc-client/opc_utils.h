#pragma once

#ifdef OPCCLIENT_EXPORTS
#define OPCCLIENT_API __declspec(dllexport)
#else
#define OPCCLIENT_API __declspec(dllimport)
#endif

#include <comdef.h>
#include <comutil.h>
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

  inline bool operator ==(ItemValue const & lhs, ItemValue const & rhs)
  {
    return lhs.handle == rhs.handle;
  }


  struct OPCCLIENT_API ItemInfo
  {
    string id;
    OPCHANDLE handle;
    VARENUM dataType;
    int index;
  };

  inline bool operator ==(ItemInfo const & lhs, ItemInfo const & rhs)
  {
    return lhs.handle == rhs.handle && lhs.id == rhs.id;
  }


  typedef function<void(string const &)> LogHandler;


  typedef function<void(vector<unique_ptr<ItemValue>> const &)> DataChangeHandler;


  typedef function<ItemInfo(size_t)> GetItemInfoHandler;


  static wchar_t * convertMBSToWCS(char const * value){
    size_t newSize = strlen(value) + 1;
    size_t convertedChars = 0;

    wchar_t * converted = new wchar_t[newSize];

    mbstowcs_s(&convertedChars, converted, newSize, value, _TRUNCATE);

    return converted;
  };


  static string fromVARIANT(VARIANT & vr)
  {
    _bstr_t bt(vr);
    return string(static_cast<char *>(bt));
  }


  static void toVariant(string input, VARIANT & output)
  {
    _bstr_t bt(input.c_str());
    reinterpret_cast<_variant_t &>(output) = bt;
  }
}