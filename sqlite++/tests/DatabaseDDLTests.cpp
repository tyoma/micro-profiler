#include <sqlite++/format.h>

#include <ut/assert.h>
#include <ut/test.h>

using namespace std;

namespace micro_profiler
{
	namespace sql
	{
		namespace
		{
			struct type_a
			{
				int a;
				string b;
			};

			struct type_b
			{
				string a;
				string b;
				int64_t c;
			};

			struct type_all
			{
				int a;
				unsigned int b;
				int64_t c;
				uint64_t d;
				string e;
				double f;
			};

			struct type_ided
			{
				unsigned int b;
				int id;
			};

			struct type_inherited : type_ided
			{
				int c;
			};

			template <typename VisitorT>
			void describe(VisitorT &&visitor, type_a *)
			{
				visitor(&type_a::a, "Foo");
				visitor(&type_a::b, "Bar");
			}

			template <typename VisitorT>
			void describe(VisitorT &&visitor, type_b *)
			{
				visitor(&type_b::a, "Lorem");
				visitor(&type_b::b, "Ipsum");
				visitor(&type_b::c, "Amet");
			}

			template <typename VisitorT>
			void describe(VisitorT &&visitor, type_all *)
			{
				visitor(&type_all::a, "A");
				visitor(&type_all::b, "b");
				visitor(&type_all::c, "C");
				visitor(&type_all::d, "D");
				visitor(&type_all::e, "E");
				visitor(&type_all::f, "F");
			}

			template <typename VisitorT>
			void describe(VisitorT &&visitor, type_ided *)
			{
				visitor(&type_ided::b, "b");
				visitor(pk(&type_ided::id), "Id");
			}

			template <typename VisitorT>
			void describe(VisitorT &&visitor, type_inherited *)
			{
				visitor(&type_inherited::b, "b");
				visitor(pk(&type_inherited::id), "id");
				visitor(&type_inherited::c, "c");
			}

			template <typename T>
			string format_columns()
			{
				string result;
				column_definition_format_visitor<T> v = {	result, true	};

				describe<T>(v);
				return result;
			}

			template <typename T>
			string format_create_table(const char *name)
			{
				string result;

				sql::format_create_table<T>(result, name);
				return result;
			};
		}

		begin_test_suite( DatabaseDDLTests )
			test( FormattingColumnsDefinitionProvidesExpectedResults )
			{
				// INIT / ACT / ASSERT
				assert_equal("Foo INTEGER NOT NULL,Bar TEXT NOT NULL", format_columns<type_a>());
				assert_equal("Lorem TEXT NOT NULL,Ipsum TEXT NOT NULL,Amet INTEGER NOT NULL", format_columns<type_b>());
				assert_equal("A INTEGER NOT NULL,b INTEGER NOT NULL,C INTEGER NOT NULL,D INTEGER NOT NULL,E TEXT NOT NULL,F REAL NOT NULL", format_columns<type_all>());
			}


			test( FormattingCreateTableWithoutKeysProvidesExpectedResults )
			{
				// INIT / ACT / ASSERT
				assert_equal("CREATE TABLE Lorem (Foo INTEGER NOT NULL,Bar TEXT NOT NULL)", format_create_table<type_a>("Lorem"));
				assert_equal("CREATE TABLE Zanzibar (Lorem TEXT NOT NULL,Ipsum TEXT NOT NULL,Amet INTEGER NOT NULL)", format_create_table<type_b>("Zanzibar"));
				assert_equal("CREATE TABLE Bar (A INTEGER NOT NULL,b INTEGER NOT NULL,C INTEGER NOT NULL,D INTEGER NOT NULL,E TEXT NOT NULL,F REAL NOT NULL)", format_create_table<type_all>("Bar"));
			}


			test( FormattingCreateTableWithPrimaryKeyProvidesExpectedResult )
			{
				// INIT / ACT / ASSERT
				assert_equal("CREATE TABLE Baz (b INTEGER NOT NULL,Id INTEGER NOT NULL PRIMARY KEY ASC)", format_create_table<type_ided>("Baz"));
				assert_equal("CREATE TABLE Baz (b INTEGER NOT NULL,id INTEGER NOT NULL PRIMARY KEY ASC,c INTEGER NOT NULL)", format_create_table<type_inherited>("Baz"));
			}
			
		end_test_suite
	}
}
