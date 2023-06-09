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

#pragma once

#include <frontend/constructors.h>

#include <list>
#include <memory>
#include <vector>
#include <wpl/form.h>
#include <wpl/layout.h>

namespace micro_profiler
{
	struct ui_helpers
	{
		typedef std::list< std::shared_ptr<void> > running_objects_t;

		template <typename FactoryT, typename AttachSignalsCB>
		static void show_dialog(running_objects_t &running_objects, const FactoryT &factory,
			std::shared_ptr<wpl::control> root, unsigned int width, unsigned int height, const std::string &caption,
			const AttachSignalsCB &attach_signals);
	};



	template <typename FactoryT, typename AttachSignalsCB>
	inline void ui_helpers::show_dialog(running_objects_t &running_objects, const FactoryT &factory,
		std::shared_ptr<wpl::control> root_control, unsigned int width, unsigned int height, const std::string &caption,
		const AttachSignalsCB &attach_signals)
	{
		using namespace std;
		using namespace wpl;

		const auto modal = factory.create_modal();
		const auto root = make_shared<overlay>();
			root->add(factory.create_control<control>("background"));
			root->add(pad_control(root_control, 5, 5));

		modal->set_root(root);
		modal->set_location(initialize<rect_i>(0, 0, static_cast<int>(width), static_cast<int>(height)));
		modal->set_caption(caption);
		modal->center_parent();
		modal->set_visible(true);

		vector<slot_connection> aux_connections;
		const auto i = running_objects.insert(running_objects.end(), shared_ptr<void>());
		const auto onclose = [&running_objects, i] {	running_objects.erase(i);	};

		attach_signals(aux_connections, onclose);
		*i = make_shared_copy(make_tuple(modal, modal->close += onclose, aux_connections));
	}
}
