#include "mocks.h"

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
						: _ptr(static_cast<const unsigned char *>(message)), _remaining(size)
					{	}

					void read(void *data, size_t size)
					{
						assert_is_true(size <= _remaining);
						mem_copy(data, _ptr, size);
						_ptr += size;
						_remaining -= size;
					}

				private:
					const unsigned char *_ptr;
					size_t _remaining;
				};
			}


			void Tracer::read_collected(acceptor &a)
			{
				mt::lock_guard<mt::mutex> l(_mutex);

				for (TracesMap::const_iterator i = _traces.begin(); i != _traces.end(); ++i)
				{
					a.accept_calls(i->first, &i->second[0], i->second.size());
				}
				_traces.clear();
			}



			class frontend
			{
			public:
				explicit frontend(shared_ptr<frontend_state> state);
				frontend(const frontend &other);
				~frontend();

				bool operator ()(const void *buffer, size_t size);

			private:
				const frontend &operator =(const frontend &rhs);

			private:
				mutable shared_ptr<frontend_state> _state;
			};


			channel_t frontend_state::create()
			{	return frontend(shared_from_this());	}


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

			bool frontend::operator ()(const void *buffer, size_t size)
			{
				buffer_reader reader(buffer, size);
				strmd::deserializer<buffer_reader, packer> a(reader);

				a(*_state);
				return true;
			}
		}
	}
}
