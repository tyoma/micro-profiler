#include <patcher/image_patch_manager.h>

#include "mocks.h"

#include <common/memory.h>
#include <common/module.h>
#include <test-helpers/constants.h>
#include <test-helpers/helpers.h>
#include <test-helpers/temporary_copy.h>
#include <ut/assert.h>
#include <ut/test.h>

using namespace std;

namespace micro_profiler
{
	namespace tests
	{
		namespace
		{
			class detacher : noncopyable
			{
			public:
				detacher(image_patch_manager &m)
					: _manager(m)
				{	}

				~detacher()
				{	_manager.detach_all();	}

			private:
				image_patch_manager &_manager;
			};
		}

		namespace mocks
		{
			class executable_memory_allocator : public micro_profiler::executable_memory_allocator
			{
			public:
				executable_memory_allocator()
					: allocated(0)
				{	}

				virtual shared_ptr<void> allocate(size_t size) override
				{
					auto real_ptr = micro_profiler::executable_memory_allocator::allocate(size);

					allocated++;
					return shared_ptr<void>(real_ptr.get(), [this, real_ptr] (void *) {
						allocated--;
					});
				}

			public:
				unsigned int allocated;
			};
		}

		begin_test_suite( ImagePatchManagerTests )
			mocks::executable_memory_allocator allocator;
			mocks::trace_events trace;
			unique_ptr<temporary_copy> module1, module2;
			vector<unsigned int> result;

			init( PrepareGuinies )
			{
				module1.reset(new temporary_copy(c_symbol_container_1));
				module2.reset(new temporary_copy(c_symbol_container_2));
			}


			test( ApplyingAPatchRetainsModule )
			{
				// INIT / ACT
				auto unloaded1 = false;
				auto unloaded2 = false;
				image_patch_manager m(trace, allocator);
				detacher dd(m);
				unique_ptr<image> image1(new image(module1->path()));
				unique_ptr<image> image2(new image(module2->path()));

				unsigned int functions1[] = {
					image1->get_symbol_rva("get_function_addresses_1"),
				};
				unsigned int functions2[] = {
					image2->get_symbol_rva("bubble_sort_expose"),
					image2->get_symbol_rva("bubble_sort2"),
				};

				image1->get_symbol<void (bool &unloaded)>("track_unload")(unloaded1);
				image2->get_symbol<void (bool &unloaded)>("track_unload")(unloaded2);

				// ACT
				m.apply(result, 13, image1->base_ptr(), load_library(module1->path()), mkrange(functions1));
				m.apply(result, 17, image2->base_ptr(), load_library(module2->path()), mkrange(functions2));
				image1.reset();

				// ASSERT
				assert_is_empty(result);
				assert_is_false(unloaded1);
				assert_is_false(unloaded2);
				assert_equal(3u, allocator.allocated);

				// ACT
				image2.reset();

				// ASSERT
				assert_is_false(unloaded1);
				assert_is_false(unloaded2);
			}


			test( ApplyingAnEmptyPatchDoesNotRetainModule )
			{
				// INIT / ACT
				auto unloaded = false;
				image_patch_manager m(trace, allocator);
				detacher dd(m);
				unique_ptr<image> image1(new image(module1->path()));

				image1->get_symbol<void (bool &unloaded)>("track_unload")(unloaded);

				// ACT
				m.apply(result, 13, image1->base_ptr(), load_library(module1->path()), range<unsigned, size_t>(0, 0));
				image1.reset();

				// ASSERT
				assert_is_true(unloaded);
				assert_equal(0u, allocator.allocated);
			}


