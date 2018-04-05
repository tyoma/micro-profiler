#pragma once

#include <atlbase.h>
#include <vsshell.h>

namespace micro_profiler
{
	inline HWND get_frame_hwnd(const CComPtr<IVsUIShell> &shell)
	{
		HWND hparent = HWND_DESKTOP;

		return shell ? shell->GetDialogOwnerHwnd(&hparent), hparent : hparent;
	}
}
