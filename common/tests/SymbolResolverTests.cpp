#include <common/symbol_resolver.h>

#include <common/module.h>
#include <common/path.h>
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
			int g_dummy;

			typedef void (*void_f_t)();
			typedef void (get_function_addresses_1_t)(void_f_t &f1, void_f_t &f2);
			typedef void (get_function_addresses_2_t)(void_f_t &f1, void_f_t &f2, void_f_t &f3);
			typedef void (get_function_addresses_3_t)(void_f_t &f);
		}

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
				assert_throws(r->add_image("", 0x12345), invalid_argument);
				assert_throws(r->add_image("missingABCDEFG", 0x23451), invalid_argument);
			}


			test( CreateResolverForValidImage )
			{
				// INIT
				image img1(get_module_info(&g_dummy).path.c_str());
				image img2("symbol_container_1");
				shared_ptr<symbol_resolver> r(symbol_resolver::create());

				// ACT / ASSERT (must not throw)
				r->add_image(img1.absolute_path(), img1.load_address());
				r->add_image(img1.absolute_path(), img1.load_address() + 0x100000);
				r->add_image(img2.absolute_path(), img2.load_address());
			}


			test( ReturnNamesOfLocalFunctions1 )
			{
				// INIT
				image img("symbol_container_1");
				shared_ptr<symbol_resolver> r(symbol_resolver::create());
				get_function_addresses_1_t *getter_1 = img.get_symbol<get_function_addresses_1_t>("get_function_addresses_1");
				void_f_t f1 = 0, f2 = 0;

				r->add_image(img.absolute_path(), img.load_address());
				getter_1(f1, f2);

				// ACT
				string name1 = r->symbol_name_by_va((long_address_t)f1);
				string name2 = r->symbol_name_by_va((long_address_t)f2);

				// ASSERT
				assert_equal(name1, "very_simple_global_function");
				assert_equal(name2, "a_tiny_namespace::function_that_hides_under_a_namespace");
			}


			test( ReturnNamesOfLocalFunctions2 )
			{
				// INIT
				image img("symbol_container_2");
				shared_ptr<symbol_resolver> r(symbol_resolver::create());
				get_function_addresses_2_t *getter_2 =
					img.get_symbol<get_function_addresses_2_t>("get_function_addresses_2");
				void_f_t f1 = 0, f2 = 0, f3 = 0;

				r->add_image(img.absolute_path(), img.load_address());
				getter_2(f1, f2, f3);

				// ACT
				string name1 = r->symbol_name_by_va((long_address_t)f1);
				string name2 = r->symbol_name_by_va((long_address_t)f2);
				string name3 = r->symbol_name_by_va((long_address_t)f3);

				// ASSERT
				assert_equal(name1, "vale_of_mean_creatures::this_one_for_the_birds");
				assert_equal(name2, "vale_of_mean_creatures::this_one_for_the_whales");
				assert_equal(name3, "vale_of_mean_creatures::the_abyss::bubble_sort");
			}


			test( RespectLoadAddress )
			{
				// INIT
				image img("symbol_container_2");
				shared_ptr<symbol_resolver> r(symbol_resolver::create());
				get_function_addresses_2_t *getter_2 =
					img.get_symbol<get_function_addresses_2_t>("get_function_addresses_2");
				void_f_t f1 = 0, f2 = 0, f3 = 0;

				r->add_image(img.absolute_path(), img.load_address() + 113);
				getter_2(f1, f2, f3);

				*reinterpret_cast<const char **>(&f1) += 113;
				*reinterpret_cast<const char **>(&f2) += 113;
				*reinterpret_cast<const char **>(&f3) += 113;

				// ACT
				string name1 = r->symbol_name_by_va((long_address_t)f1);
				string name2 = r->symbol_name_by_va((long_address_t)f2);
				string name3 = r->symbol_name_by_va((long_address_t)f3);

				// ASSERT
				assert_equal(name1, "vale_of_mean_creatures::this_one_for_the_birds");
				assert_equal(name2, "vale_of_mean_creatures::this_one_for_the_whales");
				assert_equal(name3, "vale_of_mean_creatures::the_abyss::bubble_sort");
			}


			test( LoadModuleWithNoSymbols )
			{
				// INIT
				image img("symbol_container_3_nosymbols");
				shared_ptr<symbol_resolver> r(symbol_resolver::create());
				get_function_addresses_3_t *getter_3 = img.get_symbol<get_function_addresses_3_t>("get_function_addresses_3");
				void_f_t f = 0;

				r->add_image(img.absolute_path(), img.load_address());
				getter_3(f);

				// ACT
				string name1 = r->symbol_name_by_va((long_address_t)getter_3);
				string name2 = r->symbol_name_by_va((long_address_t)f);

				// ASSERT
				assert_equal(name1, "get_function_addresses_3"); // exported function will have a name
				assert_equal(name2, "");
			}


			test( ConstantReferenceFromResolverIsTheSame )
			{
				// INIT
				image img("symbol_container_2");
				shared_ptr<symbol_resolver> r(symbol_resolver::create());
				get_function_addresses_2_t *getter_2 = img.get_symbol<get_function_addresses_2_t>("get_function_addresses_2");
				void_f_t f1 = 0, f2 = 0, f3 = 0;

				r->add_image(img.absolute_path(), img.load_address());
				getter_2(f1, f2, f3);

				// ACT
				const string *name1_1 = &r->symbol_name_by_va((long_address_t)f1);
				const string *name2_1 = &r->symbol_name_by_va((long_address_t)f2);
				const string *name3_1 = &r->symbol_name_by_va((long_address_t)f3);

				const string *name2_2 = &r->symbol_name_by_va((long_address_t)f2);
				const string *name1_2 = &r->symbol_name_by_va((long_address_t)f1);
				const string *name3_2 = &r->symbol_name_by_va((long_address_t)f3);

				// ASSERT
				assert_equal(name1_1, name1_2);
				assert_equal(name2_1, name2_2);
				assert_equal(name3_1, name3_2);
			}


			test( LoadSymbolsForSecondModule )
			{
				// INIT
				image img1("symbol_container_1"), img2("symbol_container_2");
				shared_ptr<symbol_resolver> r(symbol_resolver::create());
				get_function_addresses_1_t *getter_1 = img1.get_symbol<get_function_addresses_1_t>("get_function_addresses_1");
				get_function_addresses_2_t *getter_2 = img2.get_symbol<get_function_addresses_2_t>("get_function_addresses_2");
				void_f_t f1_1 = 0, f2_1 = 0, f1_2 = 0, f2_2 = 0, f3_2 = 0;

				getter_1(f1_1, f2_1);
				getter_2(f1_2, f2_2, f3_2);

				// ACT
				r->add_image(img1.absolute_path(), img1.load_address());
				r->add_image(img2.absolute_path(), img2.load_address());

				// ACT / ASSERT
				assert_equal(r->symbol_name_by_va((long_address_t)f1_2), "vale_of_mean_creatures::this_one_for_the_birds");
				assert_equal(r->symbol_name_by_va((long_address_t)f2_2), "vale_of_mean_creatures::this_one_for_the_whales");
				assert_equal(r->symbol_name_by_va((long_address_t)f3_2), "vale_of_mean_creatures::the_abyss::bubble_sort");
				assert_equal(r->symbol_name_by_va((long_address_t)f1_1), "very_simple_global_function");
				assert_equal(r->symbol_name_by_va((long_address_t)f2_1), "a_tiny_namespace::function_that_hides_under_a_namespace");
			}


			test( FileLineInformationIsReturnedBySymbolProvider )
			{
				// INIT
				image img1("symbol_container_1"), img2("symbol_container_2");
				shared_ptr<symbol_resolver> r(symbol_resolver::create());
				get_function_addresses_1_t *getter_1 = img1.get_symbol<get_function_addresses_1_t>("get_function_addresses_1");
				get_function_addresses_2_t *getter_2 = img2.get_symbol<get_function_addresses_2_t>("get_function_addresses_2");
				void_f_t f1_1 = 0, f2_1 = 0, f1_2 = 0, f2_2 = 0, f3_2 = 0;

				getter_1(f1_1, f2_1);
				getter_2(f1_2, f2_2, f3_2);
				r->add_image(img1.absolute_path(), img1.load_address());
				r->add_image(img2.absolute_path(), img2.load_address());

				// ACT
				pair<string, unsigned> filelines[] = {
					r->symbol_fileline_by_va((long_address_t)f1_1),
					r->symbol_fileline_by_va((long_address_t)f2_1),
					r->symbol_fileline_by_va((long_address_t)f1_2),
					r->symbol_fileline_by_va((long_address_t)f2_2),
					r->symbol_fileline_by_va((long_address_t)f3_2),
				};

				// ASSERT
				for (pair<string, unsigned> *i = begin(filelines); i != end(filelines); ++i)
					i->first = *i->first;

				pair<string, unsigned> reference[] = {
					make_pair("symbol_container_1.cpp", 1),
					make_pair("symbol_container_1.cpp", 7),
					make_pair("symbol_container_2.cpp", 3),
					make_pair("symbol_container_2.cpp", 7),
					make_pair("symbol_container_2.cpp", 13),
				};


				assert_equal(reference, filelines);
			}


			test( NoFileLineInformationIsReturnedForInvalidAddress )
			{
				// INIT
				shared_ptr<symbol_resolver> r(symbol_resolver::create());

				// ACT
				pair<string, unsigned> fileline = r->symbol_fileline_by_va(0);

				// ASSERT
				assert_equal(make_pair(string(), 0u), fileline);
			}
		end_test_suite
	}
}
