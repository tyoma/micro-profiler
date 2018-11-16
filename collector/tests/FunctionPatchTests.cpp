#include <collector/image_patch.h>
#include <collector/binary_image.h>

#include "mocks.h"

#include <collector/system.h>
#include <test-helpers/helpers.h>
#include <ut/assert.h>
#include <ut/test.h>

using namespace std;
using namespace placeholders;

namespace micro_profiler
{
	namespace tests
	{
		void hash_ints(int *values, size_t n);

		namespace mocks
		{
			const byte c_hash_ints_body_x86[] = {
				0x8B, 0x4C, 0x24, 0x08,	// mov         ecx,dword ptr [esp+8]  
				0x85, 0xC9,					// test        ecx,ecx  
				0x74, 0x18,					// je          micro_profiler::tests::hash_ints+20h (60ABD480h)  
				0x8B, 0x44, 0x24, 0x04,	// mov         eax,dword ptr [esp+4]  
				0x8D, 0x64, 0x24, 0x00,	// lea         esp,[esp]  
				0x8B, 0x10,					// mov         edx,dword ptr [eax]  
				0x69, 0xD2, 0xB1, 0x79, 0x37, 0x9E,	// imul        edx,edx,9E3779B1h  
				0x89, 0x10,					// mov         dword ptr [eax],edx  
				0x83, 0xC0, 0x04,			// add         eax,4  
				0x49,							// dec         ecx  
				0x75, 0xF0,					// jne         micro_profiler::tests::hash_ints+10h (60ABD470h)  
				0xC3,							// ret
			};

			class function_body : public micro_profiler::function_body
			{
				virtual std::string name() const {	throw 0; }
				virtual void *effective_address() const {	return address_cast_hack<void *>(&hash_ints);	}
				virtual const_byte_range body() const {	return const_byte_range(c_hash_ints_body_x86, sizeof(c_hash_ints_body_x86));	}
			};
		}

		begin_test_suite( FunctionPatchTests )
			executable_memory_allocator allocator;
			mocks::logged_hook_events trace;

			init( ReplaceTestedFunction )
			{
				scoped_unprotect unprotect(range<byte>(address_cast_hack<byte *>(&hash_ints), sizeof(mocks::c_hash_ints_body_x86)));
				memcpy(address_cast_hack<byte *>(&hash_ints), mocks::c_hash_ints_body_x86, sizeof(mocks::c_hash_ints_body_x86));
			}

			static void patch_specific(auto_ptr<function_patch> &patch, FunctionPatchTests &self, const function_body &fn, const char *name)
			{
				if (fn.name() == name)
					patch.reset(new function_patch(self.allocator, fn, &self.trace, &mocks::on_enter, &mocks::on_exit));
			}

			test( PatchedFunctionCallsHookCallbacks )
			{
				// INIT
				mocks::function_body fb;
				int data[] = { 100, 22132, 332222, };

				// INIT / ACT
				function_patch patch(allocator, fb,  &trace, &mocks::on_enter, &mocks::on_exit);

				// ACT
				hash_ints(data, 3);

				// ASSERT
				call_record reference1[] = {
					{ 0, address_cast_hack<const void *>(&hash_ints) },
					{ 0, 0 },
				};

				assert_equal(reference1, trace.call_log);

				// ACT
				hash_ints(data, 3);
				hash_ints(data, 3);

				// ASSERT
				call_record reference2[] = {
					{ 0, address_cast_hack<const void *>(&hash_ints) }, { 0, 0 },
					{ 0, address_cast_hack<const void *>(&hash_ints) }, { 0, 0 },
					{ 0, address_cast_hack<const void *>(&hash_ints) }, { 0, 0 },
				};

				assert_equal(reference2, trace.call_log);
			}


			test( DestructionOfPatchCancelsHooking )
			{
				// INIT
				mocks::function_body fb;
				auto_ptr<function_patch> patch(new function_patch(allocator, fb,  &trace, &mocks::on_enter, &mocks::on_exit));
				int data[] = { 100, 22132, 332222, };

				// ACT / ASSERT
				patch.reset();

				// ACT / ASSERT
				hash_ints(data, 3);

				// ASSERT
				assert_is_empty(trace.call_log);
			}
		end_test_suite
	}
}
