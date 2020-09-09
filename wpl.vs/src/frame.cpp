//	Copyright (c) 2011-2020 by Artem A. Gevorkyan (gevorkyan.org)
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

#include "frame.h"

#include "pane.h"

#include <logger/log.h>

#define PREAMBLE "Generic VS Frame/Form: "

using namespace std;

namespace wpl
{
	namespace vs
	{
		frame::frame(const CComPtr<IVsWindowFrame> &underlying, pane &pane_)
			: _underlying(underlying), _pane(pane_)
		{
			_closed_connection = _pane.closed += [this] { close(); };
			_underlying->SetProperty(VSFPROPID_FrameMode, CComVariant(VSFM_MdiChild));
			LOG(PREAMBLE "constructed...") % A(this);
		}

		frame::~frame()
		{
			LOG(PREAMBLE "destroying...") % A(this);
			_underlying->CloseFrame(FRAMECLOSE_NoSave);
		}

		void frame::set_view(const shared_ptr<wpl::view> &v)
		{	_pane.host->set_view(v);	}

		void frame::set_background_color(agge::color color)
		{	_pane.host->set_background_color(color);	}

		view_location frame::get_location() const
		{	throw 0;	}

		void frame::set_location(const view_location &)
		{	throw 0;	}

		void frame::set_visible(bool value)
		{	value ? _underlying->Show() : _underlying->Hide();	}

		void frame::set_caption(const wstring &caption)
		{	_underlying->SetProperty(VSFPROPID_Caption, CComVariant(caption.c_str()));	}

		void frame::set_caption_icon(const gcontext::surface_type &)
		{	throw 0;	}

		void frame::set_task_icon(const gcontext::surface_type &)
		{	throw 0;	}

		shared_ptr<form> frame::create_child()
		{	throw 0;	}

		void frame::set_style(unsigned /*styles*/)
		{	throw 0;	}

		void frame::set_font(const font &)
		{	throw 0;	}
	}
}
