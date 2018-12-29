#include "mocks.h"

using namespace std;
using namespace std::placeholders;

namespace micro_profiler
{
	namespace tests
	{
		namespace mocks
		{
			Tracer::Tracer()
				: calls_collector(10000)
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
		}
	}
}
