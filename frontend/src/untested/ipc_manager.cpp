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

#include <frontend/ipc_manager.h>

#include <common/constants.h>
#include <common/formatting.h>
#include <common/string.h>
#include <frontend/marshalled_server.h>
#include <logger/log.h>

#define PREAMBLE "IPC manager: "

using namespace std;

namespace micro_profiler
{
	namespace
	{
		class logging_server : public ipc::server, noncopyable
		{
		public:
			logging_server(const shared_ptr<ipc::server> &server, const string &endpoint_id)
				: _server(server), _endpoint_id(endpoint_id)
			{	}

			virtual std::shared_ptr<ipc::channel> create_session(ipc::channel &outbound)
			{
				LOG(PREAMBLE "creating frontend server session...") % A(_endpoint_id);
				return _server->create_session(outbound);
			}

		private:
			const shared_ptr<ipc::server> _server;
			const string _endpoint_id;
		};

		shared_ptr<void> try_run_server(const string &endpoint_id, const shared_ptr<ipc::server> &server)
		try
		{
			LOG(PREAMBLE "attempting server creation...") % A(endpoint_id);
			shared_ptr<ipc::server> lserver(new logging_server(server, endpoint_id));
			shared_ptr<void> hserver = run_server(endpoint_id, lserver);
			LOG(PREAMBLE "succeeded.");
			return hserver;
		}
		catch (const ipc::initialization_failed &e)
		{
			LOG(PREAMBLE "failed.") % A(e.what());
			return shared_ptr<void>();
		}
	}

	ipc_manager::ipc_manager(std::shared_ptr<ipc::server> server, std::shared_ptr<scheduler::queue> queue,
			port_range range_, const guid_t *com_server_id)
		: _server(new marshalled_server(server, queue)), _range(range_), _remote_enabled(false), _port(0)
	{
		if (com_server_id)
		{
#ifdef _WIN32
			const string endpoint_id = ipc::com_endpoint_id(*com_server_id);
			shared_ptr<ipc::server> lserver(new logging_server(_server, endpoint_id));
	
			_com_server_handle = run_server(endpoint_id, lserver);
#endif
		}

		_sockets_server_handle = probe_create_server(_server, ipc::localhost, _port, _range);
	}

	ipc_manager::~ipc_manager()
	{	_server->stop(); }

	unsigned short ipc_manager::get_sockets_port() const
	{	return _sockets_server_handle ? _port : 0;	}

	bool ipc_manager::remote_sockets_enabled() const
	{	return _remote_enabled;	}

	void ipc_manager::enable_remote_sockets(bool enable)
	{
		if (enable == _remote_enabled)
			return;
		_sockets_server_handle.reset(); // Free the port before reopening.
		_sockets_server_handle = probe_create_server(_server, enable ? ipc::all_interfaces : ipc::localhost, _port, _range);
		_remote_enabled = enable;
		LOG(PREAMBLE "enable remote...") % A(enable);
	}

	//bool ipc_manager::com_enabled() const;
	//void ipc_manager::enable_com(bool enable);

	shared_ptr<void> ipc_manager::probe_create_server(const shared_ptr<ipc::server> &server,
		ipc::ip_v4 interface_, unsigned short &port, port_range range_)
	{
		shared_ptr<void> hserver = port ? try_run_server(ipc::sockets_endpoint_id(interface_, port), server)
			: shared_ptr<void>();

		for (unsigned short p = range_.first, last = range_.first + range_.second; !hserver && p != last; ++p)
		{
			hserver = try_run_server(ipc::sockets_endpoint_id(interface_, p), server);
			if (hserver)
				port = p;
		}
		return hserver;
	}
}