			test( RevertingAllPatchesReleasesAModule )
			{
				// INIT
				auto unloaded1 = false;
				auto unloaded2 = false;
				image_patch_manager m(trace, allocator);
				detacher dd(m);
				unique_ptr<image> image1(new image(module1->path()));
				unique_ptr<image> image2(new image(module2->path()));

				unsigned int functions1[] = {
					image1->get_symbol_rva("get_function_addresses_1"),
					image1->get_symbol_rva("format_decimal"),
				};
				unsigned int functions2[] = {
					image2->get_symbol_rva("get_function_addresses_2"),
					image2->get_symbol_rva("guinea_snprintf"),
					image2->get_symbol_rva("function_with_a_nested_call_2"),
					image2->get_symbol_rva("bubble_sort_expose"),
					image2->get_symbol_rva("bubble_sort2"),
				};

				image1->get_symbol<void (bool &unloaded)>("track_unload")(unloaded1);
				image2->get_symbol<void (bool &unloaded)>("track_unload")(unloaded2);

				m.apply(result, 13, image1->base_ptr(), load_library(module1->path()), mkrange(functions1));
				m.apply(result, 19, image2->base_ptr(), load_library(module2->path()), mkrange(functions2));
				image1.reset();
				image2.reset();

				unsigned int remove21[] = {
					functions2[0], functions2[3],
				};

				// ACT
				m.revert(result, 19, mkrange(remove21));

				// ASSERT
				assert_is_false(unloaded1);
				assert_is_false(unloaded2);
				assert_equal(7u, allocator.allocated);

				// INIT
				unsigned int remove11[] = {
					functions1[1],
				};
				unsigned int remove22[] = {
					functions2[1], functions2[2], functions2[4],
				};

				// ACT
				m.revert(result, 13, mkrange(remove11));
				m.revert(result, 19, mkrange(remove22));

				// ASSERT
				assert_is_false(unloaded1);
				assert_is_true(unloaded2);

				// INIT
				unsigned int remove12[] = {
					functions1[0],
				};

				// ACT
				m.revert(result, 13, mkrange(remove12));

				// ASSERT
				assert_is_true(unloaded1);
				assert_equal(7u, allocator.allocated);
			}


			test( ApplyingNewPatchRetainsAModule )
			{
				// INIT
				auto unloaded = false;
				image_patch_manager m(trace, allocator);
				detacher dd(m);
				unique_ptr<image> image2(new image(module2->path()));
				auto image2_ptr = image2->base_ptr();
				unsigned int functions1[] = {
					image2->get_symbol_rva("get_function_addresses_2"),
					image2->get_symbol_rva("guinea_snprintf"),
					image2->get_symbol_rva("function_with_a_nested_call_2"),
				};
				unsigned int functions2[] = {
					image2->get_symbol_rva("bubble_sort_expose"),
				};
				unsigned int remove1[] = {
					functions1[0], functions1[2],
				};
				unsigned int remove2[] = {
					functions1[1],
				};

				image2->get_symbol<void (bool &unloaded)>("track_unload")(unloaded);

				m.apply(result, 190, image2_ptr, load_library(module2->path()), mkrange(functions1));
				image2.reset();
				m.revert(result, 190, mkrange(remove1));

				// ACT
				m.apply(result, 190, image2_ptr, load_library(module2->path()), mkrange(functions2));
				m.revert(result, 190, mkrange(remove2));

				// ASSERT
				assert_is_empty(result);
				assert_is_false(unloaded);
			}


			test( RevertingTheSamePatchTwiceDoesNotDereferenceAModule )
			{
				// INIT
				auto unloaded = false;
				image_patch_manager m(trace, allocator);
				detacher dd(m);
				unique_ptr<image> image2(new image(module2->path()));
				auto image2_ptr = image2->base_ptr();
				unsigned int functions1[] = {
					image2->get_symbol_rva("get_function_addresses_2"),
					image2->get_symbol_rva("guinea_snprintf"),
					image2->get_symbol_rva("function_with_a_nested_call_2"),
					image2->get_symbol_rva("bubble_sort2"),
				};
				unsigned int remove1[] = {
					functions1[0], functions1[1],
				};

				result.push_back(1717123); // just to make sure, results are appended
				image2->get_symbol<void (bool &unloaded)>("track_unload")(unloaded);

				m.apply(result, 190, image2_ptr, load_library(module2->path()), mkrange(functions1));
				image2.reset();
				m.revert(result, 190, mkrange(remove1));

				// ACT
				m.revert(result, 190, mkrange(remove1));

				// ASSERT
				unsigned int reference1[] = {
					1717123, functions1[0], functions1[1],
				};
				assert_is_false(unloaded);
				assert_equivalent(reference1, result);

				// INIT
				unsigned int remove2[] = {
					functions1[1], functions1[3],
				};

				// ACT
				m.revert(result, 190, mkrange(remove2));

				// ASSERT
				unsigned int reference2[] = {
					1717123, functions1[0], functions1[1], functions1[1],
				};
				assert_is_false(unloaded);
				assert_equivalent(reference2, result);

				// ACT
				m.revert(result, 190, mkrange(functions1));

				// ASSERT
				unsigned int reference3[] = {
					1717123, functions1[0], functions1[1], functions1[1], functions1[0], functions1[1], functions1[3],
				};
				assert_is_true(unloaded);
				assert_equivalent(reference3, result);
			}


