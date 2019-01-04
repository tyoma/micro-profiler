#include <common/symbol_resolver.h>

#include <common/module.h>
#include <common/path.h>
#include <test-helpers/helpers.h>

#include <ut/assert.h>
#include <ut/test.h>

using namespace std;
using namespace std::placeholders;

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

			void add_function(map<string, symbol_info> &functions, const symbol_info &si)
			{	functions.insert(make_pair(si.name, si));	}

			void add_function_mapped(map<string, symbol_info_mapped> &functions, const symbol_info_mapped &si)
			{	functions.insert(make_pair(si.name, si));	}
		}

		begin_test_suite( ImageInfoTests )

			string image_paths[4];

			init( InitializePaths )
			{
				image_paths[0] = get_module_info(&g_dummy).path.c_str();
				image_paths[1] = image("symbol_container_1").absolute_path();
				image_paths[2] = image("symbol_container_2").absolute_path();
				image_paths[3] = image("symbol_container_3_nosymbols").absolute_path();
			}


			test( LoadImageFailsWhenInvalidModuleSpecified )
			{
				// ACT / ASSERT
				assert_throws(load_image_info(""), invalid_argument);
				assert_throws(load_image_info("missingABCDEFG"), invalid_argument);
			}


			test( CreateResolverForValidImage )
			{
				// ACT / ASSERT
				assert_not_null(load_image_info(image_paths[0].c_str()));
				assert_not_null(load_image_info(image_paths[1].c_str()));
				assert_not_null(load_image_info(image_paths[2].c_str()));
				assert_not_null(load_image_info(image_paths[3].c_str()));
			}


			test( EnumerationReturnsKnownFunctions )
			{
				// INIT
				shared_ptr< image_info<symbol_info> > ii[] = {
					load_image_info(image_paths[1].c_str()),
					load_image_info(image_paths[2].c_str()),
				};
				map<string, symbol_info> functions[2];

				// ACT
				ii[0]->enumerate_functions(bind(&add_function, ref(functions[0]), _1));
				ii[1]->enumerate_functions(bind(&add_function, ref(functions[1]), _1));

				// ASSERT
				assert_equal(1u, functions[0].count("get_function_addresses_1"));
				assert_equal(1u, functions[0].count("format_decimal"));

				assert_equal(1u, functions[1].count("get_function_addresses_2"));
				assert_equal(1u, functions[1].count("function_with_a_nested_call_2"));
				assert_equal(1u, functions[1].count("bubble_sort2"));
				assert_equal(1u, functions[1].count("bubble_sort_expose"));
				assert_equal(1u, functions[1].count("guinea_snprintf"));
			}


			test( DataSymbolsAreSkipped )
			{
				// INIT
				shared_ptr< image_info<symbol_info> > ii = load_image_info(image_paths[2].c_str());
				map<string, symbol_info> functions;

				// ACT
				ii->enumerate_functions(bind(&add_function, ref(functions), _1));

				// ASSERT
				assert_equal(0u, functions.count("datum1"));
				assert_equal(0u, functions.count("datum2"));
			}


			test( FunctionRVAsAreValid )
			{
				// INIT
				image images[] = {
					image(image_paths[1].c_str()),
					image(image_paths[2].c_str()),
				};
				unsigned reference[] = {
					images[0].get_symbol_rva("get_function_addresses_1"),
					images[0].get_symbol_rva("format_decimal"),
					images[1].get_symbol_rva("get_function_addresses_2"),
					images[1].get_symbol_rva("function_with_a_nested_call_2"),
					images[1].get_symbol_rva("bubble_sort2"),
					images[1].get_symbol_rva("bubble_sort_expose"),
					images[1].get_symbol_rva("guinea_snprintf"),
				};
				shared_ptr< image_info<symbol_info> > ii[] = {
					load_image_info(image_paths[1].c_str()),
					load_image_info(image_paths[2].c_str()),
				};
				map<string, symbol_info> functions[2];

				// ACT
				ii[0]->enumerate_functions(bind(&add_function, ref(functions[0]), _1));
				ii[1]->enumerate_functions(bind(&add_function, ref(functions[1]), _1));

				// ASSERT
				assert_equal(reference[0], functions[0].find("get_function_addresses_1")->second.rva);
				assert_equal(reference[1], functions[0].find("format_decimal")->second.rva);

				assert_equal(reference[2], functions[1].find("get_function_addresses_2")->second.rva);
				assert_equal(reference[3], functions[1].find("function_with_a_nested_call_2")->second.rva);
				assert_equal(reference[4], functions[1].find("bubble_sort2")->second.rva);
				assert_equal(reference[5], functions[1].find("bubble_sort_expose")->second.rva);
				assert_equal(reference[6], functions[1].find("guinea_snprintf")->second.rva);
			}


			test( FunctionSizesAreValid )
			{
				// This test may not pass on all platoforms/compilers. Need a better one.

				// INIT
				shared_ptr< image_info<symbol_info> > ii = load_image_info(image_paths[2].c_str());
				map<string, symbol_info> functions;

				// ACT
				ii->enumerate_functions(bind(&add_function, ref(functions), _1));

				// ASSERT
				assert_equal(1u, functions.count("bubble_sort"));

				assert_is_true(0 < functions.find("bubble_sort_expose")->second.size);
				assert_is_true(functions.find("bubble_sort_expose")->second.size
					< functions.find("guinea_snprintf")->second.size);
				assert_is_true(functions.find("guinea_snprintf")->second.size
					< functions.find("bubble_sort")->second.size);
			}
		end_test_suite


		begin_test_suite( OffsetImageInfoTests )
			test( SymbolsEnumeratedAreOffsetAccordinglyToBase )
			{
				// INIT
				image img("symbol_container_2");
				shared_ptr< image_info<symbol_info> > ii = load_image_info(img.absolute_path());
				map<string, symbol_info> functions_original;
				map<string, symbol_info_mapped> functions_offset;

				ii->enumerate_functions(bind(&add_function, ref(functions_original), _1));

				// INIT / ACT
				offset_image_info oii1(ii, 0x123);

				oii1.enumerate_functions(bind(&add_function_mapped, ref(functions_offset), _1));

				// ASSERT
				assert_equal(functions_original.find("get_function_addresses_2")->second.rva + (byte *)0x123,
					functions_offset.find("get_function_addresses_2")->second.body.begin());
				assert_equal(functions_original.find("get_function_addresses_2")->second.size,
					functions_offset.find("get_function_addresses_2")->second.body.length());
				assert_equal(functions_original.find("function_with_a_nested_call_2")->second.rva + (byte *)0x123,
					functions_offset.find("function_with_a_nested_call_2")->second.body.begin());
				assert_equal(functions_original.find("function_with_a_nested_call_2")->second.size,
					functions_offset.find("function_with_a_nested_call_2")->second.body.length());
				assert_equal(functions_original.find("bubble_sort_expose")->second.rva + (byte *)0x123,
					functions_offset.find("bubble_sort_expose")->second.body.begin());
				assert_equal(functions_original.find("bubble_sort_expose")->second.size,
					functions_offset.find("bubble_sort_expose")->second.body.length());

				// INIT
				functions_offset.clear();

				// INIT / ACT
				offset_image_info oii2(ii, 0x12345678);

				oii2.enumerate_functions(bind(&add_function_mapped, ref(functions_offset), _1));

				// ASSERT
				assert_equal(functions_original.find("get_function_addresses_2")->second.rva + (byte *)0x12345678,
					functions_offset.find("get_function_addresses_2")->second.body.begin());
				assert_equal(functions_original.find("get_function_addresses_2")->second.size,
					functions_offset.find("get_function_addresses_2")->second.body.length());
				assert_equal(functions_original.find("function_with_a_nested_call_2")->second.rva + (byte *)0x12345678,
					functions_offset.find("function_with_a_nested_call_2")->second.body.begin());
				assert_equal(functions_original.find("function_with_a_nested_call_2")->second.size,
					functions_offset.find("function_with_a_nested_call_2")->second.body.length());
			}
		end_test_suite
	}
}
