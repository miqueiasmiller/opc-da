//
// C++ class to implement the OPC DA 2.0 IOPCDataCallback interface.
//
// Note that only the ::OnDataChangeMethod() is currently implemented
// here. This code is largely based on the Luiz T. S. Mendes - DELT/UFMG 
// sample client code.
//
#pragma once

#include <memory>
#include <mutex>
#include <sstream>
#include <vector>
#include "opcda.h"
#include "opc_utils.h"

using namespace std;

namespace opc
{
  class OPCDataCallback : public IOPCDataCallback
  {
  private:
    DWORD refCounter;
    LogHandler logger;
    DataChangeHandler changeHandler;
    GetItemInfoHandler getItemInfo;

    static mutex onDataChangeMtx;

  public:
    OPCDataCallback(LogHandler logFunc, DataChangeHandler onDataChange, GetItemInfoHandler itemInfoHandler);
    ~OPCDataCallback();

    DWORD getCountRef();

    /* IUnknown Methods */

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, LPVOID *ppv);
    ULONG STDMETHODCALLTYPE AddRef();
    ULONG STDMETHODCALLTYPE Release();

    /* IOPCDataCallback Methods */

    // OnDataChange notifications
    HRESULT STDMETHODCALLTYPE OnDataChange(
      DWORD dwTransID,			    // 0 for normal OnDataChange events, non-zero for Refreshes
      OPCHANDLE hGroup,			    // client group handle
      HRESULT hrMasterQuality,	// S_OK if all qualities are GOOD, otherwise S_FALSE
      HRESULT hrMasterError,		// S_OK if all errors are S_OK, otherwise S_FALSE
      DWORD dwCount,				    // number of items in the lists that follow
      OPCHANDLE *phClientItems,	// item client handles
      VARIANT *pvValues,			  // item data
      WORD *pwQualities,			  // item qualities
      FILETIME *pftTimeStamps,	// item timestamps
      HRESULT *pErrors);			  // item errors	

    // OnReadComplete notifications
    HRESULT STDMETHODCALLTYPE OnReadComplete(
      DWORD dwTransID,			    // Transaction ID returned by the server when the read was initiated
      OPCHANDLE hGroup,			    // client group handle
      HRESULT hrMasterQuality,	// S_OK if all qualities are GOOD, otherwise S_FALSE
      HRESULT hrMasterError,		// S_OK if all errors are S_OK, otherwise S_FALSE
      DWORD dwCount,				    // number of items in the lists that follow
      OPCHANDLE *phClientItems,	// item client handles
      VARIANT *pvValues,			  // item data
      WORD *pwQualities,			  // item qualities
      FILETIME *pftTimeStamps,	// item timestamps
      HRESULT *pErrors);			  // item errors	

    // OnWriteComplete notifications
    HRESULT STDMETHODCALLTYPE OnWriteComplete(
      DWORD dwTransID,			    // Transaction ID returned by the server when the write was initiated
      OPCHANDLE hGroup,			    // client group handle
      HRESULT hrMasterError,	  // S_OK if all errors are S_OK, otherwise S_FALSE
      DWORD dwCount,				    // number of items in the lists that follow
      OPCHANDLE *phClientItems,	// item client handles
      HRESULT *pErrors);			  // item errors	

    // OnCancelComplete notifications
    HRESULT STDMETHODCALLTYPE OnCancelComplete(
      DWORD dwTransID,		      // Transaction ID provided by the client when the read/write/refresh was initiated
      OPCHANDLE hGroup);        // client group handle
  };
}