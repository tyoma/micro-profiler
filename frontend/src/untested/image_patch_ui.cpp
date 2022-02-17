#include <frontend/image_patch_ui.h>

#include <frontend/headers_model.h>
#include <frontend/image_patch_model.h>
#include <frontend/selection_model.h>
#include <wpl/controls.h>
#include <wpl/factory.h>

using namespace agge;
using namespace std;
using namespace wpl;

namespace micro_profiler
{
	namespace
	{
		const auto secondary = style::height_scale(0.85);
		const auto dummy_get = [] (agge::richtext_t &, const statistics_model_context &, size_t, const call_statistics &) {};
		const auto dummy_compare = [] (const statistics_model_context &, const call_statistics &, const call_statistics &) {	return false;	};

		const headers_model::column c_columns_symbols[] = {
			{	"Rva", "RVA" + secondary, 28, agge::align_far, dummy_get, dummy_compare, true,	},
			{	"Function", "Function\n" + secondary + "qualified name", 384, agge::align_near, dummy_get, dummy_compare, true,	},
			{	"Status", "Profiling\n" + secondary + "status", 64, agge::align_near, dummy_get, dummy_compare, false,	},
			{	"Size", "Size\n" + secondary + "bytes", 64, agge::align_far, dummy_get, dummy_compare, false,	},
			{	"ModuleName", "Module\n" + secondary + "name", 120, agge::align_near, dummy_get, dummy_compare, true,	},
			{	"ModulePath", "Module\n" + secondary + "path", 150, agge::align_near, dummy_get, dummy_compare, true,	},
		};

		struct nocase_equal
		{
			static char toupper(char c)
			{	return ((97 <= c) & (c <= 122)) ? c - 32 : c;	}

			bool operator ()(char lhs, char rhs) const
			{	return toupper(lhs) == toupper(rhs);	}
		};

	}

	image_patch_ui::image_patch_ui(const factory &factory_, shared_ptr<image_patch_model> model,
			shared_ptr<const tables::patches> patches)
		: stack(false, factory_.context.cursor_manager_)
	{
		shared_ptr<button> btn;
		shared_ptr<stack> toolbar;
		shared_ptr<label> lbl;
		shared_ptr<editbox> eb;
		shared_ptr<listview> lvsymbols;
		auto header_model = make_shared<headers_model>(c_columns_symbols, headers_model::npos(), false);
		auto selection_ = model->create_selection();

		set_spacing(5);

		add(toolbar = factory_.create_control<stack>("hstack"), pixels(23), false);
			toolbar->set_spacing(5);
			toolbar->add(lbl = factory_.create_control<label>("label"), pixels(0));
				lbl->set_text(agge::style_modifier::empty + "Filter:");

			toolbar->add(eb = factory_.create_control<editbox>("editbox"), percents(100));
				_connections.push_back(eb->changed += [model, eb] {
					string filter;

					eb->get_value(filter);
					model->set_filter([filter] (const image_patch_model::record_type &r) -> bool {
						const auto b = r.symbol->name.begin();
						const auto e = r.symbol->name.end();

						return e != search(b, e, filter.begin(), filter.end(), nocase_equal());
					});
				});

		add(lvsymbols = factory_.create_control<listview>("listview"), percents(100), false, 1);
			lvsymbols->set_model(model);
			lvsymbols->set_columns_model(header_model);
			lvsymbols->set_selection_model(selection_);

		add(toolbar = factory_.create_control<stack>("hstack"), pixels(24), false);
			toolbar->set_spacing(5);
			toolbar->add(make_shared<overlay>(), percents(100));
			toolbar->add(btn = factory_.create_control<button>("button"), pixels(120), false, 100);
				btn->set_text(agge::style_modifier::empty + "Patch Selected");
				_connections.push_back(btn->clicked += [model, patches, selection_] {
					unordered_map< unsigned int, vector<unsigned int> > s;

					for (auto i = selection_->begin(); i != selection_->end(); ++i)
						s[i->persistent_id].push_back(i->rva);
					for (auto i = s.begin(); i != s.end(); ++i)
						patches->apply(i->first, range<unsigned int, size_t>(i->second.data(), i->second.size()));
					selection_->clear();
				});

			toolbar->add(btn = factory_.create_control<button>("button"), pixels(120), false, 101);
				btn->set_text(agge::style_modifier::empty + "Revert Selected");
					_connections.push_back(btn->clicked += [model, patches, selection_] {
						unordered_map< unsigned int, vector<unsigned int> > s;

						for (auto i = selection_->begin(); i != selection_->end(); ++i)
							s[i->persistent_id].push_back(i->rva);
						for (auto i = s.begin(); i != s.end(); ++i)
							patches->revert(i->first, range<unsigned int, size_t>(i->second.data(), i->second.size()));
						selection_->clear();
					});

			toolbar->add(btn = factory_.create_control<button>("button"), pixels(80), false, 102);
				btn->set_text(agge::style_modifier::empty + "Close");
				_connections.push_back(btn->clicked += [model] {
				});

		_connections.push_back(header_model->sort_order_changed += [model] (size_t column, bool ascending) {
			model->set_order(column, ascending);
		});
	}
}
