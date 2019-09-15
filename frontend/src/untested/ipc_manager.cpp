//	Copyright (c) 2011-2018 by Artem A. Gevorkyan (gevorkyan.org)
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

#include <frontend/marshalling_server.h>

using namespace std;

namespace micro_profiler
{
	namespace ipc
	{
		namespace
		{
			shared_ptr<void> try_run_server(const char *endpoint_id, const shared_ptr<server> &factory)
			try
			{
				return run_server(endpoint_id, factory);
			}
			catch (const initialization_failed &)
			{
				return shared_ptr<void>();
			}
		}

		ipc_manager::ipc_manager(const shared_ptr<server> &underlying, port_range range_)
			: _underlying(underlying), _range(range_), _remote_enabled(false), _port(0)
		{
			shared_ptr<server> m(new marshalling_server(_underlying));

			_sockets_server = probe_create_server(m, "127.0.0.1", _port, _range);
		}

		unsigned short ipc_manager::get_sockets_port() const
		{	return _sockets_server ? _port : 0;	}

		bool ipc_manager::remote_sockets_enabled() const
		{	return _remote_enabled;	}

		void ipc_manager::enable_remote_sockets(bool enable)
		{
			if (enable == _remote_enabled)
				return;

			shared_ptr<server> m(new marshalling_server(_underlying));

			_sockets_server.reset(); // Free the port before reopening.
			_sockets_server = probe_create_server(m, enable ? "0.0.0.0" : "127.0.0.1", _port, _range);
			_remote_enabled = enable;
		}

		//bool ipc_manager::com_enabled() const;
		//void ipc_manager::enable_com(bool enable);

		string ipc_manager::format_endpoint(const string &interface_, unsigned short port)
		{
			char buffer[100] = { 0 };

			snprintf(buffer, sizeof buffer - 1, "sockets|%s:%d", interface_.c_str(), port);
			return buffer;
		}

		shared_ptr<void> ipc_manager::probe_create_server(const shared_ptr<server> &underlying,
			const string &interface_, unsigned short &port, port_range range_)
		{
			shared_ptr<void> hserver = port ? try_run_server(format_endpoint(interface_, port).c_str(), underlying)
				: shared_ptr<void>();

			for (unsigned short p = range_.first, last = range_.first + range_.second; !hserver && p != last; ++p)
			{
				hserver = try_run_server(format_endpoint(interface_, p).c_str(), underlying);
				if (hserver)
					port = p;
			}
			return hserver;
		}
	}
}
