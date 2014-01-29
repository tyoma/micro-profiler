#include "Mockups.h"

#include "Helpers.h"

#include <_generated/frontend.h>
#include <common/com_helpers.h>
#include <stdexcept>

namespace std
{
	using tr1::bind;
	using tr1::ref;
	using namespace tr1::placeholders;
}

using wpl::mt::thread;
using namespace std;

namespace micro_profiler
{
	namespace tests
	{
		namespace mockups
		{
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

				STDMETHODIMP Initialize(long process_id, hyper ticks_resolution);
				STDMETHODIMP LoadImages(long count, ImageLoadInfo *images);
				STDMETHODIMP UpdateStatistics(long count, FunctionStatisticsDetailed *statistics);
				STDMETHODIMP UnloadImages(long count, hyper *image_addresses);

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

			STDMETHODIMP Frontend::Initialize(long process_id, hyper ticks_resolution)
			{
				_state.process_id = process_id;
				_state.ticks_resolution = ticks_resolution;
				if (_state.oninitialized)
					_state.oninitialized();
				return S_OK;
			}

			STDMETHODIMP Frontend::LoadImages(long count, ImageLoadInfo *images)
			{
				_state.update_log.resize(_state.update_log.size() + 1);
				FrontendState::ReceivedEntry &e = _state.update_log.back();
				
				for (; count; ++images, --count)
				{
					e.image_loads.push_back(make_pair(static_cast<uintptr_t>(images->Address), images->Path));

					wstring &path = e.image_loads.back().second;

					toupper(path);
					if (::SysStringLen(images->Path) != path.size())
						path.clear();
				}
				_state.modules_state_updated.raise();
				return S_OK;
			}

			STDMETHODIMP Frontend::UpdateStatistics(long count, FunctionStatisticsDetailed *data)
			{
				_state.update_log.resize(_state.update_log.size() + 1);
				FrontendState::ReceivedEntry &e = _state.update_log.back();

				for (; count; --count, ++data)
					e.update[reinterpret_cast<const void *>(data->Statistics.FunctionAddress)] += *data;
				_state.updated.raise();
				_state.update_lock.wait();
				return S_OK;
			}

			STDMETHODIMP Frontend::UnloadImages(long count, hyper *image_addresses)
			{
				_state.update_log.resize(_state.update_log.size() + 1);
				FrontendState::ReceivedEntry &e = _state.update_log.back();

				for (; count; ++image_addresses, --count)
					e.image_unloads.push_back(static_cast<uintptr_t>(*image_addresses));
				_state.modules_state_updated.raise();
				return S_OK;
			}


			FrontendState::FrontendState(const function<void()>& oninitialized_)
				: update_lock(true, false), oninitialized(oninitialized_), process_id(0), ticks_resolution(0),
					updated(false, true), modules_state_updated(false, true), released(false)
			{	}


			Tracer::Tracer(__int64 latency)
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

			__int64 Tracer::profiler_latency() const throw()
			{	return _latency;	}
		}
	}
}
