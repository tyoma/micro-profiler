#include "windowing.h"

#include <tchar.h>
#include <stdexcept>

using namespace std;

namespace
{
	const TCHAR c_wrapper_ptr_name[] = _T("IntegricityWrapper");
}

window_wrapper::window_wrapper(HWND hwnd)
	: _window(hwnd)
{
	_previous = reinterpret_cast<WNDPROC>(::SetWindowLongPtr(hwnd, GWL_WNDPROC, reinterpret_cast<LONG>(&window_wrapper::windowproc_proxy)));
	::SetProp(hwnd, c_wrapper_ptr_name, reinterpret_cast<HANDLE>(this));
}

LRESULT CALLBACK window_wrapper::windowproc_proxy(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
	window_wrapper *this_ = reinterpret_cast<window_wrapper *>(::GetProp(hwnd, c_wrapper_ptr_name));

	LRESULT result = this_->_previous(hwnd, message, wparam, lparam);
	if (message == WM_NCDESTROY)
		this_->_this.reset();
	return result;
}

shared_ptr<window_wrapper> window_wrapper::attach(HWND hwnd)
{
	if (::IsWindow(hwnd))
	{
		if (window_wrapper *stored = reinterpret_cast<window_wrapper *>(::GetProp(hwnd, c_wrapper_ptr_name)))
			return stored->_this;
		else
		{
			shared_ptr<window_wrapper> w(new window_wrapper(hwnd));

			w->_this = w;
			return w;
		}
	}
	else
		throw invalid_argument("");
}

HWND window_wrapper::hwnd() const
{	return _window;	}
