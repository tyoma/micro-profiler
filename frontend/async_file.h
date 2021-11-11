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

#include <common/file_stream.h>
#include <scheduler/private_queue.h>
#include <strmd/serializer.h>
#include <strmd/deserializer.h>
#include <strmd/packer.h>

namespace micro_profiler
{
	typedef strmd::deserializer<read_file_stream, strmd::varint> file_deserializer;

	template <typename T, typename DeserializerCB, typename ReadyCB>
	inline void async_file_read(std::shared_ptr<void> &request, scheduler::queue &worker, scheduler::queue &apartment,
		const std::string &path, const DeserializerCB &deserializer, const ReadyCB &ready)
	{
		using namespace scheduler;

		auto req = std::make_shared<private_worker_queue>(worker, apartment);

		request = req;
		req->schedule([path, deserializer, ready] (private_worker_queue::completion &completion) {
			auto data = std::make_shared<T>();
			auto ready_ = ready;

			try
			{
				read_file_stream r(path);
				file_deserializer d(r);

				deserializer(*data, d);
			}
			catch (...)
			{
				data.reset();
			}
			completion.deliver([ready_, data] {	ready_(data);	});
		});
	}
}
