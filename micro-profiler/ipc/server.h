#pragma once

#include <vector>

namespace micro_profiler
{
	namespace ipc
	{
		typedef unsigned char byte;

		class server
		{
		public:
			struct session;

		public:
			server(const char *server_name);
			virtual ~server();

			void run();
			void stop();

		private:
			class impl;
			class outer_session;

		private:
			virtual session *create_session() = 0;

		private:
			impl *_impl;

			friend impl;
		};

		struct server::session
		{
			virtual ~session() { }

			virtual void on_message(const std::vector<byte> &input, std::vector<byte> &output) = 0;
		};
	}
}
