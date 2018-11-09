#include <collector/binary_image.h>
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
			void insert(map<string, size_t> &container, const function_body &fn)
			{	container.insert(make_pair(fn.name(), fn.size()));	}
		}

		begin_test_suite( BinaryImageTests )
			test( LoadingImageReturnsNonNullPointer )
			{
				// INIT / ACT
				image img1(L"symbol_container_1");
				image img2(L"symbol_container_2");

				// INIT / ACT
				shared_ptr<binary_image> limg1 = load_image_at((const void *)img1.load_address());
				shared_ptr<binary_image> limg2 = load_image_at((const void *)img1.load_address());

				// ASSERT
				assert_not_null(limg1);
				assert_not_null(limg2);
			}


			test( LoadedImageEnumeratesItsExportedFunctions )
			{
				// INIT
				image img1(L"symbol_container_1");
				image img2(L"symbol_container_2");
				shared_ptr<binary_image> limg1 = load_image_at((const void *)img1.load_address());
				shared_ptr<binary_image> limg2 = load_image_at((const void *)img2.load_address());
				map<string, size_t> names1, names2;

				// ACT
				limg1->enumerate_functions(bind(&insert, ref(names1), _1));
				limg2->enumerate_functions(bind(&insert, ref(names2), _1));

				// ASSERT
				assert_equal(1u, names1.count("get_function_addresses_1"));
				assert_equal(1u, names2.count("get_function_addresses_2"));
				assert_equal(1u, names2.count("function_with_a_nested_call_2"));
			}


			test( LoadedImageEnumeratesItsInternalFunctions )
			{
				// INIT
				image img(L"symbol_container_2");
				shared_ptr<binary_image> limg = load_image_at((const void *)img.load_address());
				map<string, size_t> names;

				// ACT
				limg->enumerate_functions(bind(&insert, ref(names), _1));

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
				shared_ptr<binary_image> limg = load_image_at((const void *)img.load_address());
				map<string, size_t> names;

				// ACT
				limg->enumerate_functions(bind(&insert, ref(names), _1));

				// ASSERT
				assert_is_true(names["get_function_addresses_2"] > names["vale_of_mean_creatures::this_one_for_the_birds"]);
				assert_is_true(names["bubble_sort"] > names["get_function_addresses_2"]);
			}


			static void copy_specific(const function_body &fn, const char *name, ememory_allocator &a, void *&clone)
			{
				if (fn.name() == name)
				{
					clone = a.allocate(fn.size());
					fn.copy_relocate_to(clone);
				}
			}

			test( IndependentFreeFunctionCanBeCopiedAndCalled )
			{
				typedef void (fn_t)(int * volatile begin, int * volatile end);

				// INIT
				ememory_allocator a;
				image img(L"symbol_container_2");
				shared_ptr<binary_image> limg = load_image_at((const void *)img.load_address());
				void *clone = 0;

				// ACT
				limg->enumerate_functions(bind(&copy_specific, _1, "bubble_sort", ref(a), ref(clone)));

				// ASSERT
				assert_not_null(clone);

				// INIT
				int arr1[] = { 1, 10, 3, 90, 5, };
				int arr2[] = { 1, 5, 4, 3, 5, -10 };
				fn_t *f = address_cast_hack<fn_t *>(clone);

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
				ememory_allocator a;
				image img(L"symbol_container_2");
				shared_ptr<binary_image> limg = load_image_at((const void *)img.load_address());
				void *clone = 0;
				fn_t *original = img.get_symbol<fn_t>("get_function_addresses_2");
				ret_fn_t *values[3];

				// ACT
				limg->enumerate_functions(bind(&copy_specific, _1, "get_function_addresses_2", ref(a), ref(clone)));
				fn_t *f = address_cast_hack<fn_t *>(clone);
				f(values[0], values[1], values[2]);

				// ASSERT
				ret_fn_t *reference[3];

				original(reference[0], reference[1], reference[2]);
				assert_equal(reference, values);
			}
		end_test_suite
	}
}
