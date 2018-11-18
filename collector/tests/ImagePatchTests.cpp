#include <collector/image_patch.h>
#include <collector/binary_image.h>

#include <patcher/tests/mocks.h>

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
			mocks::trace_events trace[2];
			auto_ptr<image> images[2];
			void (*f11)();
			void (*f12)();
			void (*f13)(char *buffer0, int value);
			void (*f1F)(void (*&f1)(), void (*&f2)());
			void (*f21)(int * volatile begin, int * volatile end);
			int (*f22)(char *buffer, size_t count, const char *format, ...);
			void (*f23)(void (*&f1)(), void (*&f2)(), void (*&f3)());
			void (*f2F)(void (*&f)(int * volatile begin, int * volatile end));

			init( LoadGuineas )
			{
				images[0].reset(new image(L"symbol_container_1"));
				f1F = images[0]->get_symbol<void (void (*&f1)(), void (*&f2)())>("get_function_addresses_1");
				f13 = images[0]->get_symbol<void (char *buffer0, int value)>("format_decimal");
				f1F(f11, f12);

				images[1].reset(new image(L"symbol_container_2"));
				f22 = images[1]->get_symbol<int (char *buffer, size_t count, const char *format, ...)>("guinea_snprintf");
				f23 = images[1]->get_symbol<void (void (*&f1)(), void (*&f2)(), void (*&f3)())>("get_function_addresses_2");
				f2F = images[1]->get_symbol<void (void (*&f)(int * volatile begin, int * volatile end))>("bubble_sort_expose");
				f2F(f21);
			}

			test( FunctionsArePatchedForAlwaysTrueFilter )
			{
				// INIT
				void (*ff11)();
				void (*ff12)();

				// INIT / ACT
				image_patch ip1(load_image_at(reinterpret_cast<void *>(images[0]->load_address())), &trace[0]);
				image_patch ip2(load_image_at(reinterpret_cast<void *>(images[1]->load_address())), &trace[1]);
				int data[1234];
				char buffer[1000] = { 0 };

				ip1.apply_for(&all);
				ip2.apply_for(&all);

				// ACT
				f1F(ff11, ff12);

				// ASSERT
				assert_equal(f11, ff11);
				assert_equal(f12, ff12);

				call_record reference1[] = {
					{ 0, address_cast_hack<const void *>(f1F) },
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

				// ACT
				f22(buffer, sizeof(buffer) - 1, "%d - %s", 777, "lucky");

				// ASSERT
				assert_equal("777 - lucky", string(buffer));
			}


			test( FunctionsAreFilteredBeforeApplyingPatch )
			{
				// INIT
				void (*ff1)();
				void (*ff2)();
				void (*ff3)();

				// INIT / ACT
				image_patch ip1(load_image_at(reinterpret_cast<void *>(images[0]->load_address())), &trace[0]);
				image_patch ip2(load_image_at(reinterpret_cast<void *>(images[1]->load_address())), &trace[1]);
				char buffer[1000] = { 0 };
				int data[10];

				ip1.apply_for([] (const function_body &fb) {
					return fb.name() == "format_decimal";
				});
				ip2.apply_for([] (const function_body &fb) {
					return fb.name() == "get_function_addresses_2" || fb.name() == "bubble_sort";
				});

				// ACT
				f1F(ff1, ff2);

				// ASSERT
				assert_equal(0u, trace[0].call_log.size());

				// ACT
				f13(buffer, 1233229);

				// ASSERT
				assert_equal(2u, trace[0].call_log.size());
				assert_is_empty(trace[1].call_log);
				assert_equal("1233229", string(buffer));

				// ACT
				f22(buffer, sizeof(buffer), "%X", 127);

				// ASSERT
				assert_equal(2u, trace[0].call_log.size());
				assert_is_empty(trace[1].call_log);

				// ACT
				f21(data, array_end(data));

				// ASSERT
				assert_equal(2u, trace[0].call_log.size());
				assert_equal(2u, trace[1].call_log.size());

				// ACT
				f23(ff1, ff2, ff3);

				// ASSERT
				assert_equal(2u, trace[0].call_log.size());
				assert_equal(4u, trace[1].call_log.size());
			}

		end_test_suite
	}
}
