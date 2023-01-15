#include <patcher/image_patch_manager.h>

#include "mocks.h"

#include <common/memory.h>
#include <common/module.h>
#include <test-helpers/constants.h>
#include <test-helpers/file_helpers.h>
#include <test-helpers/helpers.h>
#include <ut/assert.h>
#include <ut/test.h>

using namespace std;

namespace micro_profiler
{
	inline bool operator ==(const patch_apply &lhs, const patch_apply &rhs)
	{	return lhs.id == rhs.id && lhs.result == rhs.result;	}

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

			pair<unsigned /*rva*/, patch_apply> mkpatch_apply(unsigned rva, patch_result::errors status, unsigned id)
			{
				patch_apply pa = {	status, id	};
				return make_pair(rva, pa);
			}
		}

		namespace mocks
		{
			class executable_memory_allocator : public micro_profiler::executable_memory_allocator
			{
			public:
				executable_memory_allocator()
					: micro_profiler::executable_memory_allocator(const_byte_range(0, 0), numeric_limits<ptrdiff_t>::max()),
						allocated(0)
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
			temporary_directory dir;
			string module1_path, module2_path;

			patch_manager::apply_results aresult;
			patch_manager::revert_results rresult;

			init( PrepareGuinies )
			{
				module1_path = dir.copy_file(c_symbol_container_1);
				module2_path = dir.copy_file(c_symbol_container_2);
			}


			test( ApplyingAPatchRetainsModule )
			{
				// INIT / ACT
				auto unloaded1 = false;
				auto unloaded2 = false;
				image_patch_manager m_(trace, allocator);
				detacher dd(m_);
				auto &m = static_cast<patch_manager &>(m_);
				unique_ptr<image> image1(new image(module1_path));
				unique_ptr<image> image2(new image(module2_path));

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
				m.apply(aresult, 13, image1->base_ptr(), module::load(module1_path), mkrange(functions1));
				m.apply(aresult, 17, image2->base_ptr(), module::load(module2_path), mkrange(functions2));
				image1.reset();

				// ASSERT
				assert_is_false(unloaded1);
				assert_is_false(unloaded2);
				assert_equal(3u, allocator.allocated);

				pair<unsigned, patch_apply> reference[] = {
					mkpatch_apply(functions1[0], patch_result::ok, 0),
					mkpatch_apply(functions2[0], patch_result::ok, 0),
					mkpatch_apply(functions2[1], patch_result::ok, 0),
				};

				assert_equal(reference, aresult);

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
				image_patch_manager m_(trace, allocator);
				detacher dd(m_);
				auto &m = static_cast<patch_manager &>(m_);
				unique_ptr<image> image1(new image(module1_path));

				image1->get_symbol<void (bool &unloaded)>("track_unload")(unloaded);

				// ACT
				m.apply(aresult, 13, image1->base_ptr(), module::load(module1_path), range<unsigned, size_t>(0, 0));
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
				image_patch_manager m_(trace, allocator);
				detacher dd(m_);
				auto &m = static_cast<patch_manager &>(m_);
				unique_ptr<image> image1(new image(module1_path));
				unique_ptr<image> image2(new image(module2_path));

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

				m.apply(aresult, 13, image1->base_ptr(), module::load(module1_path), mkrange(functions1));
				m.apply(aresult, 19, image2->base_ptr(), module::load(module2_path), mkrange(functions2));
				image1.reset();
				image2.reset();

				unsigned int remove21[] = {
					functions2[0], functions2[3],
				};

				// ACT
				m.revert(rresult, 19, mkrange(remove21));

				// ASSERT
				assert_is_false(unloaded1);
				assert_is_false(unloaded2);
				assert_equal(7u, allocator.allocated);

				pair<unsigned, patch_result::errors> reference1[] = {
					make_pair(functions2[0], patch_result::ok),
					make_pair(functions2[3], patch_result::ok),
				};

				assert_equal(reference1, rresult);

				// INIT
				unsigned int remove11[] = {
					functions1[1],
				};
				unsigned int remove22[] = {
					functions2[1], functions2[2], functions2[4],
				};

				// ACT
				m.revert(rresult, 13, mkrange(remove11));
				m.revert(rresult, 19, mkrange(remove22));

				// ASSERT
				assert_is_false(unloaded1);
				assert_is_true(unloaded2);

				pair<unsigned, patch_result::errors> reference2[] = {
					make_pair(functions2[0], patch_result::ok),
					make_pair(functions2[3], patch_result::ok),

					make_pair(functions1[1], patch_result::ok),
					make_pair(functions2[1], patch_result::ok),
					make_pair(functions2[2], patch_result::ok),
					make_pair(functions2[4], patch_result::ok),
				};

				assert_equal(reference2, rresult);

				// INIT
				unsigned int remove12[] = {
					functions1[0],
				};

				// ACT
				m.revert(rresult, 13, mkrange(remove12));

				// ASSERT
				assert_is_true(unloaded1);
				assert_equal(7u, allocator.allocated);

				pair<unsigned, patch_result::errors> reference3[] = {
					make_pair(functions2[0], patch_result::ok),
					make_pair(functions2[3], patch_result::ok),

					make_pair(functions1[1], patch_result::ok),
					make_pair(functions2[1], patch_result::ok),
					make_pair(functions2[2], patch_result::ok),
					make_pair(functions2[4], patch_result::ok),

					make_pair(functions1[0], patch_result::ok),
				};

				assert_equal(reference3, rresult);
			}


			test( ApplyingNewPatchRetainsAModule )
			{
				// INIT
				auto unloaded = false;
				image_patch_manager m_(trace, allocator);
				detacher dd(m_);
				auto &m = static_cast<patch_manager &>(m_);
				unique_ptr<image> image2(new image(module2_path));
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

				m.apply(aresult, 190, image2_ptr, module::load(module2_path), mkrange(functions1));
				image2.reset();
				m.revert(rresult, 190, mkrange(remove1));

				// ACT
				m.apply(aresult, 190, image2_ptr, module::load(module2_path), mkrange(functions2));
				m.revert(rresult, 190, mkrange(remove2));

				// ASSERT
				assert_is_false(unloaded);
			}


			test( RevertingTheSamePatchTwiceDoesNotDereferenceAModule )
			{
				// INIT
				auto unloaded = false;
				image_patch_manager m_(trace, allocator);
				detacher dd(m_);
				auto &m = static_cast<patch_manager &>(m_);
				unique_ptr<image> image2(new image(module2_path));
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

				image2->get_symbol<void (bool &unloaded)>("track_unload")(unloaded);

				m.apply(aresult, 190, image2_ptr, module::load(module2_path), mkrange(functions1));
				image2.reset();
				m.revert(rresult, 190, mkrange(remove1));
				rresult.clear();

				// ACT
				m.revert(rresult, 190, mkrange(remove1));

				// ASSERT
				assert_is_false(unloaded);

				pair<unsigned, patch_result::errors> reference1[] = {
					make_pair(functions1[0], patch_result::unchanged),
					make_pair(functions1[1], patch_result::unchanged),
				};

				assert_equal(reference1, rresult);

				// INIT
				unsigned int remove2[] = {
					functions1[1], functions1[3],
				};

				// ACT
				m.revert(rresult, 190, mkrange(remove2));

				// ASSERT
				pair<unsigned, patch_result::errors> reference2[] = {
					make_pair(functions1[0], patch_result::unchanged),
					make_pair(functions1[1], patch_result::unchanged),

					make_pair(functions1[1], patch_result::unchanged),
					make_pair(functions1[3], patch_result::ok),
				};

				assert_is_false(unloaded);
				assert_equal(reference2, rresult);

				// ACT
				m.revert(rresult, 190, mkrange(functions1));

				// ASSERT
				pair<unsigned, patch_result::errors> reference3[] = {
					make_pair(functions1[0], patch_result::unchanged),
					make_pair(functions1[1], patch_result::unchanged),

					make_pair(functions1[1], patch_result::unchanged),
					make_pair(functions1[3], patch_result::ok),

					make_pair(functions1[0], patch_result::unchanged),
					make_pair(functions1[1], patch_result::unchanged),
					make_pair(functions1[2], patch_result::ok),
					make_pair(functions1[3], patch_result::unchanged),
				};

				assert_is_true(unloaded);
				assert_equal(reference3, rresult);
			}


			test( ApplyTheSamePatchTwiceIncrementsAReferenceOnce )
			{
				// INIT
				auto unloaded = false;
				image_patch_manager m_(trace, allocator);
				detacher dd(m_);
				auto &m = static_cast<patch_manager &>(m_);
				unique_ptr<image> image2(new image(module2_path));
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

				image2->get_symbol<void (bool &unloaded)>("track_unload")(unloaded);
				m.apply(aresult, 179, image2->base_ptr(), module::load(module2_path), mkrange(functions1));
				image2.reset();
				aresult.clear();

				// ACT
				m.apply(aresult, 179, image2_ptr, module::load(module2_path), mkrange(functions2));

				// ASSERT
				pair<unsigned, patch_apply> reference1[] = {
					mkpatch_apply(functions2[0], patch_result::unchanged, 0),
					mkpatch_apply(functions2[1], patch_result::ok, 0),
					mkpatch_apply(functions2[2], patch_result::unchanged, 0),
					mkpatch_apply(functions2[3], patch_result::unchanged, 0),
				};

				assert_equal(reference1, aresult);
				assert_is_false(unloaded);

				// ACT
				m.revert(rresult, 179, mkrange(remove1));

				// ASSERT
				pair<unsigned, patch_result::errors> reference2[] = {
					make_pair(functions1[0], patch_result::ok),
					make_pair(functions1[1], patch_result::ok),
					make_pair(functions1[2], patch_result::ok),
					make_pair(functions1[3], patch_result::ok),
					make_pair(functions2[1], patch_result::ok),
				};

				assert_equal(reference2, rresult);
				assert_is_true(unloaded);
			}


			test( CallsToPatchedFunctionsLeaveTrace )
			{
				// INIT
				image image2(module2_path);
				image_patch_manager m_(trace, allocator);
				detacher dd(m_);
				auto &m = static_cast<patch_manager &>(m_);
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

				m.apply(aresult, 11, image2.base_ptr(), module::load(module2_path), mkrange(functions1));

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
				bubble_sort2(begin(digits), end(digits));

				// ASSERT
				assert_is_true(2u <= trace.call_log.size());

				// INIT
				unsigned int functions2[] = {
					image2.get_symbol_rva("function_with_a_nested_call_2"),
				};

				trace.call_log.clear();
				m.apply(aresult, 11, image2.base_ptr(), module::load(module2_path), mkrange(functions2));

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
				m.revert(rresult, 11, mkrange(remove1));

				// ACT
				guinea_snprintf(buffer, sizeof(buffer), "%d", 1318);
				bubble_sort2(begin(digits), end(digits));

				// ASSERT
				assert_is_empty(trace.call_log);
			}


			test( FailureToActivateAPatchIsReported )
			{
				// INIT
				image image2(module2_path);
				mocks::trace_events trace2;
				image_patch_manager m1(trace, allocator), m2(trace2, allocator);
				detacher dd1(m1), dd2(m2);
				unsigned int functions1[] = {
					image2.get_symbol_rva("guinea_snprintf"),
					image2.get_symbol_rva("bubble_sort2"),
				};
				unsigned int functions2[] = {
					image2.get_symbol_rva("get_function_addresses_2"),
					image2.get_symbol_rva("guinea_snprintf"),
					image2.get_symbol_rva("function_with_a_nested_call_2"),
					image2.get_symbol_rva("bubble_sort_expose"),
					image2.get_symbol_rva("bubble_sort2"),
				};

				m1.apply(aresult, 11, image2.base_ptr(), module::load(module2_path), mkrange(functions1));
				aresult.clear();

				// ACT
				m2.apply(aresult, 11, image2.base_ptr(), module::load(module2_path), mkrange(functions2));

				// ASSERT
				pair<unsigned, patch_apply> reference1[] = {
					mkpatch_apply(functions2[0], patch_result::ok, 0),
					mkpatch_apply(functions2[1], patch_result::error, 0),
					mkpatch_apply(functions2[2], patch_result::ok, 0),
					mkpatch_apply(functions2[3], patch_result::ok, 0),
					mkpatch_apply(functions2[4], patch_result::error, 0),
				};

				assert_equal(reference1, aresult);

				// INIT
				unsigned int functions3[] = {
					image2.get_symbol_rva("get_function_addresses_2"),
				};

				aresult.clear();

				// ACT
				m1.apply(aresult, 11, image2.base_ptr(), module::load(module2_path), mkrange(functions3));

				// ASSERT
				pair<unsigned, patch_apply> reference2[] = {
					mkpatch_apply(functions3[0], patch_result::error, 0),
				};

				assert_equal(reference2, aresult);
			}
		end_test_suite
	}
}
