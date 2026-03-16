#pragma once

#include <coipc/endpoint.h>

#include <common/range.h>
#include <functional>
#include <mt/mutex.h>
#include <vector>

namespace micro_profiler
{
	namespace ipc
	{
		namespace tests
		{
			namespace mocks
			{
				class channel : public coipc::channel
				{
				public:
					std::function<void (coipc::const_byte_range payload)> on_message;
					std::function<void ()> on_disconnect;

				private:
					virtual void disconnect() throw();
					virtual void message(coipc::const_byte_range payload);
				};

				class session : public coipc::channel
				{
				public:
					session();

				public:
					coipc::channel *outbound;
					std::vector< std::vector<byte> > payloads_log;
					unsigned disconnections;
					std::function<void()> received_message;
					std::function<void()> disconnected;

				private:
					virtual void disconnect() throw();
					virtual void message(coipc::const_byte_range payload);
				};

				class server : public coipc::server
				{
				public:
					std::vector< std::shared_ptr<session> > sessions;
					std::function<void (const std::shared_ptr<session> &new_session)> session_created;

				private:
					virtual coipc::channel_ptr_t create_session(coipc::channel &outbound);

				private:
					mt::mutex _mutex;
				};



				inline void channel::disconnect() throw()
				{
					if (on_disconnect)
						on_disconnect();
				}

				inline void channel::message(coipc::const_byte_range payload)
				{
					if (on_message)
						on_message(payload);
				}


				inline session::session()
					: disconnections(0)
				{	}

				inline void session::disconnect() throw()
				{
					disconnections++;
					if (disconnected)
						disconnected();
				}

				inline void session::message(coipc::const_byte_range payload)
				{
					payloads_log.push_back(std::vector<byte>(payload.begin(), payload.end()));
					if (received_message)
						received_message();
				}


				inline coipc::channel_ptr_t server::create_session(coipc::channel &outbound)
				{
					std::shared_ptr<session> s(new session);
					mt::lock_guard<mt::mutex> lock(_mutex);

					s->outbound = &outbound;
					sessions.push_back(s);
					if (session_created)
						session_created(s);
					return s;
				}
			}
		}
	}
}
