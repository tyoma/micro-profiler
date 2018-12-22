#include <common/constants.h>
#include <test-helpers/helpers.h>

namespace micro_profiler
{
	namespace tests
	{
		namespace
		{
			struct initializer
			{
				initializer()
				{
					setenv(c_frontend_id_env, "sockets|127.0.0.1:6111", 1);
				}
			} c_initializer;
		}
	}
}
