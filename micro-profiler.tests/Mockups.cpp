#include "Mockups.h"

#include "Helpers.h"

#include <common/serialization.h>

#include <ObjIdl.h>
#include <stdexcept>
#include <strmd/strmd/deserializer.h>
#include <ut/assert.h>

using wpl::mt::thread;
using namespace std;
using namespace std::placeholders;

namespace micro_profiler
{
	namespace tests
	{
		namespace mockups
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
						memcpy(data, _ptr, size);
						_ptr += size;
						_remaining -= size;
					}

				private:
					SAFEARRAY *_sa;
					const byte *_ptr;
					size_t _remaining;
				};
			}

			class Frontend : public ISequentialStream
			{
			public:
				static void Create(FrontendState& state, ISequentialStream **frontend);

			private:
				explicit Frontend(FrontendState& state);
				~Frontend();

				STDMETHODIMP QueryInterface(REFIID riid, void **ppv);
				STDMETHODIMP_(ULONG) AddRef();
				STDMETHODIMP_(ULONG) Release();

				STDMETHODIMP Read(void *pv, ULONG cb, ULONG *pcbRead);
				STDMETHODIMP Write(const void *pv, ULONG cb, ULONG *pcbWritten);

				const Frontend &operator =(const Frontend &rhs);

			private:
				unsigned int _refcount;
				FrontendState &_state;
			};

			function<void(ISequentialStream **)> FrontendState::MakeFactory()
			{	return bind(&Frontend::Create, ref(*this), _1);	}

			Frontend::Frontend(FrontendState &state)
				: _refcount(0), _state(state)
			{	}

			Frontend::~Frontend()
			{	_state.released = true;	}

			void Frontend::Create(FrontendState& state, ISequentialStream **frontend)
			{	static_cast<ISequentialStream *>(new Frontend(state))->QueryInterface(frontend);	}

			STDMETHODIMP Frontend::QueryInterface(REFIID riid, void **ppv)
			{
				if (riid != IID_IUnknown && riid != __uuidof(ISequentialStream))
					return E_NOINTERFACE;
				*reinterpret_cast<IUnknown **>(ppv) = this;
				AddRef();
				return S_OK;
			}

			STDMETHODIMP_(ULONG) Frontend::AddRef()
			{	return ++_refcount;	}

			STDMETHODIMP_(ULONG) Frontend::Release()
			{
				if (!--_refcount)
					delete this;
				return 0;
			}

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
					for (loaded_modules::iterator i = e.image_loads.begin(); i != e.image_loads.end(); ++i)
						toupper(i->second);				
					state.modules_state_updated.raise();
					break;

				case update_statistics:
					a(e.update);
					state.updated.raise();
					state.update_lock.wait();
					break;

				case modules_unloaded:
					a(e.image_unloads);
					state.modules_state_updated.raise();
				}
			}

			STDMETHODIMP Frontend::Read(void *, ULONG, ULONG *)
			{	return E_NOTIMPL;	}

			STDMETHODIMP Frontend::Write(const void *message, ULONG size, ULONG *written)
			{
				buffer_reader reader(message, size);
				strmd::deserializer<buffer_reader> a(reader);

				a(_state);
				*written = size;
				return S_OK;
			}


			FrontendState::FrontendState(const function<void()>& oninitialized_)
				: update_lock(true, false), oninitialized(oninitialized_), updated(false, true),
					modules_state_updated(false, true), released(false)
			{	}


			Tracer::Tracer(timestamp_t latency)
				: _latency(latency)
			{	}

			void Tracer::read_collected(acceptor &a)
			{
				scoped_lock l(_mutex);

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
