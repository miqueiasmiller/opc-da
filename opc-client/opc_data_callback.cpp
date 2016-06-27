//
// C++ class to implement the OPC DA 2.0 IOPCDataCallback interface.
//
// Note that only the ::OnDataChangeMethod() is currently implemented
// here. This code is largely based on the Luiz T. S. Mendes - DELT/UFMG 
// sample client code.
//

#include "opc_data_callback.h"

namespace opc
{
  mutex OPCDataCallback::onDataChangeMtx;


  OPCDataCallback::OPCDataCallback(LogHandler logFunc, DataChangeHandler onDataChange) :
    logger(logFunc), refCounter(0), changeHandler(onDataChange)
  {
  }


  OPCDataCallback::~OPCDataCallback()
  {
  }


  HRESULT STDMETHODCALLTYPE OPCDataCallback::QueryInterface(REFIID riid, LPVOID *ppv)
  {
    // Validate the pointer
    if (ppv == nullptr)
      return E_POINTER;       // invalid pointer

    // Standard COM practice requires that we invalidate output arguments
    // if an error is encountered.  Let's assume an error for now and invalidate
    // ppInterface.  We will reset it to a valid interface pointer later if we
    // determine requested ID is valid:
    *ppv = nullptr;

    if (riid == IID_IUnknown)
    {
      *ppv = (IUnknown*)this;
    }
    else if (riid == IID_IOPCDataCallback)
    {
      *ppv = (IOPCDataCallback*)this;
    }
    else
    {
      return (E_NOINTERFACE); //unsupported interface
    }

    // Success: increment the reference counter.
    AddRef();

    return S_OK;
  }


  ULONG STDMETHODCALLTYPE OPCDataCallback::AddRef()
  {
    // Atomically increment the reference count and return the value.
    return InterlockedIncrement((volatile LONG *)&refCounter);
  }


  ULONG STDMETHODCALLTYPE OPCDataCallback::Release()
  {
    if (InterlockedDecrement((volatile LONG *)&refCounter) == 0)
    {
      delete this;
      return 0;
    }
    else
    {
      return refCounter;
    }
  }

  // OnDataChange method. This method is provided to handle notifications
  // from an OPC Group for exception based (unsolicited) data changes and
  // refreshes. Data for one or possibly more active items in the group
  // will be provided.
  //
  // Returns:
  //	HRESULT - 
  //		S_OK - Processing of advisement successful.
  //		E_INVALIDARG - One of the arguments was invalid.
  // **************************************************************************
  HRESULT STDMETHODCALLTYPE OPCDataCallback::OnDataChange(
    DWORD dwTransID,          // Zero for normal OnDataChange events, non-zero for Refreshes.
    OPCHANDLE hGroup,					// Client group handle.
    HRESULT hrMasterQuality,	// S_OK if all qualities are GOOD, otherwise S_FALSE.
    HRESULT hrMasterError,		// S_OK if all errors are S_OK, otherwise S_FALSE.
    DWORD dwCount,						// Number of items in the lists that follow.
    OPCHANDLE *phClientItems,	// Item client handles.
    VARIANT *pvValues,				// Item data.
    WORD *pwQualities,				// Item qualities.
    FILETIME *pftTimeStamps,	// Item timestamps.
    HRESULT *pErrors)					// Item errors.
  {
    lock_guard<mutex> lock(onDataChangeMtx);

    ostringstream msg;
    vector<unique_ptr<ItemValue>> itemValueList;

    if (dwCount == 0 || phClientItems == NULL || pvValues == NULL ||
      pwQualities == NULL || pftTimeStamps == NULL || pErrors == NULL)
    {
      msg << ">> !!! OPCDataCallback::OnDataChange: invalid arguments." << endl;
      logger(msg.str());

      return E_INVALIDARG;
    }

    for (DWORD dwItem = 0; dwItem < dwCount; dwItem++)
    {
      itemValueList.push_back(make_unique<ItemValue>(
        ItemValue{ phClientItems[dwItem], pvValues[dwItem], pwQualities[dwItem] & OPC_QUALITY_MASK }
      ));
    }

    msg << ">> Items were changed. Total: " << itemValueList.size() << endl;
    logger(msg.str());

    changeHandler(itemValueList);

    // Return "success" code. Note this does not mean that there were no 
    // errors reported by the OPC Server, only that we successfully processed
    // the callback.
    return S_OK;
  }


  // The remaining methods of IOPCDataCallback are not implemented here, so
  // we just use dummy functions that simply return S_OK.
  HRESULT STDMETHODCALLTYPE OPCDataCallback::OnReadComplete(
    DWORD dwTransID,
    OPCHANDLE hGroup,
    HRESULT hrMasterQuality,
    HRESULT hrMasterError,
    DWORD dwCount,
    OPCHANDLE *phClientItems,
    VARIANT *pvValues,
    WORD *pwQualities,
    FILETIME *pftTimeStamps,
    HRESULT *pErrors)
  {
    return S_OK;
  }


  HRESULT STDMETHODCALLTYPE OPCDataCallback::OnWriteComplete(
    DWORD dwTransID,
    OPCHANDLE hGroup,
    HRESULT hrMasterError,
    DWORD dwCount,
    OPCHANDLE *phClientItems,
    HRESULT *pErrors)
  {
    return S_OK;
  }


  HRESULT STDMETHODCALLTYPE OPCDataCallback::OnCancelComplete(
    DWORD dwTransID,
    OPCHANDLE hGroup)
  {
    return S_OK;
  }
}