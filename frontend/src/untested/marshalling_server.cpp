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

#include <frontend/marshalling_server.h>

#include <functional>
#include <mt/mutex.h>
#include <vector>

using namespace std;

namespace micro_profiler
{
	class outbound_wrapper : public ipc::channel
	{
	public:
		outbound_wrapper(ipc::channel &underlying);

		void stop();

	private:
		virtual void disconnect() throw();
		virtual void message(const_byte_range payload);

	private:
		mt::mutex _mutex;
		ipc::channel *_underlying;
	};

	class marshalling_session : public ipc::channel
	{
	public:
		marshalling_session(shared_ptr<scheduler::queue> queue, shared_ptr<ipc::server> underlying, ipc::channel &outbound);
		~marshalling_session();

	private:
		virtual void disconnect() throw();
		virtual void message(const_byte_range payload);

	private:
		shared_ptr<scheduler::queue> _queue;
		shared_ptr<ipc::channel> _underlying;
		shared_ptr<outbound_wrapper> _outbound;
	};



	outbound_wrapper::outbound_wrapper(ipc::channel &underlying)
		: _underlying(&underlying)
	{	}

	void outbound_wrapper::stop()
	{
		mt::lock_guard<mt::mutex> lock(_mutex);
		_underlying = nullptr;
	}

	void outbound_wrapper::disconnect() throw()
	{
		mt::lock_guard<mt::mutex> lock(_mutex);
		if (_underlying)
			_underlying->disconnect();
	}

	void outbound_wrapper::message(const_byte_range payload)
	{
		mt::lock_guard<mt::mutex> lock(_mutex);
		if (_underlying)
			_underlying->message(payload);
	}


	marshalling_session::marshalling_session(shared_ptr<scheduler::queue> queue, shared_ptr<ipc::server> underlying,
			ipc::channel &outbound_)
		: _queue(queue), _outbound(make_shared<outbound_wrapper>(outbound_))
	{
		typedef pair<shared_ptr<ipc::channel>, shared_ptr<ipc::channel> > composite_t;

		const auto outbound = _outbound;

		_queue->schedule([this, underlying, outbound] {
			const auto composite = make_shared<composite_t>(outbound, underlying->create_session(*outbound));
			_underlying = shared_ptr<ipc::channel>(composite, composite->second.get());
		});
	}

	marshalling_session::~marshalling_session()
	{
		auto underlying = move(_underlying);
		function<void ()> destroy = [underlying] {	};

		_outbound->stop();
		_queue->schedule(move(destroy));
	}

	void marshalling_session::disconnect() throw()
	{
		auto underlying = _underlying;

		_queue->schedule([underlying] {
			if (underlying)
				underlying->disconnect();
		});
	}

	void marshalling_session::message(const_byte_range payload)
	{
		auto underlying = _underlying;
		auto data = make_shared< vector<byte> >(payload.begin(), payload.end());

		_queue->schedule([underlying, data] {
			if (underlying)
				underlying->message(const_byte_range(data->data(), data->size()));
		});
	}


	marshalling_server::marshalling_server(shared_ptr<ipc::server> underlying, shared_ptr<scheduler::queue> queue)
		: _underlying(underlying), _queue(queue)
	{	}

	marshalling_server::~marshalling_server()
	{
		auto underlying = move(_underlying);
		function<void ()> destroy = [underlying] {	};

		_queue->schedule(move(destroy));
	}

	void marshalling_server::stop()
	{	_underlying = nullptr;	}

	shared_ptr<ipc::channel> marshalling_server::create_session(ipc::channel &outbound)
	{	return shared_ptr<ipc::channel>(new marshalling_session(_queue, _underlying, outbound));	}
}
