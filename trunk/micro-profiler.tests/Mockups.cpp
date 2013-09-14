#include "Mockups.h"

#include <common/com_helpers.h>
#include <stdexcept>

namespace std
{
	using tr1::bind;
	using tr1::ref;
	using namespace tr1::placeholders;
}

using namespace std;

namespace micro_profiler
{
	namespace tests
	{
		namespace mockups
		{
			function<void(IProfilerFrontend **)> Frontend::MakeFactory(State& state)
			{
				return bind(&Frontend::Create, ref(state), _1);
			}

			Frontend::Frontend(State &state)
				: _refcount(0), _state(state)
			{	}

			Frontend::~Frontend()
			{	_state.released = true;	}

			void Frontend::Create(State& state, IProfilerFrontend **frontend)
			{
				state.creator_thread_id = thread::current_thread_id();
				static_cast<IProfilerFrontend *>(new Frontend(state))->QueryInterface(frontend);
			}

			STDMETHODIMP Frontend::QueryInterface(REFIID riid, void **ppv)
			{
				if (riid != IID_IUnknown && riid != __uuidof(IProfilerFrontend))
					return E_NOINTERFACE;
				*ppv = (IUnknown *)this;
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

			STDMETHODIMP Frontend::Initialize(BSTR executable, hyper load_address, hyper ticks_resolution)
			{
				_state.executable = executable;
				if (::SysStringLen(executable) != _state.executable.size())
					throw invalid_argument("'executable' argument is invalid!");
				_state.load_address = load_address;
				_state.ticks_resolution = ticks_resolution;
				return S_OK;
			}

			STDMETHODIMP Frontend::UpdateStatistics(long count, FunctionStatisticsDetailed *data)
			{
				_state.update_log.push_back(statistics_map_detailed());

				statistics_map_detailed &s = _state.update_log.back();
				for (; count; --count, ++data)
				{
					State::TriggersMap::const_iterator i = _state.triggers.find(data->Statistics.FunctionAddress);

					if (i != _state.triggers.end())
					{
						i->second();
					}
					s[reinterpret_cast<const void *>(data->Statistics.FunctionAddress)] += *data;
				}
				return S_OK;
			}


			Frontend::State::State()
				: creator_thread_id(0), released(false), load_address(0), ticks_resolution(0)
			{	}


			Tracer::Tracer(__int64 latency)
				: _latency(latency)
			{	}

			void Tracer::read_collected(acceptor &a)
			{
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
