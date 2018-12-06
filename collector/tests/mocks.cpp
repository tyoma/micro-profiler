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


			Tracer::Tracer(timestamp_t latency)
				: _latency(latency)
			{	}

			Tracer::~Tracer() throw()
			{	}

			void Tracer::read_collected(acceptor &a)
			{
				mt::lock_guard<mt::mutex> l(_mutex);

				for (TracesMap::const_iterator i = _traces.begin(); i != _traces.end(); ++i)
				{
					a.accept_calls(i->first, &i->second[0], i->second.size());
				}
				_traces.clear();
			}

			timestamp_t Tracer::profiler_latency() const throw()
			{	return _latency;	}


			template <typename ArchiveT>
			void serialize(ArchiveT &a, frontend_state &state)
			{
				commands c;
				initialization_data id;
				loaded_modules lm;
				statistics_map_detailed u;
				unloaded_modules um;

				a(c);
				switch (c)
				{
				case init:
					if (state.initialized)
						a(id), state.initialized(id);
					break;

				case modules_loaded:
					if (state.modules_loaded)
						a(lm), state.modules_loaded(lm);
					break;

				case update_statistics:
					if (state.updated)
						a(u), state.updated(u);
					break;

				case modules_unloaded:
					if (state.modules_unloaded)
						a(um), state.modules_unloaded(um);
					break;
				}
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


			frontend_state::frontend_state(const shared_ptr<void> &ownee)
				: _ownee(ownee)
			{	}

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
