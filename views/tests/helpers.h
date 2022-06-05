#pragma once

#include <tuple>
#include <utility>
#include <vector>

namespace micro_profiler
{
	namespace views
	{
		namespace tests
		{
			struct key_first
			{
				template <typename P>
				typename P::first_type operator ()(const P &record) const
				{	return record.first;	}

				template <typename IndexT, typename P>
				void operator ()(IndexT &, P &record, const typename P::first_type &key) const
				{	record.first = key;	}
			};

			struct key_second
			{
				template <typename P>
				typename P::second_type operator ()(const P &record) const
				{	return record.second;	}

				template <typename IndexT, typename P>
				void operator ()(IndexT &, P &record, const typename P::second_type &key) const
				{	record.second = key;	}
			};

			template <int n>
			struct key_n
			{
				template <typename T>
				typename std::tuple_element<n, T>::type operator ()(const T &record) const
				{	return std::get<n>(record);	}
			};

			template <int n>
			struct key_n_gen
			{
				template <typename T>
				typename std::tuple_element<n, T>::type operator ()(const T &record) const
				{	return std::get<n>(record);	}

				template <typename IndexT, typename T>
				void operator ()(IndexT &, T &record, const typename std::tuple_element<n, T>::type &key) const
				{	std::get<n>(record) = key;	}
			};



			template <typename TableT, typename T>
			inline typename TableT::const_iterator add_record(TableT &table_, const T &object)
			{
				auto r = table_.create();

				*r = object;
				r.commit();
				return r;
			}

			template <typename TableT, typename ContainerT>
			inline std::vector<typename TableT::const_iterator> add_records(TableT &table_, const ContainerT &objects)
			{
				std::vector<typename TableT::const_iterator> iterators;

				for (auto i = std::begin(objects); i != std::end(objects); ++i)
					iterators.push_back(add_record(table_, *i));
				return iterators;
			}
		}
	}
}
