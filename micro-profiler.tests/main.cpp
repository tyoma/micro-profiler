#include <algorithm>
#include <common/constants.h>
#include <common/string.h>
#include <common/time.h>
#include <test-helpers/helpers.h>

using namespace std;

namespace micro_profiler
{
	namespace tests
	{
		guid_t c_mock_frontend_id;

		namespace
		{
			struct initializer
			{
				initializer()
				{
					c_mock_frontend_id = generate_id();
					setenv(c_frontend_id_env, to_string(c_mock_frontend_id).c_str(), 1);
				}
			} c_initializer;
		}
	}
}
