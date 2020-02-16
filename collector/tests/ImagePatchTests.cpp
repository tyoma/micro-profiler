#include <collector/image_patch.h>

#include "mocks_image_info.h"

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
			bool all(const symbol_info_mapped &)
			{	return true;	}
		}

		begin_test_suite( ImagePatchTests )
			mocks::trace_events trace[2];
			auto_ptr<image> images[2];
			shared_ptr< image_info<symbol_info_mapped> > image_infos[2];
			void (*f11)();
			void (*f12)();
			void (*f13)(char *buffer0, int value);
			void (*f1F)(void (*&f1)(), void (*&f2)());
			void (*f21)(int * volatile begin, int * volatile end);
			int (*f22)(char *buffer, size_t count, const char *format, ...);
			void (*f23)(void (*&f1)(), void (*&f2)(), void (*&f3)());
			void (*f2F)(void (*&f)(int * volatile begin, int * volatile end));

			init( FailX64 )
			{
				assert_equal(4u, sizeof(void*));
			}

			init( LoadGuineas )
			{
				images[0].reset(new image("symbol_container_1"));
				image_infos[0].reset(new offset_image_info(load_image_info(images[0]->absolute_path()),
					static_cast<size_t>(images[0]->base())));
				f1F = images[0]->get_symbol<void (void (*&f1)(), void (*&f2)())>("get_function_addresses_1");
				f13 = images[0]->get_symbol<void (char *buffer0, int value)>("format_decimal");
				f1F(f11, f12);

				images[1].reset(new image("symbol_container_2"));
				image_infos[1].reset(new offset_image_info(load_image_info(images[1]->absolute_path()),
					static_cast<size_t>(images[1]->base())));
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
				image_patch ip1(image_infos[0], &trace[0]);
				image_patch ip2(image_infos[1], &trace[1]);
				int data[1234];
				char buffer[1000] = { 0 };

				ip1.apply_for(&all);
				ip2.apply_for(&all);

				// ACT
				f1F(ff11, ff12);

				// ASSERT
				assert_equal(f11, ff11);
				assert_equal(f12, ff12);

				mocks::call_record reference1[] = {
					{ 0, address_cast_hack<const void *>(f1F) },
					{ 0, 0 },
				};

				assert_equal(reference1, trace[0].call_log);
				assert_is_empty(trace[1].call_log);

				// ACT
				f21(data, array_end(data));

				// ASSERT
				mocks::call_record reference2[] = {
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
				image_patch ip1(image_infos[0], &trace[0]);
				image_patch ip2(image_infos[1], &trace[1]);
				char buffer[1000] = { 0 };
				int data[10];

				ip1.apply_for([] (const symbol_info_mapped &fb) {
					return fb.name == "format_decimal";
				});
				ip2.apply_for([] (const symbol_info_mapped &fb) {
					return fb.name == "get_function_addresses_2" || fb.name == "bubble_sort";
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


			test( FunctionIsNotHookedIfAnExceptionThrownWhileFiltering )
			{
				// INIT
				void (*ff1)();
				void (*ff2)();
				void (*ff3)();
				void (*ff4)(int * volatile begin, int * volatile end);

				// INIT / ACT
				auto_ptr<image_patch> ip(new image_patch(image_infos[1], &trace[0]));
				int data[10];

				f2F(ff4);

				ip->apply_for([] (const symbol_info_mapped &fb) -> bool {
					if (fb.name != "get_function_addresses_2")
						return true;
					throw runtime_error("");
				});

				// ACT
				f23(ff1, ff2, ff3);
				ff4(data, array_end(data));

				// ASSERT
				mocks::call_record reference1[] = {
					{ 0, address_cast_hack<const void *>(ff4) },
					{ 0, 0 },
				};

				assert_equal(reference1, trace[0].call_log);

				// INIT
				ip.reset(new image_patch(image_infos[1], &trace[1]));

				ip->apply_for([] (const symbol_info_mapped &fb) -> bool {
					if (fb.name != "bubble_sort")
						return true;
					throw runtime_error("");
				});

				// ACT
				f23(ff1, ff2, ff3);
				ff4(data, array_end(data));

				// ASSERT
				mocks::call_record reference2[] = {
					{ 0, address_cast_hack<const void *>(f23) },
					{ 0, 0 },
				};

				assert_equal(reference2, trace[1].call_log);
			}


			test( DuplicatedFunctionsAreOnlyPatchedOnce )
			{
				// INIT
				shared_ptr<mocks::image_info> info(new mocks::image_info);
				image_patch ip(info, &trace[0]);
				char buffer[100];

				info->add_function(f13);
				info->add_function(f13);
				ip.apply_for(&all);

				// ACT
				f13(buffer, 123322);

				// ASSERT
				mocks::call_record reference[] = {
					{ 0, address_cast_hack<const void *>(f13) },
					{ 0, 0 },
				};

				assert_equal(reference, trace[0].call_log);
			}


			test( FunctionsAreOnlyPatchedOnce )
			{
				// INIT
				auto_ptr<image_patch> ip(new image_patch(image_infos[1], &trace[0]));
				int data[10];
				char buffer[100] = { 0 };

				ip->apply_for([] (const symbol_info_mapped &symbol) {
					return symbol.name == "bubble_sort" || symbol.name == "guinea_snprintf";
				});

				// ACT
				ip->apply_for([] (const symbol_info_mapped &symbol) {
					return symbol.name == "bubble_sort" || symbol.name == "guinea_snprintf";
				});

				// ACT / ASSERT
				f22(buffer, sizeof(buffer), "%d", 1234);
				f21(data, array_end(data));

				// ASSERT
				assert_equal(4u, trace[0].call_log.size());

				// ACT / ASSERT
				f22(buffer, sizeof(buffer), "%d", 7751);

				// ASSERT
				assert_equal(6u, trace[0].call_log.size());
			}


			test( PatchesAreRemovedWhenUnmatchedFromFilter )
			{
				// INIT
				auto_ptr<image_patch> ip(new image_patch(image_infos[1], &trace[0]));
				int data[10];
				char buffer[100] = { 0 };

				ip->apply_for([] (const symbol_info_mapped &symbol) {
					return symbol.name == "bubble_sort" || symbol.name == "guinea_snprintf";
				});

				// ACT
				ip->apply_for([] (const symbol_info_mapped &symbol) {
					return symbol.name == "bubble_sort";
				});

				// ACT / ASSERT
				f22(buffer, sizeof(buffer), "%d", 1234);
				f21(data, array_end(data));

				// ASSERT
				mocks::call_record reference1[] = {
					{ 0, address_cast_hack<const void *>(f21) },
					{ 0, 0 },
				};

				assert_equal(reference1, trace[0].call_log);

				// INIT
				ip->apply_for([] (const symbol_info_mapped &symbol) {
					return symbol.name == "bubble_sort" || symbol.name == "guinea_snprintf";
				});
				trace[0].call_log.clear();

				// ACT
				ip->apply_for([] (const symbol_info_mapped &symbol) {
					return symbol.name == "guinea_snprintf";
				});

				// ACT / ASSERT
				f22(buffer, sizeof(buffer), "%d", 1234);
				f21(data, array_end(data));

				// ASSERT
				mocks::call_record reference2[] = {
					{ 0, address_cast_hack<const void *>(f22) },
					{ 0, 0 },
				};

				assert_equal(reference2, trace[0].call_log);
			}
		end_test_suite
	}
}
