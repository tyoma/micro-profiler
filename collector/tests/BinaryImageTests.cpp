#include <collector/binary_image.h>
#include <collector/binary_translation.h>
#include <collector/allocator.h>

#include <test-helpers/helpers.h>

#include <map>
#include <ut/assert.h>
#include <ut/test.h>

using namespace std;
using namespace placeholders;

namespace micro_profiler
{
	namespace tests
	{
		namespace
		{
			void insert_name_size(map<string, size_t> &container, const function_body &fn)
			{	container.insert(make_pair(fn.name(), fn.size()));	}

			void insert_name_address(map<string, void *> &container, const function_body &fn)
			{	container.insert(make_pair(fn.name(), fn.effective_address()));	}

			shared_ptr<void> copy(const_byte_range original, const void *base)
			{
				executable_memory_allocator em;
				shared_ptr<void> ptr = em.allocate(original.length());

				move_function(static_cast<byte *>(ptr.get()), static_cast<const byte *>(base), original);
				return ptr;
			}
		}


		begin_test_suite( BinaryImageTests )
			test( LoadingImageReturnsNonNullPointer )
			{
				// INIT / ACT
				image img1(L"symbol_container_1");
				image img2(L"symbol_container_2");

				// INIT / ACT
				shared_ptr<binary_image> limg1 = load_image_at((void *)img1.load_address());
				shared_ptr<binary_image> limg2 = load_image_at((void *)img1.load_address());

				// ASSERT
				assert_not_null(limg1);
				assert_not_null(limg2);
			}


			test( LoadedImageEnumeratesItsExportedFunctions )
			{
				// INIT
				image img1(L"symbol_container_1");
				image img2(L"symbol_container_2");
				shared_ptr<binary_image> limg1 = load_image_at((void *)img1.load_address());
				shared_ptr<binary_image> limg2 = load_image_at((void *)img2.load_address());
				map<string, size_t> names1, names2;
				map<string, void *> names1_, names2_;

				// ACT
				limg1->enumerate_functions(bind(&insert_name_size, ref(names1), _1));
				limg2->enumerate_functions(bind(&insert_name_size, ref(names2), _1));
				limg1->enumerate_functions(bind(&insert_name_address, ref(names1_), _1));
				limg2->enumerate_functions(bind(&insert_name_address, ref(names2_), _1));

				// ASSERT
				assert_equal(1u, names1.count("get_function_addresses_1"));
				assert_equal(1u, names2.count("get_function_addresses_2"));
				assert_equal(1u, names2.count("function_with_a_nested_call_2"));
				assert_equal(img1.get_symbol_address("get_function_addresses_1"), names1_["get_function_addresses_1"]);
				assert_equal(img2.get_symbol_address("get_function_addresses_2"), names2_["get_function_addresses_2"]);
				assert_equal(img2.get_symbol_address("function_with_a_nested_call_2"), names2_["function_with_a_nested_call_2"]);
			}


			test( LoadedImageEnumeratesItsInternalFunctions )
			{
				// INIT
				image img(L"symbol_container_2");
				shared_ptr<binary_image> limg = load_image_at((void *)img.load_address());
				map<string, size_t> names;

				// ACT
				limg->enumerate_functions(bind(&insert_name_size, ref(names), _1));

				// ASSERT
				assert_equal(1u, names.count("vale_of_mean_creatures::this_one_for_the_birds"));
				assert_equal(1u, names.count("vale_of_mean_creatures::this_one_for_the_whales"));
				assert_equal(1u, names.count("vale_of_mean_creatures::the_abyss::bubble_sort"));
				assert_equal(1u, names.count("bubble_sort"));
			}


			test( SizeIsProperlyReturned )
			{
				// INIT
				image img(L"symbol_container_2");
				shared_ptr<binary_image> limg = load_image_at((void *)img.load_address());
				map<string, size_t> names;

				// ACT
				limg->enumerate_functions(bind(&insert_name_size, ref(names), _1));

				// ASSERT
				assert_is_true(names["get_function_addresses_2"] > names["vale_of_mean_creatures::this_one_for_the_birds"]);
				assert_is_true(names["bubble_sort"] > names["get_function_addresses_2"]);
			}


			static void copy_specific(const function_body &fn, const char *name, shared_ptr<void> &clone)
			{
				if (fn.name() == name)
					clone = copy(fn.body(), fn.effective_address());
			}

			test( IndependentFreeFunctionCanBeCopiedAndCalled )
			{
				typedef void (fn_t)(int * volatile begin, int * volatile end);

				// INIT
				image img(L"symbol_container_2");
				shared_ptr<binary_image> limg = load_image_at((void *)img.load_address());
				shared_ptr<void> clone;

				// ACT
				limg->enumerate_functions(bind(&copy_specific, _1, "bubble_sort", ref(clone)));

				// ASSERT
				assert_not_null(clone);

				// INIT
				int arr1[] = { 1, 10, 3, 90, 5, };
				int arr2[] = { 1, 5, 4, 3, 5, -10 };
				fn_t *f = address_cast_hack<fn_t *>(clone.get());

				// ACT
				f(arr1, array_end(arr1));
				f(arr2, array_end(arr2));
				
				// ASSERT
				int reference1[] = { 1, 3, 5, 10, 90, };
				int reference2[] = { -10, 1, 3, 4, 5, 5, };

				assert_equal(reference1, arr1);
				assert_equal(reference2, arr2);
			}


			test( FunctionRequiringImageRelocationPatchCanBeCopiedAndCalled )
			{
				typedef void (ret_fn_t)();
				typedef void (fn_t)(ret_fn_t *&f1, ret_fn_t *&f2, ret_fn_t *&f3);

				// INIT
				image img(L"symbol_container_2");
				shared_ptr<binary_image> limg = load_image_at((void *)img.load_address());
				shared_ptr<void> clone;
				fn_t *original = img.get_symbol<fn_t>("get_function_addresses_2");
				ret_fn_t *values[3];

				// ACT
				limg->enumerate_functions(bind(&copy_specific, _1, "get_function_addresses_2", ref(clone)));
				fn_t *f = address_cast_hack<fn_t *>(clone.get());
				f(values[0], values[1], values[2]);

				// ASSERT
				ret_fn_t *reference[3];

				original(reference[0], reference[1], reference[2]);
				assert_equal(reference, values);
			}


			test( FunctionRequiringCallsRelocationCanBeCopiedAndCalled )
			{
				typedef void (fn_t)(int * volatile begin, int * volatile end);

				// INIT
				image img(L"symbol_container_2");
				shared_ptr<binary_image> limg = load_image_at((void *)img.load_address());
				shared_ptr<void> clone;

				// ACT
				limg->enumerate_functions(bind(&copy_specific, _1, "bubble_sort2", ref(clone)));

				// ASSERT
				assert_not_null(clone);

				// INIT
				int arr1[] = { 1, 10, 3, 90, 5, 11, };
				int arr2[] = { 1, 5, 4, 3, 5, -10, 20, };
				fn_t *f = address_cast_hack<fn_t *>(clone.get());

				// ACT
				f(arr1, array_end(arr1));
				f(arr2, array_end(arr2));
				
				// ASSERT
				int reference1[] = { 1, 3, 5, 10, 11, 90, };
				int reference2[] = { -10, 1, 3, 4, 5, 5, 20, };

				assert_equal(reference1, arr1);
				assert_equal(reference2, arr2);
			}
		end_test_suite
	}
}
