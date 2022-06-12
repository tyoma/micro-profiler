#include "mock_frontend.h"

#include <collector/primitives.h>
#include <common/memory.h>
#include <common/serialization.h>
#include <common/stream.h>
#include <strmd/deserializer.h>
#include <ut/assert.h>

using namespace std;
using namespace std::placeholders;

namespace micro_profiler
{
	namespace tests
	{
		namespace mocks
		{
			class frontend : public ipc::channel
			{
			public:
				explicit frontend(shared_ptr<frontend_state> state);
				frontend(const frontend &other);
				~frontend();

				virtual void disconnect() throw();
				virtual void message(const_byte_range payload);

			private:
				const frontend &operator =(const frontend &rhs);

			private:
				mutable shared_ptr<frontend_state> _state;
			};


			ipc::channel_ptr_t frontend_state::create()
			{	return ipc::channel_ptr_t(new frontend(shared_from_this()));	}


			frontend::frontend(shared_ptr<frontend_state> state)
				: _state(state)
			{
				if (_state->constructed)
					_state->constructed();
			}

			frontend::frontend(const frontend &other)
			{	swap(other._state, _state);	}

			frontend::~frontend()
			{
				if (_state && _state->destroyed)
						_state->destroyed();
			}

			void frontend::disconnect() throw()
			{	}

			void frontend::message(const_byte_range payload)
			{
				buffer_reader reader(payload);
				strmd::deserializer<buffer_reader, packer> a(reader);

				a(*_state);
			}
		}
	}
}
