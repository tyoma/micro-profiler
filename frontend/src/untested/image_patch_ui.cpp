#include <frontend/image_patch_ui.h>

#include <frontend/columns_layout.h>
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
		const auto header_model = make_shared<headers_model>(c_patched_symbols_columns, headers_model::npos(), false);
		const auto selection_ = make_shared< sdb::table<selected_symbol> >();

		set_spacing(5);

		add(toolbar = factory_.create_control<stack>("hstack"), pixels(23), false);
			toolbar->set_spacing(5);
			toolbar->add(lbl = factory_.create_control<label>("label"), pixels(0));
				lbl->set_text(agge::style_modifier::empty + "Filter:");

			toolbar->add(eb = factory_.create_control<editbox>("editbox"), percents(100));
				_connections.push_back(eb->changed += [model, eb] {
					string filter;

					eb->get_value(filter);
					model->set_filter([filter] (const tables::patched_symbol_adaptor &r) {
						return end(r.symbol().name) != search(begin(r.symbol().name), end(r.symbol().name),
							filter.begin(), filter.end(), nocase_equal());
					});
				});

		add(lvsymbols = factory_.create_control<listview>("listview"), percents(100), false, 1);
			lvsymbols->set_model(model);
			lvsymbols->set_columns_model(header_model);
			lvsymbols->set_selection_model(make_shared< selection<selected_symbol> >(selection_, [model] (size_t item_index) -> selected_symbol {
				const tables::patched_symbol_adaptor item(model->ordered()[item_index]);
				return make_tuple(item.symbol().module_id, item.symbol().rva, item.symbol().size);
			}));

		add(toolbar = factory_.create_control<stack>("hstack"), pixels(24), false);
			toolbar->set_spacing(5);
			toolbar->add(make_shared<overlay>(), percents(100));
			toolbar->add(btn = factory_.create_control<button>("button"), pixels(120), false, 100);
				btn->set_text(agge::style_modifier::empty + "Patch Selected");
				_connections.push_back(btn->clicked += [model, patches, selection_] {
					unordered_map< unsigned int, vector<tables::patches::patch_def> > s;

					for (auto i = selection_->begin(); i != selection_->end(); ++i)
						s[get<0>(*i)].push_back(tables::patches::patch_def(get<1>(*i), get<2>(*i)));
					for (auto i = s.begin(); i != s.end(); ++i)
						patches->apply(i->first, make_range(i->second));
					selection_->clear();
				});

			toolbar->add(btn = factory_.create_control<button>("button"), pixels(120), false, 101);
				btn->set_text(agge::style_modifier::empty + "Revert Selected");
					_connections.push_back(btn->clicked += [model, patches, selection_] {
						unordered_map< unsigned int, vector<unsigned int> > s;

						for (auto i = selection_->begin(); i != selection_->end(); ++i)
							s[get<0>(*i)].push_back(get<1>(*i));
						for (auto i = s.begin(); i != s.end(); ++i)
							patches->revert(i->first, make_range(i->second));
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
