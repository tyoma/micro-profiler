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

#include <ipc/channel_client.h>

#include <windows.h>

namespace micro_profiler
{
	namespace
	{
		class com_initializer
		{
		public:
			com_initializer()
			{	::CoInitialize(0);	}

			com_initializer(const com_initializer &)
			{	::CoInitialize(0);	}

			~com_initializer()
			{	::CoUninitialize();	}
		};

		class sequential_stream : com_initializer
		{
		public:
			sequential_stream(const CLSID &id)
				: _frontend(0)
			{
				::CoCreateInstance(id, NULL, CLSCTX_LOCAL_SERVER, IID_ISequentialStream, (void **)&_frontend);
				if (!_frontend)
					throw channel_creation_exception("");
			}

			sequential_stream(const sequential_stream &other)
				: _frontend(other._frontend)
			{	_frontend->AddRef();	}

			~sequential_stream()
			{	_frontend->Release();	}

			bool operator()(const void *buffer, size_t size)
			{
				ULONG written;

				return S_OK == _frontend->Write(buffer, static_cast<ULONG>(size), &written);
			}

		private:
			ISequentialStream *_frontend;
		};
	}

	channel_creation_exception::channel_creation_exception(const char *message)
		: runtime_error(message)
	{	}

	channel_t open_channel(const guid_t &id)
	{	return sequential_stream(reinterpret_cast<const CLSID &>(id));	}
}
