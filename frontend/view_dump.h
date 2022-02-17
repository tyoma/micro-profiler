//	Copyright (c) 2011-2022 by Artem A. Gevorkyan (gevorkyan.org)
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

#include "table_model_impl.h"

#include <agge.text/richtext.h>
#include <common/string.h>

namespace micro_profiler
{
	struct dump
	{
		template <typename BaseT, typename U, typename CtxT>
		static void as_tab_separated(std::string &content, const table_model_impl<BaseT, U, CtxT> &view)
		{
			locale_lock ll("");
			const std::string lf = "\n";
			const std::string crlf = "\r\n";
			agge::richtext_t rtext((agge::font_style_annotation()));
			std::string text;
			const auto &o = view.ordered();
			const auto b = view.columns().begin();
			const auto e = view.columns().end();
			auto context = view.context();

			context.canonical = true;
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
			for (auto row = 0u; row != o.size(); ++row)
			{
				const auto &record = o[row];

				for (auto j = b; j != e; ++j)
				{
					rtext.clear();
					j->get_text(rtext, context, row, record);
					if (j != b)
						content += '\t';
					content += rtext.underlying();
				}
				content += crlf;
			}
		}
	};
}
