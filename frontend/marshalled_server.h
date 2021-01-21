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
#include <ipc/endpoint.h>

namespace scheduler
{
	struct queue;
}

namespace micro_profiler
{
	class marshalled_server : public ipc::server, noncopyable
	{
	public:
		marshalled_server(std::shared_ptr<ipc::server> underlying, std::shared_ptr<scheduler::queue> queue);
		~marshalled_server();

		void stop();

	private:
		class lifetime;
		class outbound_wrapper;
		class marshalled_session;

	private:
		virtual std::shared_ptr<ipc::channel> create_session(ipc::channel &outbound);

	private:
		const std::shared_ptr<lifetime> _lifetime;
		std::shared_ptr<ipc::server> _underlying;
		const std::shared_ptr<scheduler::queue> _queue;
	};

	class marshalled_server::outbound_wrapper : public ipc::channel, noncopyable
	{
	public:
		outbound_wrapper(ipc::channel &underlying);

		void stop();

	private:
		virtual void disconnect() throw();
		virtual void message(const_byte_range payload);

	private:
		const std::shared_ptr<lifetime> _lifetime;
		ipc::channel &_underlying;
	};

	class marshalled_server::marshalled_session : public ipc::channel, noncopyable
	{
	public:
		marshalled_session(std::shared_ptr<scheduler::queue> queue, ipc::channel &outbound);
		~marshalled_session();

		void create_underlying(std::shared_ptr<ipc::server> underlying_server);

	private:
		virtual void disconnect() throw();
		virtual void message(const_byte_range payload);

	private:
		const std::shared_ptr<lifetime> _lifetime;
		const std::shared_ptr<scheduler::queue> _queue;
		std::shared_ptr<ipc::channel> _underlying;
		std::shared_ptr<outbound_wrapper> _outbound;
	};
}
