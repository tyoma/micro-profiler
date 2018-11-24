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

#include <frontend/async_symbol_resolver.h>

using namespace std;

namespace micro_profiler
{
	namespace
	{
		void dummy(const shared_ptr<symbol_resolver> &)
		{
		}
	}

	async_symbol_resolver::async_symbol_resolver(const resolver_factory_t &resolver_factory,
			const shared_ptr<queue> &client_queue, const shared_ptr<queue> &cradle_queue)
		: _client_queue(client_queue),_cradle_queue(cradle_queue)
	{
		_cradle_queue->post([this, resolver_factory] {
			_underlying = resolver_factory();
		});
	}

	async_symbol_resolver::~async_symbol_resolver()
	{	_cradle_queue->post(bind(&dummy, move(_underlying)));	}

	void async_symbol_resolver::get_symbol(shared_ptr<void> &hrequest, address_t address, const symbol_acceptor_t &onready)
	{
		shared_ptr<symbol_acceptor_t> request(new symbol_acceptor_t(onready));
		weak_ptr<symbol_acceptor_t> wrequest(request);

		hrequest = request;
		_cradle_queue->post([this, address, wrequest] {
			if (auto request = wrequest.lock())
			{
				weak_ptr<symbol_acceptor_t> wrequest2 = wrequest;
				symbol_resolver::symbol_t symbol;

				_underlying->get_symbol(address, symbol);
				_client_queue->post([wrequest2, symbol] {
					if (auto request = wrequest2.lock())
						(*request)(symbol);
				});
			}
		});
	}
}
