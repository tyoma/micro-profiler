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
			template <typename PairT>
			struct key_first
			{
				typedef typename PairT::first_type key_type;

				key_type operator ()(const PairT &item) const
				{	return item.first;	}

				template <typename IndexT>
				void operator ()(IndexT &, PairT &item, const key_type &key) const
				{	item.first = key;	}
			};

			template <typename PairT>
			struct key_second
			{
				typedef typename PairT::second_type key_type;

				key_type operator ()(const PairT &item) const
				{	return item.second;	}

				template <typename IndexT>
				void operator ()(IndexT &, PairT &item, const key_type &key) const
				{	item.second = key;	}
			};

			template <typename KeyT, int n>
			struct key_n
			{
				typedef KeyT key_type;

				template <typename T>
				key_type operator ()(const T &item) const
				{	return std::get<n>(item);	}
			};

			template <typename KeyT, int n>
			struct key_n_gen
			{
				typedef KeyT key_type;

				template <typename T>
				key_type operator ()(const T &item) const
				{	return std::get<n>(item);	}

				template <typename IndexT, typename T>
				void operator ()(IndexT &, T &item, const key_type &key) const
				{	std::get<n>(item) = key;	}
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
