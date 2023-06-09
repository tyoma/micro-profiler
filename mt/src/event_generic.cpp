//	Copyright (c) 2011-2023 by Artem A. Gevorkyan (gevorkyan.org)
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

#include <mt/event.h>

#include <condition_variable>

using namespace std;

namespace mt
{
	class event::impl
	{
	public:
		impl(bool initial, bool auto_reset)
			: _state(initial), _auto(auto_reset)
		{	}

		void wait()
		{
			unique_lock<mutex> l(_mtx);

			return _cv.wait(l, [this] {
				const auto state = _state;

				if (_auto)
					_state = false;
				return state;
			});
		}

		bool wait(milliseconds timeout)
		{
			unique_lock<mutex> l(_mtx);
			const bool state = _cv.wait_for(l, timeout, [this] { return _state; });

			if (_auto & state)
				_state = false;
			return state;
		}

		void set()
		{
			_mtx.lock();
			_state = true;
			_cv.notify_all();
			_mtx.unlock();
		}

		void reset()
		{
			_mtx.lock();
			_state = false;
			_mtx.unlock();
		}

	private:
		condition_variable _cv;
		mutex _mtx;
		bool _state, _auto;
	};

	event::event(bool initial, bool auto_reset)
		: _impl(new impl(initial, auto_reset))
	{	}

	event::~event()
	{	}

	void event::wait()
	{	_impl->wait();	}

	bool event::wait(milliseconds period)
	{	return _impl->wait(period);	}

	void event::set()
	{	_impl->set();	}

	void event::reset()
	{	_impl->reset();	}
}
