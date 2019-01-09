#include <test-helpers/mock_frontend.h>

#include <common/memory.h>
#include <common/serialization.h>
#include <ut/assert.h>

using namespace std;
using namespace std::placeholders;

namespace micro_profiler
{
	namespace tests
	{
		namespace mocks
		{
			namespace
			{
				class buffer_reader
				{
				public:
					buffer_reader(const void *message, size_t size)
						: _ptr(static_cast<const byte *>(message)), _remaining(size)
					{	}

					void read(void *data, size_t size)
					{
						assert_is_true(size <= _remaining);
						mem_copy(data, _ptr, size);
						_ptr += size;
						_remaining -= size;
					}

				private:
					const byte *_ptr;
					size_t _remaining;
				};
			}


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


			shared_ptr<ipc::channel> frontend_state::create()
			{	return shared_ptr<ipc::channel>(new frontend(shared_from_this()));	}


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
				buffer_reader reader(payload.begin(), payload.length());
				strmd::deserializer<buffer_reader, packer> a(reader);

				a(*_state);
			}
		}
	}
}
