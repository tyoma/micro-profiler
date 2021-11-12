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

namespace micro_profiler
{
	template <typename OperationCB, typename ReadyCB>
	struct async_file_request
	{
		async_file_request(const std::string &path, const OperationCB &operation_, const ReadyCB &ready_,
			scheduler::queue &worker, scheduler::queue &apartment);

		const std::string path;
		const OperationCB operation;
		const ReadyCB ready;
		scheduler::private_worker_queue queue;
	};



	template <typename O, typename R>
	inline async_file_request<O, R>::async_file_request(const std::string &path_, const O &operation_, const R &ready_,
			scheduler::queue &worker, scheduler::queue &apartment)
		: path(path_), operation(operation_), ready(ready_), queue(worker, apartment)
	{	}


	template <typename T, typename ReaderCB, typename ReadyCB>
	inline void async_file_read(std::shared_ptr<void> &request, scheduler::queue &worker, scheduler::queue &apartment,
		const std::string &path, const ReaderCB &reader, const ReadyCB &ready)
	{
		using namespace scheduler;

		auto req = std::make_shared< async_file_request<ReaderCB, ReadyCB> >(path, reader, ready, worker, apartment);
		auto &rreq = *req;

		request = req;
		req->queue.schedule([&rreq] (private_worker_queue::completion &completion) {
			auto &req = rreq;
			auto data = std::make_shared<T>();

			try
			{
				read_file_stream r(req.path);

				req.operation(*data, r);
			}
			catch (...)
			{
				data.reset();
			}
			completion.deliver([&req, data] {	req.ready(data);	});
		});
	}

	template <typename WriterCB, typename ReadyCB>
	inline void async_file_write(std::shared_ptr<void> &request, scheduler::queue &worker, scheduler::queue &apartment,
		const std::string &path, const WriterCB &writer, const ReadyCB &ready)
	{
		using namespace scheduler;

		auto req = std::make_shared< async_file_request<WriterCB, ReadyCB> >(path, writer, ready, worker, apartment);
		auto &rreq = *req;

		request = req;
		req->queue.schedule([&rreq] (private_worker_queue::completion &completion) {
			auto &req = rreq;
			auto success = true;

			try
			{
				write_file_stream w(req.path);

				req.operation(w);
			}
			catch (...)
			{
				success = false;
			}
			completion.deliver([&req, success] {	req.ready(success);	});
		});
	}
}
