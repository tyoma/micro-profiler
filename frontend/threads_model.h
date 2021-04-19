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

#include <common/noncopyable.h>
#include <common/hash.h>
#include <common/types.h>
#include <common/unordered_map.h>
#include <frontend/ordered_view.h>
#include <wpl/models.h>

namespace strmd
{
	template <typename StreamT, typename PackerT>
	class serializer;
	struct indexed_associative_container_reader;
}

namespace micro_profiler
{
	struct threads_model_reader;

	class threads_model : public wpl::list_model<std::string>,
		containers::unordered_map<unsigned int, thread_info, knuth_hash>, noncopyable
	{
	public:
		typedef std::function<void (const std::vector<unsigned int> &threads)> request_threads_t;

	public:
		threads_model(const request_threads_t &requestor);

		template <typename IteratorT>
		void notify_threads(IteratorT begin_, IteratorT end_);

		bool get_native_id(unsigned int &native_id, unsigned int thread_id) const throw();
		bool get_key(unsigned int &thread_id, index_type index) const throw();

		virtual index_type get_count() const throw() override;
		virtual void get_value(index_type index, std::string &text) const override;
		virtual std::shared_ptr<const wpl::trackable> track(index_type index) const override;

	private:
		const request_threads_t _requestor;
		ordered_view< containers::unordered_map<unsigned int, thread_info, knuth_hash> > _view;
		std::vector<unsigned int> _ids_buffer;

	private:
		template <typename StreamT, typename PackerT>
		friend class strmd::serializer;
		friend struct strmd::indexed_associative_container_reader;
		friend struct threads_model_reader;
	};



	template <typename IteratorT>
	inline void threads_model::notify_threads(IteratorT begin_, IteratorT end_)
	{
		for (; begin_ != end_; ++begin_)
		{
			if (end() != find(*begin_))
				continue;

			thread_info &ti = operator [](*begin_);

			ti.native_id = 0u;
			ti.complete = false;
		}
		_ids_buffer.clear();
		for (const_iterator i = begin(); i != end(); ++i)
		{
			if (!i->second.complete)
				_ids_buffer.push_back(i->first);
		}
		_requestor(_ids_buffer);
	}
}
