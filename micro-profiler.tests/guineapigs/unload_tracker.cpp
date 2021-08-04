#include "export.h"

#include <vector>

namespace
{
	std::vector<bool *> g_unload_flags;

	class unload_tracker
	{
	public:
		~unload_tracker()
		{
			for (auto i = g_unload_flags.begin(); i != g_unload_flags.end(); ++i)
				**i = true;
		}
	} g_tracker;
}

extern "C" PUBLIC void track_unload(bool &unloaded)
{	g_unload_flags.push_back(&unloaded);	}

