#pragma once

#include <memory>
#include <string>
#include <vector>

namespace micro_profiler
{
	namespace ipc
	{
		typedef unsigned char byte;

		class client
		{
		public:
			class impl;

		public:
			client(const char *server_name);
			~client();

			void call(const std::vector<byte> &input, std::vector<byte> &output);

			static void enumerate_servers(std::vector<std::string> &servers);

		private:
			std::auto_ptr<impl> _impl;
		};
	}
}
