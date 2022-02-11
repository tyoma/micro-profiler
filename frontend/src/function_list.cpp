//	Copyright (c) 2011-2022 by Artem A. Gevorkyan (gevorkyan.org) and Denis Burenko
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

#include <frontend/function_list.h>

#include <common/formatting.h>
#include <common/string.h>
#include <frontend/columns_layout.h>
#include <frontend/nested_statistics_model.h>
#include <frontend/nested_transform.h>
#include <frontend/symbol_resolver.h>

#include <cmath>
#include <clocale>
#include <stdexcept>
#include <utility>

using namespace std;
using namespace placeholders;

namespace micro_profiler
{
	namespace
	{
		static const string c_empty;

		template <typename T, typename U>
		shared_ptr<T> make_bound(shared_ptr<U> underlying)
		{
			typedef pair<shared_ptr<U>, T> pair_t;
			auto p = make_shared<pair_t>(underlying, T(*underlying));
			return shared_ptr<T>(p, &p->second);
		}

		statistics_model_context create_context(shared_ptr<const tables::statistics> underlying, double tick_interval,
			shared_ptr<symbol_resolver> resolver, shared_ptr<const tables::threads> threads, bool canonical = false)
		{
			statistics_model_context context = {
				tick_interval,
				[underlying] (id_t id) {	return underlying->by_id.find(id);	},
				threads,
				resolver,
				canonical,
			};

			return context;
		}
	}


	shared_ptr<linked_statistics> create_callees_model(shared_ptr<const tables::statistics> underlying,
		double tick_interval, shared_ptr<symbol_resolver> resolver, shared_ptr<const tables::threads> threads,
		shared_ptr< vector<id_t> > scope)
	{
		return construct_nested<callees_transform>(create_context(underlying, tick_interval, resolver, threads),
			underlying, scope, c_callee_statistics_columns);
	}

	shared_ptr<linked_statistics> create_callers_model(shared_ptr<const tables::statistics> underlying,
		double tick_interval, shared_ptr<symbol_resolver> resolver, shared_ptr<const tables::threads> threads,
		shared_ptr< vector<id_t> > scope)
	{
		return construct_nested<callers_transform>(create_context(underlying, tick_interval, resolver, threads),
			underlying, scope, c_caller_statistics_columns);
	}


	functions_list::functions_list(shared_ptr<tables::statistics> statistics, double tick_interval,
			shared_ptr<symbol_resolver> resolver, shared_ptr<const tables::threads> threads)
		: container_view_model<wpl::richtext_table_model, views::filter<tables::statistics>, statistics_model_context>(
			make_bound< views::filter<tables::statistics> >(statistics),
			create_context(statistics, tick_interval, resolver, threads)), _tick_interval(tick_interval),
			_resolver(resolver), _threads(threads)
	{
		add_columns(c_statistics_columns);

		_invalidation_connections[0] = statistics->invalidate += [this] {	fetch();	};
		_invalidation_connections[1] = resolver->invalidate += [this] {	this->invalidate(this->npos());	};
	}

	void functions_list::print(string &content) const
	{
		const string lf = "\n";
		const string crlf = "\r\n";
		const char* old_locale = ::setlocale(LC_NUMERIC, NULL);
		bool locale_ok = ::setlocale(LC_NUMERIC, "") != NULL;
		agge::richtext_t rtext((agge::font_style_annotation()));
		string text;
		const auto b = columns().begin();
		const auto e = columns().end();
		const auto context = create_context(_statistics, _tick_interval, _resolver, _threads, true);

		content.clear();
		for (auto i = b; i != e; ++i)
		{
			text = i->caption.underlying();
			replace(text, lf, crlf);
			if (i != b)
				content += '\t';
			content += '\"', content += text, content += '\"';
		}
		content += crlf;
		for (auto row = 0u; row != get_count(); ++row)
		{
			for (auto j = b; j != e; ++j)
			{
				rtext.clear();
				j->get_text(rtext, context, row, get_entry(row));
				if (j != b)
					content += '\t';
				content += rtext.underlying();
			}
			content += crlf;
		}

		if (locale_ok)
			::setlocale(LC_NUMERIC, old_locale);
	}
}
