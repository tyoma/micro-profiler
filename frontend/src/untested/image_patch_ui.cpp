#include <frontend/image_patch_ui.h>

#include <frontend/headers_model.h>
#include <frontend/image_patch_model.h>
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

		const headers_model::column c_columns_symbols[] = {
			{	"Rva", "RVA" + secondary, 28, headers_model::dir_none, agge::align_far	},
			{	"Function", "Function\n" + secondary + "qualified name", 384, headers_model::dir_ascending, agge::align_near	},
			{	"Size", "Size" + secondary, 64, headers_model::dir_ascending, agge::align_far	},
			{	"ModuleId", "Module\n" + secondary + "persistent id", 64, headers_model::dir_descending, agge::align_far	},
		};
	}

	image_patch_ui::image_patch_ui(const factory &factory_, shared_ptr<image_patch_model> model)
		: stack(false, factory_.context.cursor_manager_)
	{
		shared_ptr<button> btn;
		shared_ptr<stack> toolbar;
		shared_ptr<label> lbl;
		shared_ptr<editbox> eb;
		shared_ptr<listview> lvsymbols;
		auto header_model = make_shared<headers_model>(c_columns_symbols, headers_model::npos(), false);

		set_spacing(5);

		add(toolbar = factory_.create_control<stack>("hstack"), pixels(24), false);
			toolbar->set_spacing(5);
			toolbar->add(lbl = factory_.create_control<label>("label"), pixels(0));
				lbl->set_text(agge::style_modifier::empty + "Filter:");

			toolbar->add(eb = factory_.create_control<editbox>("editbox"), percents(100));
				_connections.push_back(eb->changed += [this, model, eb] {
					string filter;

					eb->get_value(filter);
					model->set_filter(filter);
				});

		add(lvsymbols = factory_.create_control<listview>("listview"), percents(100), false, 1);
			lvsymbols->set_model(model);
			lvsymbols->set_columns_model(header_model);
			_connections.push_back(lvsymbols->selection_changed += [this, model] (size_t index, bool selected) {
				if (selected)
				{
					_selection.push_back(model->track(index));
				}
				else
				{
					for (auto i = _selection.begin(); i != _selection.end(); ++i)
					{
						if ((*i)->index() == index)
						{
							_selection.erase(i);
							break;
						}
					}
				}
			});
			_connections.push_back(header_model->sort_order_changed += [model] (size_t column, bool ascending) {
				model->sort_by(column, ascending);
			});

		add(toolbar = factory_.create_control<stack>("hstack"), pixels(24), false);
			toolbar->set_spacing(5);
			toolbar->add(make_shared<overlay>(), percents(100));
			toolbar->add(btn = factory_.create_control<button>("button"), pixels(120), false, 100);
				btn->set_text(agge::style_modifier::empty + "Patch Selected");
				_connections.push_back(btn->clicked += [this, model, lvsymbols] {
					vector<image_patch_model::index_type> selected;

					for (auto i = _selection.begin(); i != _selection.end(); ++i)
						selected.push_back((*i)->index());
					model->patch(selected, true);
					lvsymbols->select(table_model_base::npos(), true);
					_selection.clear();
				});

			toolbar->add(btn = factory_.create_control<button>("button"), pixels(120), false, 101);
				btn->set_text(agge::style_modifier::empty + "Revert Selected");
				_connections.push_back(btn->clicked += [this, model, lvsymbols] {
					vector<image_patch_model::index_type> selected;

					for (auto i = _selection.begin(); i != _selection.end(); ++i)
						selected.push_back((*i)->index());
					model->patch(selected, false);
					lvsymbols->select(table_model_base::npos(), true);
					_selection.clear();
				});

			toolbar->add(btn = factory_.create_control<button>("button"), pixels(80), false, 102);
				btn->set_text(agge::style_modifier::empty + "Close");
				_connections.push_back(btn->clicked += [this, model] {
				});
	}
}
