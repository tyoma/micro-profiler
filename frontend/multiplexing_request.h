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

#include <list>
#include <map>
#include <memory>

namespace micro_profiler
{
	template <typename KeyT, typename CallbackT>
	class multiplexing_request
	{
	public:
		typedef std::shared_ptr<void> handle_t;
		typedef std::map<KeyT, multiplexing_request> map_type;
		typedef std::shared_ptr<map_type> map_type_ptr;

		struct request_init
		{
			multiplexing_request &multiplexed;
			handle_t *underlying;
		};

	public:
		static request_init create(handle_t &request, const map_type_ptr &requests, const KeyT &key, const CallbackT &callback)
		{
			auto i = requests->find(key);
			auto &mx = requests->end() == i
				? i = requests->insert(std::make_pair(key, multiplexing_request(requests))).first, i->second._self = i,
					i->second
				: i->second;
			auto underlying = mx._callbacks.empty() ? &mx._underlying : nullptr;
			auto inner = mx._callbacks.insert(mx._callbacks.end(), callback);
			request_init r = {
				mx,
				underlying
			};

			request.reset(&*inner, [&mx, inner] (void *) {	mx.destroy_inner(inner);	});
			return r;
		}

		template <typename ExecutorT>
		void invoke(const ExecutorT &executor)
		{
			invocation_guard g(this);

			for (auto i = _callbacks.begin(); i != _callbacks.end(); ++i)
				*i ? executor(*i) : void();
		}

	private:
		class invocation_guard
		{
		public:
			invocation_guard(multiplexing_request *owner)
				: _owner(owner)
			{	_owner->enter_invocation();	}

			~invocation_guard()
			{	_owner->exit_invocation();	}

		private:
			multiplexing_request *_owner;
		};

	private:
		multiplexing_request(const map_type_ptr &requests) throw()
			: _requests(requests), _in_progress(false), _require_cleanup(false)
		{	}

		void enter_invocation() throw()
		{	_in_progress = true;	}

		void exit_invocation() throw()
		{
			if (_require_cleanup)
			{
				_callbacks.remove_if([] (const CallbackT &callback) {	return !callback;	});
				_require_cleanup = false;
			}
			_in_progress = false;
			destroy_if_empty();
		}

		template <typename IteratorT>
		void destroy_inner(IteratorT inner) throw()
		{
			if (_in_progress)
				*inner = CallbackT(), _require_cleanup = true;
			else
				_callbacks.erase(inner), destroy_if_empty();
		}

		void destroy_if_empty() throw()
		{
			if (!_callbacks.empty())
				return;

			auto requests = _requests;

			requests->erase(_self);
		}

	private:
		std::list<CallbackT> _callbacks;
		const map_type_ptr _requests;
		typename map_type::iterator _self;
		handle_t _underlying;
		bool _in_progress : 1, _require_cleanup : 1;
	};
}
