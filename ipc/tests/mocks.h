#pragma once

#include <ipc/endpoint.h>

#include <vector>

namespace micro_profiler
{
	namespace ipc
	{
		namespace tests
		{
			namespace mocks
			{
				class session : public ipc::channel
				{
				public:
					std::vector< std::vector<byte> > payloads_log;

				private:
					virtual void disconnect();
					virtual void message(const_byte_range payload);
				};

				class session_factory : public ipc::session_factory
				{
				public:
					std::vector< std::shared_ptr<session> > sessions;

				private:
					virtual std::shared_ptr<channel> create_session(channel &passive);
				};



				inline void session::disconnect()
				{	}

				inline void session::message(const_byte_range payload)
				{	payloads_log.push_back(std::vector<byte>(payload.begin(), payload.end()));	}


				inline std::shared_ptr<channel> session_factory::create_session(channel &/*passive*/)
				{
					std::shared_ptr<session> s(new session);

					sessions.push_back(s);
					return s;
				}
			}
		}
	}
}
