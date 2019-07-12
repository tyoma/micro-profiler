#pragma once

#include <windows.h>

struct IProfferAsyncService;
struct IVsTask;

MIDL_INTERFACE("c3a2d62e-64be-4008-924f-7e303e2b0001")
IAsyncProgressCallback : public IUnknown
{
public:
	virtual STDMETHODIMP ReportProgress(REFGUID guidService, LPCWSTR szWaitMessage, LPCWSTR szProgressText,
		LONG iCurrentStep, LONG iTotalSteps) = 0;
};

MIDL_INTERFACE("257B63FA-8388-4FEB-9DB8-3DB22F4405DE")
IAsyncServiceProvider : public IUnknown
{
public:
	virtual STDMETHODIMP QueryServiceAsync(REFGUID guidService, IVsTask **ppTask) = 0;
};

MIDL_INTERFACE("3EC4D7F6-4036-4406-A393-2FFF7B2E78A1")
IAsyncLoadablePackageInitialize : public IUnknown
{
public:
	virtual STDMETHODIMP Initialize(IAsyncServiceProvider *pServiceProvider, IProfferAsyncService *pProfferService,
		IAsyncProgressCallback *pProgressCallback, IVsTask **ppTask) = 0;

};
