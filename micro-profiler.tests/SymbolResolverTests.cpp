#include <frontend/symbol_resolver.h>

#include "Helpers.h"

#include <ut/assert.h>
#include <ut/test.h>

using namespace std;

namespace micro_profiler
{
	namespace tests
	{
		typedef void (*get_function_addresses_1_t)(const void *&f1, const void *&f2);
		typedef void (*get_function_addresses_2_t)(const void *&f1, const void *&f2, const void *&f3);
		typedef void (*get_function_addresses_3_t)(const void *&f);

		begin_test_suite( SymbolResolverTests )
			test( ResolverCreationReturnsNonNullObject )
			{
				// INIT / ACT / ASSERT
				assert_not_null(symbol_resolver::create());
			}


			test( LoadImageFailsWhenInvalidModuleSpecified )
			{
				// INIT
				shared_ptr<symbol_resolver> r(symbol_resolver::create());

				// ACT / ASSERT
				assert_throws(r->add_image(L"", reinterpret_cast<const void *>(0x12345)), invalid_argument);
				assert_throws(r->add_image(L"missingABCDEFG.dll", reinterpret_cast<const void *>(0x23451)), invalid_argument);
			}


			test( CreateResolverForValidImage )
			{
				// INIT
				image img1(_T("micro-profiler.tests.dll"));
				image img2(_T("symbol_container_1.dll"));
				shared_ptr<symbol_resolver> r(symbol_resolver::create());

				// ACT / ASSERT (must not throw)
				r->add_image(img1.absolute_path(), img1.load_address());
				r->add_image(img1.absolute_path(),
					reinterpret_cast<const void *>(reinterpret_cast<uintptr_t>(img1.load_address()) + 0x100000));
				r->add_image(img2.absolute_path(), img2.load_address());
			}


			test( ReturnNamesOfLocalFunctions1 )
			{
				// INIT
				image img(_T("symbol_container_1.dll"));
				shared_ptr<symbol_resolver> r(symbol_resolver::create());
				get_function_addresses_1_t getter_1 =
					reinterpret_cast<get_function_addresses_1_t>(img.get_symbol_address("get_function_addresses_1"));
				const void *f1 = 0, *f2 = 0;

				r->add_image(img.absolute_path(), img.load_address());
				getter_1(f1, f2);

				// ACT
				wstring name1 = r->symbol_name_by_va(f1);
				wstring name2 = r->symbol_name_by_va(f2);

				// ASSERT
				assert_equal(name1, L"very_simple_global_function");
				assert_equal(name2, L"a_tiny_namespace::function_that_hides_under_a_namespace");
			}


			test( ReturnNamesOfLocalFunctions2 )
			{
				// INIT
				image img(_T("symbol_container_2.dll"));
				shared_ptr<symbol_resolver> r(symbol_resolver::create());
				get_function_addresses_2_t getter_2 =
					reinterpret_cast<get_function_addresses_2_t>(img.get_symbol_address("get_function_addresses_2"));
				const void *f1 = 0, *f2 = 0, *f3 = 0;

				r->add_image(img.absolute_path(), img.load_address());
				getter_2(f1, f2, f3);

				// ACT
				wstring name1 = r->symbol_name_by_va(f1);
				wstring name2 = r->symbol_name_by_va(f2);
				wstring name3 = r->symbol_name_by_va(f3);

				// ASSERT
				assert_equal(name1, L"vale_of_mean_creatures::this_one_for_the_birds");
				assert_equal(name2, L"vale_of_mean_creatures::this_one_for_the_whales");
				assert_equal(name3, L"vale_of_mean_creatures::the_abyss::bubble_sort");
			}


