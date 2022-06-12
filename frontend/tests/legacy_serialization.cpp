#include "legacy_serialization.h"

#include <cstdint>
#include <collector/primitives.h>
#include <frontend/serialization.h>
#include <strmd/serializer.h>
#include <test-helpers/helpers.h>

using namespace std;

namespace micro_profiler
{
	namespace
	{
		template <typename T>
		struct unversion
		{
			unversion(const T &from)
				: contained(from)
			{	}

			T contained;
		};

		template <typename A, typename T>
		void serialize(A &a, unversion<T> &d)
		{	serialize(a, d.contained, 4);	}

		typedef unversion<function_statistics> fs;
		typedef unversion<mapped_module_ex> mm;
		typedef unversion<symbol_info> si;
		typedef unversion<thread_info> ti;

		struct mi
		{
			mi(const module_info_metadata &from)
				: path(from.path), symbols(from.symbols.begin(), from.symbols.end()),
					files(from.source_files.begin(), from.source_files.end())
			{	}

			string path;
			vector<si> symbols;
			vector< pair<unsigned, string> > files;
		};

		template <typename T>
		struct fsd
		{
			fsd(const call_graph_node<T> &from)
				: self(from), callees(from.callees.begin(), from.callees.end())
			{	}

			fs self;
			vector< pair<T, fs> > callees;
		};


		template <typename A>
		void serialize(A &a, mi &d)
		{
			a(d.symbols);
			a(d.files);
		}

		template <typename A, typename T>
		void serialize(A &a, fsd<T> &d)
		{
			a(d.self);
			a(d.callees);
		}
	}

	namespace tests
	{
		template <typename ArchiveT>
		void serialize(ArchiveT &a, file_v3_components &d)
		{
			a(d.ticks_per_second);
			a(vector< pair<unsigned, mm> >(d.mappings.begin(), d.mappings.end()));
			a(vector< pair<unsigned, mi> >(d.modules.begin(), d.modules.end()));
			a(vector< pair< long_address_t, fsd<long_address_t> > >(d.statistics.begin(), d.statistics.end()));
		}

		template <typename ArchiveT>
		void serialize(ArchiveT &a, file_v4_components &d)
		{
			a(d.ticks_per_second);
			a(vector< pair<unsigned, mm> >(d.mappings.begin(), d.mappings.end()));
			a(vector< pair<unsigned, mi> >(d.modules.begin(), d.modules.end()));
			a(vector< pair< legacy_function_key, fsd<legacy_function_key> > >(d.statistics.begin(), d.statistics.end()));
			a(vector< pair<unsigned, ti> >(d.threads.begin(), d.threads.end()));
		}

		void serialize_legacy(vector_adapter &buffer, const file_v3_components &components)
		{
			strmd::serializer<vector_adapter, strmd::varint> ser(buffer);

			ser(components);
		}

		void serialize_legacy(vector_adapter &buffer, const file_v4_components &components)
		{
			strmd::serializer<vector_adapter, strmd::varint> ser(buffer);

			ser(components);
		}
	}
}
