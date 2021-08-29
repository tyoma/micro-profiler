//	Copyright (c) 2011-2021 by Artem A. Gevorkyan (gevorkyan.org) and Denis Burenko
//
//	Permission is hereby granted, free of charge, to any person obtaining a copy
//	of this software and associated documentation files (the "Software"), to deal
//	in the Software without restriction, including without limitation the rights
//	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//	copies of the Software, and to permit persons to whom the Software is
//	furnished to do so, subject to the following conditions:
//
//	The above copyright notice and this permission notice shall be included in
//	all copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
//	THE SOFTWARE.

#pragma once

#include "statistics_model.h"

namespace micro_profiler
{
	namespace tables
	{
		struct statistics;
	}


	struct serialization_context_file_v3;
	class threads_model;

	struct linked_statistics : wpl::richtext_table_model
	{
		virtual ~linked_statistics() {	}
		virtual void set_order(index_type column, bool ascending) = 0;
		virtual std::shared_ptr< wpl::list_model<double> > get_column_series() const = 0;
		virtual std::shared_ptr< selection<statistic_types::key> > create_selection() const = 0;
	};

	struct linked_statistics_ex : linked_statistics
	{
		virtual void on_updated() = 0;
		virtual void detach() = 0;
	};

	class functions_list : public statistics_model_impl<wpl::richtext_table_model, statistic_types::map_detailed>
	{
	public:
		typedef statistic_types::map_detailed::value_type value_type;

	public:
		functions_list(std::shared_ptr<tables::statistics> statistics, double tick_interval,
			std::shared_ptr<symbol_resolver> resolver, std::shared_ptr<threads_model> threads);
		virtual ~functions_list();

		void clear();
		void print(std::string &content) const;
		std::shared_ptr<linked_statistics> watch_children(key_type item) const;
		std::shared_ptr<linked_statistics> watch_parents(key_type item) const;

		static std::shared_ptr<functions_list> create(timestamp_t ticks_per_second,
			std::shared_ptr<symbol_resolver> resolver, std::shared_ptr<threads_model> threads);

	public:
		bool updates_enabled;

	private:
		typedef statistics_model_impl<wpl::richtext_table_model, statistic_types::map_detailed> base;
		typedef std::list<linked_statistics_ex *> linked_statistics_list_t;

	private:
		functions_list(std::shared_ptr<statistic_types::map_detailed> statistics, double tick_interval,
			std::shared_ptr<symbol_resolver> resolver, std::shared_ptr<threads_model> threads);

		void on_updated();

	private:
		using base::_tick_interval;
		std::shared_ptr<statistic_types::map_detailed> _statistics;
		std::shared_ptr<linked_statistics_list_t> _linked;
		wpl::slot_connection _connection;

	private:
		template <typename ArchiveT, typename ContextT>
		friend void serialize(ArchiveT &archive, functions_list &model, unsigned int version, ContextT &context);
	};
}
