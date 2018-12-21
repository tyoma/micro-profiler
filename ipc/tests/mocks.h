#pragma once

#include <ipc/endpoint.h>

#include <functional>
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
					ipc::channel *other_side;
					std::vector< std::vector<byte> > payloads_log;
					std::function<void()> received_message;
					std::function<void()> disconnected;

				private:
					virtual void disconnect() throw();
					virtual void message(const_byte_range payload);
				};

				class session_factory : public ipc::session_factory
				{
				public:
					std::vector< std::shared_ptr<session> > sessions;
					std::function<void (const std::shared_ptr<session> &new_session)> session_opened;

				private:
					virtual std::shared_ptr<channel> create_session(channel &other_side);
				};



				inline void session::disconnect() throw()
				{
					if (disconnected)
						disconnected();
				}

				inline void session::message(const_byte_range payload)
				{
					payloads_log.push_back(std::vector<byte>(payload.begin(), payload.end()));
					if (received_message)
						received_message();
				}


				inline std::shared_ptr<channel> session_factory::create_session(channel &other_side)
				{
					std::shared_ptr<session> s(new session);

					s->other_side = &other_side;
					sessions.push_back(s);
					if (session_opened)
						session_opened(s);
					return s;
				}
			}
		}
	}
}
