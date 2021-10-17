//	Copyright (c) 2011-2021 by Artem A. Gevorkyan (gevorkyan.org)
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

#pragma once

#include "types.h"

#include <common/allocator.h>
#include <common/noncopyable.h>
#include <common/compiler.h>
#include <memory>
#include <mt/mutex.h>
#include <mt/thread_callbacks.h>
#include <mt/tls.h>
#include <vector>

namespace micro_profiler
{
	template <typename Q>
	class thread_queue_manager : noncopyable
	{
	public:
		typedef std::function<unsigned int ()> id_gen_cb;

	public:
		thread_queue_manager(allocator &allocator_, const buffering_policy &policy, mt::thread_callbacks &callbacks,
			const id_gen_cb &id_gen);

		void set_buffering_policy(const buffering_policy &policy);
		template <typename ReaderT>
		void read_collected(const ReaderT &reader);
		void flush() throw();
		Q &get_queue();

	protected:
		Q &construct_queue();

	private:
		typedef std::vector< std::shared_ptr<Q> > queues_t;

	private:
		mt::tls<Q> _queue_pointers_tls;

		queues_t _queues;
		mt::thread_callbacks &_thread_callbacks;
		allocator &_allocator;
		mt::mutex _mtx;
		buffering_policy _policy;
		const id_gen_cb _id_gen;
	};



	template <typename Q>
	inline thread_queue_manager<Q>::thread_queue_manager(allocator &allocator_, const buffering_policy &policy,
			mt::thread_callbacks &callbacks, const id_gen_cb &id_gen)
		: _thread_callbacks(callbacks), _allocator(allocator_), _policy(policy), _id_gen(id_gen)
	{	}

	template <typename Q>
	inline void thread_queue_manager<Q>::set_buffering_policy(const buffering_policy &policy)
	{
		mt::lock_guard<mt::mutex> l(_mtx);

		for (auto i = _queues.begin(); i != _queues.end(); ++i)
			(*i)->set_buffering_policy(policy);
		_policy = policy;
	}

	template <typename Q>
	template <typename ReaderT>
	inline void thread_queue_manager<Q>::read_collected(const ReaderT &reader)
	{
		mt::lock_guard<mt::mutex> l(_mtx);

		for (auto i = _queues.begin(); i != _queues.end(); ++i)
			(*i)->read_collected(reader);
	}

	template <typename Q>
	inline void thread_queue_manager<Q>::flush() throw()
	{
		if (auto *trace = _queue_pointers_tls.get())
			trace->flush();
	}

	template <typename Q>
	inline Q &thread_queue_manager<Q>::get_queue()
	{
		if (auto *q = _queue_pointers_tls.get())
			return *q;
		return construct_queue();
	}

	template <typename Q>
	FORCE_NOINLINE inline Q &thread_queue_manager<Q>::construct_queue()
	{
		unsigned int id;
		buffering_policy policy(0, 0, 0);

		{
			mt::lock_guard<mt::mutex> l(_mtx);
			
			id = _id_gen();
			policy = _policy;
		}

		const auto trace = std::make_shared<Q>(_allocator, policy, id);

		_thread_callbacks.at_thread_exit([trace] {	trace->flush();	});
		_queue_pointers_tls.set(trace.get());

		{
			mt::lock_guard<mt::mutex> l(_mtx);

			_queues.push_back(trace);
		}
		return *trace;
	}
}
