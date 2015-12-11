#pragma once

#include <functional>
#include <memory>
#include <vector>

namespace std
{
	using tr1::function;
}

namespace micro_profiler
{
	namespace ipc
	{
		typedef unsigned char byte;

		class server
		{
		public:
			class impl;
			typedef std::function<void(const std::vector<byte> &input, std::vector<byte> &output)> callback;

		public:
			server(const char *server_name, const callback& cb);
			~server();

		private:
			std::auto_ptr<impl> _impl;
		};
	}
}
