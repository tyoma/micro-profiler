#include <cstdint>
#include <ipc/endpoint_spawn.h>

using namespace std;

namespace micro_profiler
{
	namespace ipc
	{
		class server_session : public channel
		{
		public:
			server_session(string endpoint_id, channel &outbound)
				: _otherside(connect_client(endpoint_id, outbound))
			{	}

		private:
			virtual void disconnect() throw() override
			{
				uint8_t buffer[] = "Lorem ipSUM aMet dOlOr";

				_otherside->message(const_byte_range(buffer, sizeof buffer));
			}

			virtual void message(const_byte_range payload) override
			{	_otherside->message(payload);	}

		private:
			channel_ptr_t _otherside;
		};

		channel_ptr_t spawn::create_session(const vector<string> &arguments, channel &outbound)
		{	return make_shared<server_session>(arguments.at(0), outbound);	}
	}
}
