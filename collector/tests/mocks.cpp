#include "mocks.h"

#include <test-helpers/helpers.h>

#include <common/memory.h>
#include <common/serialization.h>
#include <stdexcept>
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

			class Frontend
			{
			public:
				explicit Frontend(FrontendState &state);
				Frontend(const Frontend &other);
				~Frontend();

				bool operator ()(const void *buffer, size_t size);

				static Frontend Create(FrontendState& state);

			private:
				const Frontend &operator =(const Frontend &rhs);

			private:
				FrontendState &_state;
			};

			frontend_factory_t FrontendState::MakeFactory()
			{	return bind(&Frontend::Create, ref(*this));	}

			Frontend::Frontend(FrontendState &state)
				: _state(state)
			{	++_state.ref_count;	}

			Frontend::Frontend(const Frontend &other)
				: _state(other._state)
			{	++_state.ref_count;	}

			Frontend::~Frontend()
			{	--_state.ref_count;	}

			Frontend Frontend::Create(FrontendState& state)
			{	return Frontend(state);	}

			template <typename ArchiveT>
			void serialize(ArchiveT &a, FrontendState &state)
			{
				commands c;

				a(c);
				state.update_log.push_back(FrontendState::ReceivedEntry());
				FrontendState::ReceivedEntry &e = state.update_log.back();
				switch (c)
				{
				case init:
					state.update_log.pop_back();
					a(state.process_init);
					if (state.oninitialized)
						state.oninitialized();
					break;

				case modules_loaded:
					a(e.image_loads);
					state.modules_state_updated.set();
					break;

				case update_statistics:
					a(e.update);
					state.updated.set();
					state.update_lock.wait();
					break;

				case modules_unloaded:
					a(e.image_unloads);
					state.modules_state_updated.set();
				}
			}

			bool Frontend::operator ()(const void *message, size_t size)
			{
				buffer_reader reader(message, size);
				strmd::deserializer<buffer_reader, packer> a(reader);

				a(_state);
				return true;
			}


			FrontendState::FrontendState(const function<void()>& oninitialized_)
				: update_lock(true, false), oninitialized(oninitialized_), ref_count(0)
			{	}


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
		}
	}
}
