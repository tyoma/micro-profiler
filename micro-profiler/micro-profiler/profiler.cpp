#include "profiler.h"

#include <vector>

using namespace std;
using namespace stdext;

namespace micro_profiler
{
	class thread_profiler
	{
		class call_statistics
		{
			void *function;
			unsigned __int64 entered_at, exclusive_entered_at, exclusive_spent;

		public:
			call_statistics(void *function_)
				: function(function_), exclusive_spent(0), entered_at(timestamp())
			{
				exclusive_entered_at = entered_at;
			}

			void enter_subroutine()
			{	exclusive_spent += timestamp() - exclusive_entered_at;	}

			void exit_subroutine()
			{	exclusive_entered_at = timestamp();	}

			void leave(hash_map<void * /*function address*/, function_statistics> &statistics)
			{
				unsigned __int64 ts = timestamp();
				function_statistics &s = statistics[function];

				++s.calls;
				s.exclusive_time += exclusive_spent + ts - exclusive_entered_at;
				s.inclusive_time += ts - entered_at;
			}
		};

		hash_map<void * /*function address*/, function_statistics> _statistics;
		vector<call_statistics> _shadow_stack;

	public:
		void enter_function(void *caller)
		{
			if (!_shadow_stack.empty())
				_shadow_stack.back().enter_subroutine();
			_shadow_stack.push_back(call_statistics(caller));
		}

		void exit_function()
		{
			_shadow_stack.back().leave(_statistics);
			_shadow_stack.pop_back();
			if (!_shadow_stack.empty())
				_shadow_stack.back().exit_subroutine();
		}

		void clear()
		{	_statistics.clear();	}

		void retrieve(	vector< pair<void *, function_statistics> > &statistics) const
		{
			for (hash_map<void *, function_statistics>::const_iterator i = _statistics.begin(); i != _statistics.end(); ++i)
				statistics.push_back(*i);
		}
	};


	profiler::profiler()
		: _collection_enabled(true)
	{	}

	profiler::~profiler()
	{	}

	void profiler::clear()
	{
		scoped_lock l(_thread_profilers_mtx);

		for (hash_map<unsigned int, thread_profiler>::iterator i = _profilers.begin(); i != _profilers.end(); ++i)
			i->second.clear();
	}

	void profiler::retrieve(vector<thread_calls_statistics> &statistics) const
	{
		scoped_lock l(_thread_profilers_mtx);

		statistics.clear();
		for (hash_map<unsigned int, thread_profiler>::const_iterator i = _profilers.begin(); i != _profilers.end(); ++i)
		{
			statistics.push_back(thread_calls_statistics());
			statistics.back().thread_id = i->first;
			i->second.retrieve(statistics.back().statistics);
		}
	}

	profiler &profiler::instance()
	{
		static profiler _profiler;
		return _profiler;
	}

	void profiler::enter(void *caller)
	{
		thread_profiler *p = 0;

		{
			scoped_lock l(_thread_profilers_mtx);

			p = _collection_enabled ? &_profilers[current_thread_id()] : 0;
		}
		if (p)
			p->enter_function(caller);
	}

	void profiler::exit()
	{
		thread_profiler *p = 0;

		{
			scoped_lock l(_thread_profilers_mtx);

			p = _collection_enabled ? &_profilers[current_thread_id()] : 0;
		}
		if (p)
			p->exit_function();
	}

	void profiler::enter_function(void *caller)
	{	instance().enter(caller);	}

	void profiler::exit_function()
	{	instance().exit();	}
}

extern "C" __declspec(naked, dllexport) void _penter2()
{
	_asm 
	{
		pushad
		mov  eax, esp
		add  eax, 32
		mov  eax, dword ptr[eax]
		sub  eax, 5
		push eax
		call micro_profiler::profiler::enter_function
		pop eax
		popad
		ret
	}
}

extern "C" void __declspec(naked, dllexport) _cdecl _pexit2()
{
	_asm 
	{
		pushad
		call micro_profiler::profiler::exit_function
		popad
		ret
	}
}
