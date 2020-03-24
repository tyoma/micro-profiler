//	Copyright (c) 2011-2020 by Artem A. Gevorkyan (gevorkyan.org)
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

#include <common/thread_monitor.h>

#include <common/string.h>

#include <list>
#include <mt/mutex.h>
#include <mt/tls.h>
#include <unordered_map>
#include <windows.h>

using namespace std;

namespace micro_profiler
{
	namespace
	{
		shared_ptr<void> get_current_thread()
		{
			HANDLE handle = NULL;

			::DuplicateHandle(::GetCurrentProcess(), ::GetCurrentThread(), ::GetCurrentProcess(), &handle,
				0, FALSE, DUPLICATE_SAME_ACCESS);
			return shared_ptr<void>(handle, &CloseHandle);
		}

		long long microseconds(FILETIME t)
		{
			long long v = t.dwHighDateTime;

			v <<= 32;
			v += t.dwLowDateTime;
			v /= 10; // FILETIME is expressed in 100-ns intervals
			return v;
		}
	}


	class thread_callbacks_impl : public thread_callbacks
	{
	public:
		void notify_thread_exit() throw();

		virtual void at_thread_exit(const atexit_t &handler);

	private:
		typedef vector<atexit_t> destructors_t;

	private:
		mt::mutex _mutex;
		list<destructors_t> _all_destructors;
		mt::tls<destructors_t> _thread_destructors;
	};


	class thread_monitor_impl : public thread_monitor, public enable_shared_from_this<thread_monitor_impl>
	{
	public:
		thread_monitor_impl(thread_callbacks &callbacks);

		virtual thread_id register_self();
		virtual thread_info get_info(thread_id id) const;

	private:
		typedef HRESULT (WINAPI *GetThreadDescription_t)(HANDLE hThread, PWSTR *ppszThreadDescription);

		struct thread_info_ex : thread_info
		{
			static void update(thread_info &destination, HANDLE hthread, GetThreadDescription_t GetThreadDescription_);

			shared_ptr<void> handle;
		};

		typedef unordered_map<thread_id, thread_info_ex> threads_map;
		typedef unordered_map<unsigned int, threads_map::value_type *> running_threads_map;

	private:
		static void thread_exited(const weak_ptr<thread_monitor_impl> &wself, unsigned int native_id);

	private:
		thread_callbacks &_callbacks;
		mutable mt::mutex _mutex;
		mutable threads_map _threads;
		running_threads_map _alive_threads;
		thread_id _next_id;
		shared_ptr<void> _kernel_dll;
		GetThreadDescription_t _GetThreadDescription;
	};



	void thread_callbacks_impl::notify_thread_exit() throw()
	{
		if (destructors_t *destructors = _thread_destructors.get())
		{
			while (!destructors->empty())
			{
				atexit_t d(destructors->back());

				destructors->pop_back();
				d();
			}
		}
	}

	void thread_callbacks_impl::at_thread_exit(const atexit_t &handler)
	{
		destructors_t *destructors = _thread_destructors.get();

		if (!destructors)
		{
			mt::lock_guard<mt::mutex> lock(_mutex);

			destructors = &*_all_destructors.insert(_all_destructors.end(), destructors_t());
			_thread_destructors.set(destructors);
		}
		destructors->push_back(handler);
	}


	thread_monitor_impl::thread_monitor_impl(thread_callbacks &callbacks)
		: _callbacks(callbacks), _next_id(0), _kernel_dll(::LoadLibraryA("kernel32.dll"), &::FreeLibrary),
			_GetThreadDescription(reinterpret_cast<GetThreadDescription_t>(::GetProcAddress(
				static_cast<HMODULE>(_kernel_dll.get()), "GetThreadDescription")))
	{	}

	thread_monitor::thread_id thread_monitor_impl::register_self()
	{
		const unsigned int id = ::GetCurrentThreadId();
		mt::lock_guard<mt::mutex> lock(_mutex);
		threads_map::value_type *&p = _alive_threads[id];

		if (!p)
		{
			weak_ptr<thread_monitor_impl> wself = shared_from_this();

			p = &*_threads.insert(make_pair(_next_id++, thread_info_ex())).first;
			p->second.native_id = id;
			p->second.handle = get_current_thread();
			p->second.end_time = mt::milliseconds(0);
			_callbacks.at_thread_exit(bind(&thread_monitor_impl::thread_exited, wself, id));
		}
		return p->first;
	}

	thread_info thread_monitor_impl::get_info(thread_id id) const
	{
		mt::lock_guard<mt::mutex> lock(_mutex);
		threads_map::iterator i = _threads.find(id);

		if (i == _threads.end())
			throw invalid_argument("Unknown thread id!");

		thread_info ti = i->second;

		if (i->second.handle)
			thread_info_ex::update(ti, i->second.handle.get(), _GetThreadDescription);
		return ti;
	}

	void thread_monitor_impl::thread_exited(const weak_ptr<thread_monitor_impl> &wself, unsigned int native_id)
	{
		if (shared_ptr<thread_monitor_impl> self = wself.lock())
		{
			FILETIME exit_time = {};
			mt::lock_guard<mt::mutex> lock(self->_mutex);
			running_threads_map::iterator i = self->_alive_threads.find(native_id);

			thread_info_ex::update(i->second->second, i->second->second.handle.get(), self->_GetThreadDescription);
			::GetSystemTimeAsFileTime(&exit_time);
			i->second->second.end_time = mt::milliseconds(microseconds(exit_time) / 1000);
			i->second->second.handle.reset();
			self->_alive_threads.erase(i);
		}
	}


	void thread_monitor_impl::thread_info_ex::update(thread_info &destination, HANDLE hthread,
		GetThreadDescription_t GetThreadDescription_)
	{
		PWSTR description2;
		FILETIME dummy, user;

		::GetThreadTimes(hthread, &dummy, &dummy, &dummy, &user);
		if (GetThreadDescription_ && SUCCEEDED(GetThreadDescription_(hthread, &description2)))
			destination.description = unicode(description2), ::LocalFree(description2);
		destination.cpu_time = mt::milliseconds(microseconds(user) / 1000);
	}


	thread_callbacks_impl &get_thread_callbacks_impl()
	{
		static thread_callbacks_impl callbacks;
		return callbacks;
	}

	thread_callbacks &get_thread_callbacks()
	{	return get_thread_callbacks_impl();	}

	shared_ptr<thread_monitor> create_thread_monitor(thread_callbacks &callbacks)
	{	return shared_ptr<thread_monitor>(new thread_monitor_impl(callbacks));	}
}

BOOL WINAPI DllMain(HINSTANCE /*hinstance*/, DWORD reason, LPVOID /*reserved*/)
{
	if (DLL_THREAD_DETACH == reason)
		micro_profiler::get_thread_callbacks_impl().notify_thread_exit();
	return TRUE;
}
