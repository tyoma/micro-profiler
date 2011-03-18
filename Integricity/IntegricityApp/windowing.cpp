#include "windowing.h"

#include <tchar.h>
#include <stdexcept>

using namespace std;
using namespace placeholders;

namespace
{
	const TCHAR c_wrapper_ptr_name[] = _T("IntegricityWrapperPtr");

	class disconnector : noncopyable, public destructible
	{
		shared_ptr<window_wrapper> _target;

	public:
		disconnector(shared_ptr<window_wrapper> target)
			: _target(target)
		{	}

		~disconnector() throw()
		{	_target->unadvise();	}
	};
}

window_wrapper::window_wrapper(HWND hwnd)
	: _window(hwnd)
{
	_previous_handler = reinterpret_cast<WNDPROC>(::SetWindowLongPtr(hwnd, GWL_WNDPROC, reinterpret_cast<LONG>(&window_wrapper::windowproc_proxy)));
	::SetProp(hwnd, c_wrapper_ptr_name, reinterpret_cast<HANDLE>(this));
}

LRESULT CALLBACK window_wrapper::windowproc_proxy(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
	shared_ptr<window_wrapper> w(window_wrapper::extract(hwnd));

	if (message == WM_NCDESTROY)
		::RemoveProp(hwnd, c_wrapper_ptr_name);

	LRESULT result = w->_user_handler != 0 ? (*w->_user_handler)(message, wparam, lparam, bind(w->_previous_handler, w->hwnd(), _1, _2, _3))
			: w->_previous_handler(hwnd, message, wparam, lparam);

	if (message == WM_NCDESTROY)
		w->_this.reset();
	return result;
}

shared_ptr<window_wrapper> window_wrapper::extract(HWND hwnd)
{
	window_wrapper *attached = reinterpret_cast<window_wrapper *>(::GetProp(hwnd, c_wrapper_ptr_name));

	return attached ? attached->shared_from_this() : 0;
}

shared_ptr<window_wrapper> window_wrapper::attach(HWND hwnd)
{
	if (::IsWindow(hwnd))
	{
		shared_ptr<window_wrapper> w(window_wrapper::extract(hwnd));

		if (w == 0)
		{
			w.reset(new window_wrapper(hwnd));
			w->_this = w;
		}
		return w;
	}
	else
		throw invalid_argument("");
}

shared_ptr<destructible> window_wrapper::advise(const user_handler_t &user_handler)
{
	_user_handler.reset(new user_handler_t(user_handler));
	return shared_ptr<destructible>(new disconnector(shared_from_this()));
}

void window_wrapper::unadvise() throw()
{	_user_handler.reset();	}

HWND window_wrapper::hwnd() const
{	return _window;	}
