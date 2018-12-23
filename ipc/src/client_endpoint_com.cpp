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

#include <ipc/com/endpoint.h>

#include <common/module.h>
#include <common/string.h>
#include <functional>

using namespace std;

namespace micro_profiler
{
	namespace ipc
	{
		namespace com
		{
			struct com_initialize
			{
				com_initialize();
				~com_initialize();
			};

			class client_session : public channel
			{
			public:
				client_session(const char *destination_endpoint_id, channel &inbound);

				virtual void disconnect() throw();
				virtual void message(const_byte_range payload);

			private:
				static shared_ptr<void> create_activation_context();
				static shared_ptr<void> lock_activation_context(const shared_ptr<void> &ctx);

			private:
				channel &_inbound;
				com_initialize _com_initialize;
				CComPtr<ISequentialStream> _stream;
			};



			com_initialize::com_initialize()
			{	::CoInitialize(NULL);	}

			com_initialize::~com_initialize()
			{	::CoUninitialize();	}


			client_session::client_session(const char *destination_endpoint_id, channel &inbound)
				: _inbound(inbound)
			{
				shared_ptr<void> ctx = create_activation_context();
				shared_ptr<void> ctx_lock = lock_activation_context(ctx);
				const guid_t id = from_string(destination_endpoint_id);

				if (S_OK != _stream.CoCreateInstance(reinterpret_cast<const GUID &>(id), NULL, CLSCTX_LOCAL_SERVER))
					throw connection_refused(destination_endpoint_id);
			}

			void client_session::disconnect() throw()
			{	}

			void client_session::message(const_byte_range payload)
			{
				ULONG written;
				HRESULT hr = _stream->Write(payload.begin(), static_cast<ULONG>(payload.length()), &written);

				if (S_OK != hr)
					_inbound.disconnect();
			}

			shared_ptr<void> client_session::create_activation_context()
			{
				ACTCTX ctx = { sizeof(ACTCTX), };
				HMODULE hmodule = (HMODULE)get_module_info((void *)&create_activation_context).load_address;

				ctx.hModule = hmodule;
				ctx.lpResourceName = ISOLATIONAWARE_MANIFEST_RESOURCE_ID;
				ctx.dwFlags = ACTCTX_FLAG_HMODULE_VALID | ACTCTX_FLAG_RESOURCE_NAME_VALID;
				return shared_ptr<void>(::CreateActCtx(&ctx), &::ReleaseActCtx);
			}

			shared_ptr<void> client_session::lock_activation_context(const shared_ptr<void> &ctx)
			{
				ULONG_PTR cookie;

				::ActivateActCtx(ctx.get(), &cookie);
				return shared_ptr<void>(reinterpret_cast<void*>(cookie), bind(&::DeactivateActCtx, 0, cookie));
			}


			shared_ptr<channel> connect_client(const char *destination_endpoint_id, channel &inbound)
			{	return shared_ptr<channel>(new client_session(destination_endpoint_id, inbound));	}
		}
	}
}
