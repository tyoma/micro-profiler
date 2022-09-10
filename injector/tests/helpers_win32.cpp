#include "helpers.h"

#include <common/module.h>
#include <ut/assert.h>
#include <windows.h>

#ifndef NT_SUCCESS
	#define NT_SUCCESS(Status) (((NTSTATUS)(Status)) >= 0)
#endif

namespace micro_profiler
{
	namespace tests
	{
		namespace
		{
			const auto ntdll = module::load("ntdll");
			NTSTATUS (NTAPI *NtSuspendProcess)(HANDLE hprocess) = ntdll / "NtSuspendProcess";
			NTSTATUS (NTAPI *NtResumeProcess)(HANDLE hprocess) = ntdll / "NtResumeProcess";
		}

		process_suspend::process_suspend(unsigned pid)
			: _hprocess(::OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid), &::CloseHandle)
		{	assert_is_true(!!_hprocess && NT_SUCCESS(NtSuspendProcess(_hprocess.get())));	}

		process_suspend::~process_suspend()
		{	NtResumeProcess(_hprocess.get());	}
	}
}
