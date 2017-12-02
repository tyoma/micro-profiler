#include "Mockups.h"

#include "Helpers.h"

#include <common/serialization.h>

#include <_generated/frontend.h>
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
					buffer_reader(const byte *data, size_t size)
						: _ptr(data), _remaining(size)
					{	}

					void read(void *data, size_t size)
					{
						assert_is_true(size <= _remaining);
						memcpy(data, _ptr, size);
						_ptr += size;
						_remaining -= size;
					}

				private:
					const byte *_ptr;
					size_t _remaining;
				};
			}

			class Frontend : public IProfilerFrontend
			{
			public:
				static void Create(FrontendState& state, IProfilerFrontend **frontend);

			private:
				explicit Frontend(FrontendState& state);
				~Frontend();

				STDMETHODIMP QueryInterface(REFIID riid, void **ppv);
				STDMETHODIMP_(ULONG) AddRef();
				STDMETHODIMP_(ULONG) Release();

				STDMETHODIMP Initialize(const byte *init, long init_size);
				STDMETHODIMP LoadImages(const byte *images, long images_size);
				STDMETHODIMP UpdateStatistics(const byte *statistics, long statistics_size);
				STDMETHODIMP UnloadImages(const byte *images, long images_size);

				const Frontend &operator =(const Frontend &rhs);

			private:
				unsigned int _refcount;
				FrontendState &_state;
			};

			function<void(IProfilerFrontend **)> FrontendState::MakeFactory()
			{	return bind(&Frontend::Create, ref(*this), _1);	}

			Frontend::Frontend(FrontendState &state)
				: _refcount(0), _state(state)
			{	}

			Frontend::~Frontend()
			{	_state.released = true;	}

			void Frontend::Create(FrontendState& state, IProfilerFrontend **frontend)
			{	static_cast<IProfilerFrontend *>(new Frontend(state))->QueryInterface(frontend);	}

			STDMETHODIMP Frontend::QueryInterface(REFIID riid, void **ppv)
			{
				if (riid != IID_IUnknown && riid != __uuidof(IProfilerFrontend))
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

			STDMETHODIMP Frontend::Initialize(const byte *init, long init_size)
			{
				buffer_reader reader(init, init_size);
				strmd::deserializer<buffer_reader> a(reader);

				a(_state.process_init);
				if (_state.oninitialized)
					_state.oninitialized();
				return S_OK;
			}

			STDMETHODIMP Frontend::LoadImages(const byte *images, long images_size)
			{
				buffer_reader reader(images, images_size);
				strmd::deserializer<buffer_reader> a(reader);

				_state.update_log.resize(_state.update_log.size() + 1);
				a(_state.update_log.back().image_loads);
				loaded_modules &image_loads = _state.update_log.back().image_loads;
				for (loaded_modules::iterator i = image_loads.begin(); i != image_loads.end(); ++i)
					toupper(i->second);
				
				_state.modules_state_updated.raise();
				return S_OK;
			}

			STDMETHODIMP Frontend::UpdateStatistics(const byte *statistics, long statistics_size)
			{
				buffer_reader reader(statistics, statistics_size);
				strmd::deserializer<buffer_reader> a(reader);

				_state.update_log.resize(_state.update_log.size() + 1);
				a(_state.update_log.back().update);

				_state.updated.raise();
				_state.update_lock.wait();
				return S_OK;
			}

			STDMETHODIMP Frontend::UnloadImages(const byte *images, long images_size)
			{
				_state.update_log.resize(_state.update_log.size() + 1);
				FrontendState::ReceivedEntry &e = _state.update_log.back();
				buffer_reader reader(images, images_size);
				strmd::deserializer<buffer_reader> a(reader);

				a(e.image_unloads);
				_state.modules_state_updated.raise();
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
