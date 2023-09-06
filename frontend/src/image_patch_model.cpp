//	Copyright (c) 2011-2023 by Artem A. Gevorkyan (gevorkyan.org)
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

#include <frontend/image_patch_model.h>

#include <common/formatting.h>
#include <common/path.h>
#include <frontend/columns_layout.h>
#include <frontend/database_views.h>
#include <frontend/keyer.h>
#include <frontend/selection_model.h>
#include <frontend/trackables_provider.h>

using namespace std;

namespace micro_profiler
{
	image_patch_model::image_patch_model(shared_ptr<filter_view_t> underlying, shared_ptr<const tables::modules> modules)
		: table_model_impl<wpl::richtext_table_model, views::filter<tables::patched_symbols>, image_patch_model_context>(
			underlying, initialize<image_patch_model_context>()), _underlying(underlying), _modules(modules)
	{	add_columns(c_patched_symbols_columns);	}

	shared_ptr<image_patch_model> image_patch_model::create(shared_ptr<const tables::patches> patches,
		shared_ptr<const tables::modules> modules, shared_ptr<const tables::module_mappings> mappings,
		shared_ptr<const tables::symbols> symbols, shared_ptr<const tables::source_files> source_files)
	{
		const auto ps = patched_symbols(symbols, modules, source_files, mappings, patches);
		const auto m = shared_ptr<image_patch_model>(new image_patch_model(make_shared<filter_view_t>(*ps), modules));
		const auto result = make_shared_aspect(make_shared_copy(make_tuple(ps, m, ps->invalidate += [m] {	m->fetch();	},
			mappings->invalidate += [m, mappings] {	m->request_missing(*mappings);	})), m.get());

		m->request_missing(*mappings);
		return result;
	}

	wpl::slot_connection image_patch_model::maintain_legacy_symbols(tables::modules &modules,
		shared_ptr<tables::symbols> symbols, shared_ptr<tables::source_files> source_files)
	{
		return modules.created += [symbols, source_files] (tables::modules::const_iterator m) {
			for (auto i = begin(m->symbols); i != end(m->symbols); ++i)
			{
				auto r = symbols->create();

				(*r).module_id = m->id;
				static_cast<symbol_info &>(*r) = *i;
				r.commit();
			}
			for (auto i = begin(m->source_files); i != end(m->source_files); ++i)
			{
				auto r = source_files->create();

				(*r).id = i->first;
				(*r).module_id = m->id;
				(*r).path = i->second;
				r.commit();
			}
			symbols->invalidate();
			source_files->invalidate();
		};
	}

	void image_patch_model::request_missing(const tables::module_mappings &mappings)
	{
		for (auto i = mappings.begin(); i != mappings.end(); ++i)
		{
			auto req = _requests.insert(make_pair(i->module_id, shared_ptr<void>()));

			if (!req.second)
				continue;
			_modules->request_presence(req.first->second, i->module_id, [this] (const module_info_metadata &/*metadata*/) {
				fetch();
			});
		}
	}
}
