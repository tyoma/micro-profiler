#include "helpers.h"

#include <ut/assert.h>
#include <windows.h>

using namespace std;

namespace micro_profiler
{
	namespace tests
	{
		namespace
		{
			class unlocked_code_segment
			{
			public:
				unlocked_code_segment(void *address)
					: _owned(get_range_for(address)), _unprotect(_owned), _original(begin(_owned), end(_owned))
				{	}

				~unlocked_code_segment()
				{	copy(begin(_original), end(_original), begin(_owned));	}

			private:
				static byte_range get_range_for(void *address)
				{
					MEMORY_BASIC_INFORMATION mi;

					assert_is_true(!!::VirtualQuery(address, &mi, sizeof mi));
					return byte_range(static_cast<byte *>(mi.BaseAddress), mi.RegionSize);
				}

			private:
				byte_range _owned;
				scoped_unprotect _unprotect;
				vector<byte> _original;
			};
		}

		shared_ptr<void> temporary_unlock_code_at(void *address)
		{	return make_shared<unlocked_code_segment>(address);	}
	}
}
