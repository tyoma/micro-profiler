#pragma once

#include <windows.h>

struct IVsTask;
struct IVsTaskBody;
struct IVsTaskCompletionSource;

enum __VSTASKRUNCONTEXT {
	VSTC_BACKGROUNDTHREAD = 0,
	VSTC_UITHREAD_SEND = 1,
	VSTC_UITHREAD_BACKGROUND_PRIORITY = 2,
	VSTC_UITHREAD_IDLE_PRIORITY = 3,
	VSTC_CURRENTCONTEXT = 4,
	VSTC_BACKGROUNDTHREAD_LOW_IO_PRIORITY = 5,
	VSTC_UITHREAD_NORMAL_PRIORITY = 6
} ;
typedef DWORD VSTASKRUNCONTEXT;

enum __VSTASKWAITOPTIONS {
	VSTWO_None = 0,
	VSTWO_AbortOnTaskCancellation = 0x1
};
typedef DWORD VSTASKWAITOPTIONS;

enum __VSTASKCONTINUATIONOPTIONS {
	VSTCO_None = 0,
	VSTCO_PreferFairness = 1,
	VSTCO_LongRunning = 2,
	VSTCO_AttachedToParent = 4,
	VSTCO_DenyChildAttach = 8,
	VSTCO_LazyCancelation = 32,
	VSTCO_NotOnRanToCompletion = 0x10000,
	VSTCO_NotOnFaulted = 0x20000,
	VSTCO_OnlyOnCanceled = 0x30000,
	VSTCO_NotOnCanceled = 0x40000,
	VSTCO_OnlyOnFaulted = 0x50000,
	VSTCO_OnlyOnRanToCompletion = 0x60000,
	VSTCO_ExecuteSynchronously = 0x80000,
	VSTCO_IndependentlyCanceled = 0x40000000,
	VSTCO_NotCancelable = 0x80000000,
	VSTCO_Default = VSTCO_NotOnFaulted
} ;
typedef DWORD VSTASKCONTINUATIONOPTIONS;

enum __VSTASKCREATIONOPTIONS {
	VSTCRO_None	= VSTCO_None,
	VSTCRO_PreferFairness	= VSTCO_PreferFairness,
	VSTCRO_LongRunning	= VSTCO_LongRunning,
	VSTCRO_AttachedToParent	= VSTCO_AttachedToParent,
	VSTCRO_DenyChildAttach	= VSTCO_DenyChildAttach,
	VSTCRO_NotCancelable	= VSTCO_NotCancelable
} ;
typedef DWORD VSTASKCREATIONOPTIONS;


MIDL_INTERFACE("05a07459-551f-4cdf-b38a-16089d083110")
IVsTaskBody : public IUnknown
{
public:
	virtual STDMETHODIMP DoWork(IVsTask *pTask, DWORD dwCount, IVsTask *pParentTasks[], VARIANT *pResult) = 0;
};

MIDL_INTERFACE("0b98eab8-00bb-45d0-ae2f-3de35cd68235")
IVsTask : public IUnknown
{
public:
	virtual STDMETHODIMP ContinueWith(VSTASKRUNCONTEXT context, IVsTaskBody *pTaskBody, IVsTask **ppTask) = 0;
	virtual STDMETHODIMP ContinueWithEx(VSTASKRUNCONTEXT context, VSTASKCONTINUATIONOPTIONS options,
		IVsTaskBody *pTaskBody, VARIANT pAsyncState, IVsTask **ppTask) = 0;
	virtual STDMETHODIMP Start() = 0;
	virtual STDMETHODIMP Cancel() = 0;
	virtual STDMETHODIMP GetResult(VARIANT *pResult) = 0;
	virtual STDMETHODIMP AbortIfCanceled() = 0;
	virtual STDMETHODIMP Wait() = 0;
	virtual STDMETHODIMP WaitEx(int millisecondsTimeout, VSTASKWAITOPTIONS options,
		VARIANT_BOOL *pTaskCompleted) = 0;
	virtual STDMETHODIMP get_IsFaulted(VARIANT_BOOL *pResult) = 0;
	virtual STDMETHODIMP get_IsCompleted(VARIANT_BOOL *pResult) = 0;
	virtual STDMETHODIMP get_IsCanceled(VARIANT_BOOL *pResult) = 0;
	virtual STDMETHODIMP get_AsyncState(VARIANT *pAsyncState) = 0;
	virtual STDMETHODIMP get_Description(BSTR *ppDescriptionText) = 0;
	virtual STDMETHODIMP put_Description(LPCOLESTR pDescriptionText) = 0;
};

MIDL_INTERFACE("83cfbaaf-0df9-403d-ae42-e738f0ac9735")
IVsTaskSchedulerService : public IUnknown
{
public:
	virtual STDMETHODIMP CreateTask(VSTASKRUNCONTEXT context, IVsTaskBody *pTaskBody, IVsTask **ppTask) = 0;
	virtual STDMETHODIMP CreateTaskEx(VSTASKRUNCONTEXT context, VSTASKCREATIONOPTIONS options, IVsTaskBody *pTaskBody,
		VARIANT pAsyncState, IVsTask **ppTask) = 0;
	virtual STDMETHODIMP ContinueWhenAllCompleted(VSTASKRUNCONTEXT context, DWORD dwTasks, IVsTask *pDependentTasks[],
		IVsTaskBody *pTaskBody, IVsTask **ppTask) = 0;
	virtual STDMETHODIMP ContinueWhenAllCompletedEx(VSTASKRUNCONTEXT context, DWORD dwTasks, IVsTask *pDependentTasks[],
		VSTASKCONTINUATIONOPTIONS options, IVsTaskBody *pTaskBody, VARIANT pAsyncState, IVsTask **ppTask) = 0;
	virtual STDMETHODIMP CreateTaskCompletionSource(IVsTaskCompletionSource **ppTaskSource) = 0;
	virtual STDMETHODIMP CreateTaskCompletionSourceEx(VSTASKCREATIONOPTIONS options, VARIANT asyncState,
		IVsTaskCompletionSource **ppTaskSource) = 0;
};

MIDL_INTERFACE("ab244925-40a8-4c5c-b0a5-717bb5d615b6")
SVsTaskSchedulerService : public IUnknown
{
public:
};
