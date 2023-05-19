#include "helpers.h"

#include <common/module.h>
#include <ut/assert.h>

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
					auto l = module::platform().lock_at(address);
					auto r = find_if(l->regions.begin(), l->regions.end(), [&] (const mapped_region &r) {
						return r.address <= address && address < r.address + r.size; 
					});

					assert_not_equal(l->regions.end(), r);
					return byte_range(r->address, r->size);
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