			test( ApplyTheSamePatchTwiceIncrementsAReferenceOnce )
			{
				// INIT
				auto unloaded = false;
				image_patch_manager m(trace, allocator);
				detacher dd(m);
				unique_ptr<image> image2(new image(module2->path()));
				auto image2_ptr = image2->base_ptr();
				unsigned int functions1[] = {
					image2->get_symbol_rva("get_function_addresses_2"),
					image2->get_symbol_rva("guinea_snprintf"),
					image2->get_symbol_rva("function_with_a_nested_call_2"),
					image2->get_symbol_rva("bubble_sort2"),
				};
				unsigned int functions2[] = {
					image2->get_symbol_rva("get_function_addresses_2"),
					image2->get_symbol_rva("bubble_sort_expose"),
					image2->get_symbol_rva("bubble_sort2"),
					image2->get_symbol_rva("function_with_a_nested_call_2"),
				};
				unsigned int remove1[] = {
					functions1[0], functions1[1], functions1[2], functions1[3], functions2[1],
				};

				result.push_back(17123); // just to make sure, results are appended
				image2->get_symbol<void (bool &unloaded)>("track_unload")(unloaded);
				m.apply(result, 179, image2->base_ptr(), load_library(module2->path()), mkrange(functions1));
				image2.reset();

				// ACT
				m.apply(result, 179, image2_ptr, load_library(module2->path()), mkrange(functions2));

				// ASSERT
				unsigned int reference[] = {
					17123, functions2[0], functions2[2], functions2[3],
				};

				assert_equivalent(reference, result);
				assert_is_false(unloaded);

				// ACT
				m.revert(result, 179, mkrange(remove1));

				// ASSERT
				assert_equivalent(reference, result);
				assert_is_true(unloaded);
			}


			test( CallsToPatchedFunctionsLeaveTrace )
			{
				// INIT
				image image2(module2->path());
				image_patch_manager m(trace, allocator);
				detacher dd(m);
				void (*ff[3])();
				char buffer[100];
				int digits[] = {	14, 3233, 1, 19,	};
				auto get_function_addresses_2 = image2.get_symbol<void (void (*&)(), void (*&)(), void (*&)())>("get_function_addresses_2");
				auto guinea_snprintf = image2.get_symbol<int (char *buffer, size_t count, const char *format, ...)>("guinea_snprintf");
				auto function_with_a_nested_call_2 = image2.get_symbol<void ()>("function_with_a_nested_call_2");
				auto bubble_sort2 = image2.get_symbol<void (int * volatile begin, int * volatile end)>("bubble_sort2");
				unsigned int functions1[] = {
					image2.get_symbol_rva("get_function_addresses_2"),
					image2.get_symbol_rva("guinea_snprintf"),
					image2.get_symbol_rva("bubble_sort2"),
				};

				m.apply(result, 11, image2.base_ptr(), load_library(module2->path()), mkrange(functions1));

				// ACT
				get_function_addresses_2(ff[0], ff[1], ff[2]);

				// ASSERT
				assert_is_true(2u <= trace.call_log.size());

				// INIT
				trace.call_log.clear();

				// ACT
				guinea_snprintf(buffer, sizeof(buffer), "%d", 1318);

				// ASSERT
				assert_is_true(2u <= trace.call_log.size());

				// INIT
				trace.call_log.clear();

				// ACT
				function_with_a_nested_call_2();

				// ASSERT
				assert_is_empty(trace.call_log);

				// INIT
				trace.call_log.clear();

				// ACT
				bubble_sort2(digits, array_end(digits));

				// ASSERT
				assert_is_true(2u <= trace.call_log.size());

				// INIT
				unsigned int functions2[] = {
					image2.get_symbol_rva("function_with_a_nested_call_2"),
				};

				trace.call_log.clear();
				m.apply(result, 11, image2.base_ptr(), load_library(module2->path()), mkrange(functions2));

				// ACT
				function_with_a_nested_call_2();

				// ASSERT
				assert_is_true(2u <= trace.call_log.size());

				// INIT
				unsigned int remove1[] = {
					functions1[1], functions1[2],
				};

				trace.call_log.clear();

				// ACT
				m.revert(result, 11, mkrange(remove1));

				// ACT
				guinea_snprintf(buffer, sizeof(buffer), "%d", 1318);
				bubble_sort2(digits, array_end(digits));

				// ASSERT
				assert_is_empty(trace.call_log);
			}
		end_test_suite
	}
}
