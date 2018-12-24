//	Copyright (c) 2011-2018 by Artem A. Gevorkyan (gevorkyan.org)
//
//	Permission is hereby granted, free of charge, to any person obtaining a copy
//	of this software and associated documentation files (the "Software"), to deal
//	in the Software without restriction, including without limitation the rights
//	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//	copies of the Software, and to permit persons to whom the Software is
//	furnished to do so, subject to the following conditions:
//
//	The above copyright notice and this permission notice shall be included in
//	all copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
//	THE SOFTWARE.

#include "marshalling_server.h"

#include <functional>
#include <windows.h>

using namespace std;

namespace micro_profiler
{
	class marshalling_window
	{
	public:
		marshalling_window();
		~marshalling_window();

		void call(const function<void ()> &f);

	private:
		static LRESULT CALLBACK wndproc(HWND, UINT message, WPARAM wparam, LPARAM lparam);

	private:
		HWND _hwnd;
	};

	class marshalling_session : public ipc::channel
	{
	public:
		marshalling_session(const shared_ptr<marshalling_window> &m, ipc::server &underlying, ipc::channel &outbound);
		~marshalling_session();

	private:
		virtual void disconnect() throw();
		virtual void message(const_byte_range payload);

	private:
		shared_ptr<marshalling_window> _m;
		shared_ptr<ipc::channel> _underlying;
	};



	marshalling_window::marshalling_window()
		: _hwnd(::CreateWindowA("static", 0, 0, 0, 0, 0, 0, HWND_MESSAGE, 0, 0, 0))
	{	::SetWindowLong(_hwnd, GWLP_WNDPROC, (ULONG_PTR)&wndproc);	}

	marshalling_window::~marshalling_window()
	{	::DestroyWindow(_hwnd);	}

	void marshalling_window::call(const function<void ()> &f)
	{	::SendMessage(_hwnd, WM_USER, 0, (LPARAM)&f);	}

	LRESULT CALLBACK marshalling_window::wndproc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
	{
		if (WM_USER == message)
		{
			const function<void ()> &f = *(const function<void ()> *)lparam;

			f();
			return 1;
		}
		else
		{
			return ::DefWindowProc(hwnd, message, wparam, lparam);
		}
	}


	marshalling_session::marshalling_session(const shared_ptr<marshalling_window> &m, ipc::server &underlying,
			ipc::channel &outbound)
		: _m(m)
	{
		_m->call([&] {
			_underlying = underlying.create_session(outbound);
		});
	}

	marshalling_session::~marshalling_session()
	{
		_m->call([&] {
			_underlying.reset();
		});
	}

	void marshalling_session::disconnect() throw()
	{
		_m->call([&] {
			_underlying->disconnect();
		});
	}

	void marshalling_session::message(const_byte_range payload)
	{
		_m->call([&] {
			_underlying->message(payload);
		});
	}


	marshalling_server::marshalling_server(const shared_ptr<ipc::server> &underlying)
		: _underlying(underlying), _m(new marshalling_window)
	{	}

	marshalling_server::~marshalling_server()
	{
		_m->call([&] {
			_underlying.reset();
		});
	}

	shared_ptr<ipc::channel> marshalling_server::create_session(ipc::channel &outbound)
	{	return shared_ptr<ipc::channel>(new marshalling_session(_m, *_underlying, outbound));	}
}
