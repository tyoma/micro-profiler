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

#include <collector/thread_monitor.h>

#include <common/string.h>
#include <mt/thread_callbacks.h>
#include <mt/tls.h>
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

		mt::milliseconds milliseconds(FILETIME t)
		{
			long long v = t.dwHighDateTime;

			v <<= 32;
			v += t.dwLowDateTime;
			return mt::milliseconds(v / 10000); // FILETIME is expressed in 100-ns intervals
		}
	}



	class thread_monitor_impl : public thread_monitor, public enable_shared_from_this<thread_monitor_impl>
	{
	public:
		thread_monitor_impl(mt::thread_callbacks &callbacks);

		virtual thread_id register_self();
		virtual void update_live_info(thread_info &info, unsigned int native_id) const;

	private:
		typedef HRESULT (WINAPI *GetThreadDescription_t)(HANDLE hThread, PWSTR *ppszThreadDescription);

		struct live_thread_info
		{
			live_thread_info();

			threads_map::value_type *thread_info_entry;
			shared_ptr<void> handle;
		};

		typedef unordered_map<unsigned int /*native_id*/, live_thread_info> running_threads_map;

	private:
		static void thread_exited(const weak_ptr<thread_monitor_impl> &wself, unsigned int native_id);

	private:
		mt::thread_callbacks &_callbacks;
		running_threads_map _alive_threads;
		thread_id _next_id;
		shared_ptr<void> _kernel_dll;
		GetThreadDescription_t _GetThreadDescription;
	};



	thread_monitor_impl::live_thread_info::live_thread_info()
		: thread_info_entry(0)
	{	}


	thread_monitor_impl::thread_monitor_impl(mt::thread_callbacks &callbacks)
		: _callbacks(callbacks), _next_id(0), _kernel_dll(::LoadLibraryA("kernel32.dll"), &::FreeLibrary),
			_GetThreadDescription(reinterpret_cast<GetThreadDescription_t>(::GetProcAddress(
				static_cast<HMODULE>(_kernel_dll.get()), "GetThreadDescription")))
	{	}

	thread_monitor::thread_id thread_monitor_impl::register_self()
	{
		const unsigned int id = ::GetCurrentThreadId();
		mt::lock_guard<mt::mutex> lock(_mutex);
		live_thread_info &lti = _alive_threads[id];

		if (!lti.thread_info_entry)
		{
			weak_ptr<thread_monitor_impl> wself = shared_from_this();
			thread_info ti = { id, string(), mt::milliseconds(), mt::milliseconds(0), mt::milliseconds(), false };

			lti.thread_info_entry = &*_threads.insert(make_pair(_next_id++, ti)).first;
			lti.handle = get_current_thread();
			_callbacks.at_thread_exit(bind(&thread_monitor_impl::thread_exited, wself, id));
		}
		return lti.thread_info_entry->first;
	}

	void thread_monitor_impl::update_live_info(thread_info &info, unsigned int native_id) const
	{
		running_threads_map::const_iterator i = _alive_threads.find(native_id);

		if (i != _alive_threads.end())
		{
			FILETIME dummy, user;
			PWSTR description;

			::GetThreadTimes(i->second.handle.get(), &dummy, &dummy, &dummy, &user);
			if (_GetThreadDescription && SUCCEEDED(_GetThreadDescription(i->second.handle.get(), &description)))
				info.description = unicode(description), ::LocalFree(description);
			info.cpu_time = milliseconds(user);
		}
	}

	void thread_monitor_impl::thread_exited(const weak_ptr<thread_monitor_impl> &wself, unsigned int native_id)
	{
		if (shared_ptr<thread_monitor_impl> self = wself.lock())
		{
			FILETIME exit_time = {};
			mt::lock_guard<mt::mutex> lock(self->_mutex);
			running_threads_map::iterator i = self->_alive_threads.find(native_id);
			thread_info &ti = i->second.thread_info_entry->second;

			self->update_live_info(ti, native_id);
			::GetSystemTimeAsFileTime(&exit_time);
			ti.end_time = milliseconds(exit_time);
			ti.complete = true;
			self->_alive_threads.erase(i);
		}
	}


	shared_ptr<thread_monitor> create_thread_monitor(mt::thread_callbacks &callbacks)
	{	return shared_ptr<thread_monitor>(new thread_monitor_impl(callbacks));	}
}
