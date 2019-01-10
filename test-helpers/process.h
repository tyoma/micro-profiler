#pragma once

#include "helpers.h"

#include <ipc/endpoint.h>
#include <memory>
#include <mt/event.h>
#include <string>

namespace micro_profiler
{
	namespace tests
	{
		struct running_process
		{
			virtual ~running_process() {	}
			virtual unsigned int get_pid() const = 0;
			virtual void wait() const = 0;
		};


		class controllee_session : public ipc::channel
		{
		public:
			controllee_session(ipc::channel &outbound);

			void disconnect_client();
			void send(const_byte_range payload);

			virtual void disconnect() throw();
			virtual void message(const_byte_range payload);

		public:
			std::vector< std::vector<byte> > messages;

		private:
			ipc::channel *_outbound;
			mt::event _ready;
		};

		template <typename SessionT = controllee_session>
		class runner_controller : public ipc::server
		{
		public:
			bool wait_connection();

		public:
			std::vector< std::shared_ptr<SessionT> > sessions;

		private:
			// ipc::server methods
			virtual std::shared_ptr<ipc::channel> create_session(ipc::channel &outbound);

		private:
			mt::event _ready;
		};



		std::shared_ptr<running_process> create_process(const std::string &executable_path,
			const std::string &command_line);


		inline controllee_session::controllee_session(ipc::channel &outbound)
			: _outbound(&outbound)
		{	}

		inline void controllee_session::disconnect_client()
		{	_outbound->disconnect();	}

		inline void controllee_session::send(const_byte_range payload)
		{	_outbound->message(payload);	}

		inline void controllee_session::disconnect() throw()
		{	}

		inline void controllee_session::message(const_byte_range payload)
		{
			messages.push_back(std::vector<byte>(payload.begin(), payload.end()));
			_ready.set();
		}


		template <typename SessionT>
		inline bool runner_controller<SessionT>::wait_connection()
		{	return _ready.wait(mt::milliseconds(10000));	}

		template <typename SessionT>
		inline std::shared_ptr<ipc::channel> runner_controller<SessionT>::create_session(ipc::channel &outbound)
		{
			std::shared_ptr<SessionT> s(new SessionT(outbound));

			sessions.push_back(s);
			_ready.set();
			return s;
		}
	}
}