			test( RespectLoadAddress )
			{
				// INIT
				image img(_T("symbol_container_2.dll"));
				shared_ptr<symbol_resolver> r(symbol_resolver::create());
				get_function_addresses_2_t getter_2 =
					reinterpret_cast<get_function_addresses_2_t>(img.get_symbol_address("get_function_addresses_2"));
				const void *f1 = 0, *f2 = 0, *f3 = 0;

				r->add_image(img.absolute_path(),
					reinterpret_cast<const void *>(reinterpret_cast<uintptr_t>(img.load_address()) + 113));
				getter_2(f1, f2, f3);

				*reinterpret_cast<const char **>(&f1) += 113;
				*reinterpret_cast<const char **>(&f2) += 113;
				*reinterpret_cast<const char **>(&f3) += 113;

				// ACT
				wstring name1 = r->symbol_name_by_va(f1);
				wstring name2 = r->symbol_name_by_va(f2);
				wstring name3 = r->symbol_name_by_va(f3);

				// ASSERT
				assert_equal(name1, L"vale_of_mean_creatures::this_one_for_the_birds");
				assert_equal(name2, L"vale_of_mean_creatures::this_one_for_the_whales");
				assert_equal(name3, L"vale_of_mean_creatures::the_abyss::bubble_sort");
			}


			test( LoadModuleWithNoSymbols )
			{
				// INIT
				image img(_T("symbol_container_3_nosymbols.dll"));
				shared_ptr<symbol_resolver> r(symbol_resolver::create());
				get_function_addresses_3_t getter_3 =
					reinterpret_cast<get_function_addresses_3_t>(img.get_symbol_address("get_function_addresses_3"));
				const void *f = 0;

				r->add_image(img.absolute_path(), img.load_address());
				getter_3(f);

				// ACT
				wstring name1 = r->symbol_name_by_va(getter_3);
				wstring name2 = r->symbol_name_by_va(f);

				// ASSERT
				assert_equal(name1, L"get_function_addresses_3"); // exported function will have a name
				assert_equal(name2, L"");
			}


			test( ConstantReferenceFromResolverIsTheSame )
			{
				// INIT
				image img(_T("symbol_container_2.dll"));
				shared_ptr<symbol_resolver> r(symbol_resolver::create());
				get_function_addresses_2_t getter_2 =
					reinterpret_cast<get_function_addresses_2_t>(img.get_symbol_address("get_function_addresses_2"));
				const void *f1 = 0, *f2 = 0, *f3 = 0;

				r->add_image(img.absolute_path(), img.load_address());
				getter_2(f1, f2, f3);

				// ACT
				const wstring *name1_1 = &r->symbol_name_by_va(f1);
				const wstring *name2_1 = &r->symbol_name_by_va(f2);
				const wstring *name3_1 = &r->symbol_name_by_va(f3);

				const wstring *name2_2 = &r->symbol_name_by_va(f2);
				const wstring *name1_2 = &r->symbol_name_by_va(f1);
				const wstring *name3_2 = &r->symbol_name_by_va(f3);

				// ASSERT
				assert_equal(name1_1, name1_2);
				assert_equal(name2_1, name2_2);
				assert_equal(name3_1, name3_2);
			}


			test( LoadSymbolsForSecondModule )
			{
				// INIT
				image img1(_T("symbol_container_1.dll")), img2(_T("symbol_container_2.dll"));
				shared_ptr<symbol_resolver> r(symbol_resolver::create());
				get_function_addresses_1_t getter_1 =
					reinterpret_cast<get_function_addresses_1_t>(img1.get_symbol_address("get_function_addresses_1"));
				get_function_addresses_2_t getter_2 =
					reinterpret_cast<get_function_addresses_2_t>(img2.get_symbol_address("get_function_addresses_2"));
				const void  *f1_1 = 0, *f2_1 = 0, *f1_2 = 0, *f2_2 = 0, *f3_2 = 0;

				getter_1(f1_1, f2_1);
				getter_2(f1_2, f2_2, f3_2);

				// ACT
				r->add_image(img1.absolute_path(), img1.load_address());
				r->add_image(img2.absolute_path(), img2.load_address());

				// ACT / ASSERT
				assert_equal(r->symbol_name_by_va(f1_2), L"vale_of_mean_creatures::this_one_for_the_birds");
				assert_equal(r->symbol_name_by_va(f2_2), L"vale_of_mean_creatures::this_one_for_the_whales");
				assert_equal(r->symbol_name_by_va(f3_2), L"vale_of_mean_creatures::the_abyss::bubble_sort");
				assert_equal(r->symbol_name_by_va(f1_1), L"very_simple_global_function");
				assert_equal(r->symbol_name_by_va(f2_1), L"a_tiny_namespace::function_that_hides_under_a_namespace");
			}
		end_test_suite
	}
}
