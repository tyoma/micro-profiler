#include <collector/image_patch.h>
#include <collector/binary_image.h>

#include "mocks.h"

#include <test-helpers/helpers.h>
#include <ut/assert.h>
#include <ut/test.h>

using namespace std;

namespace micro_profiler
{
	namespace tests
	{
		namespace
		{
			bool all(const function_body &)
			{	return true;	}
		}

		begin_test_suite( ImagePatchTests )
			mocks::logged_hook_events trace[2];
			auto_ptr<image> images[2];
			void (*f11)();
			void (*f12)();
			void (*f13)(void (*&f1)(), void (*&f2)());
			void (*f21)(int * volatile begin, int * volatile end);
			void (*f22)(void (*&f)(int * volatile begin, int * volatile end));

			init( LoadGuineas )
			{
				images[0].reset(new image(L"symbol_container_1"));
				f13 = images[0]->get_symbol<void (void (*&f1)(), void (*&f2)())>("get_function_addresses_1");
				f13(f11, f12);

				images[1].reset(new image(L"symbol_container_2"));
				f22 = images[1]->get_symbol<void (void (*&f)(int * volatile begin, int * volatile end))>("bubble_sort_expose");
				f22(f21);
			}

			test( FunctionsArePatchedForAlwaysTrueFilter )
			{
				// INIT
				void (*ff11)();
				void (*ff12)();

				// INIT / ACT
				image_patch ip1(load_image_at(reinterpret_cast<void *>(images[0]->load_address())), &trace[0],
					&mocks::on_enter, &mocks::on_exit);
				image_patch ip2(load_image_at(reinterpret_cast<void *>(images[1]->load_address())), &trace[1],
					&mocks::on_enter, &mocks::on_exit);
				int data[1234];

				ip1.apply_for(&all);
				ip2.apply_for(&all);

				// ACT
				f13(ff11, ff12);

				// ASSERT
				assert_equal(f11, ff11);
				assert_equal(f12, ff12);

				call_record reference1[] = {
					{ 0, address_cast_hack<const void *>(f13) },
					{ 0, 0 },
				};

				assert_equal(reference1, trace[0].call_log);
				assert_is_empty(trace[1].call_log);

				// ACT
				f21(data, array_end(data));

				// ASSERT
				call_record reference2[] = {
					{ 0, address_cast_hack<const void *>(f21) },
					{ 0, 0 },
				};

				assert_equal(reference1, trace[0].call_log);
				assert_equal(reference2, trace[1].call_log);
			}

		end_test_suite
	}
}
