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

#include <frontend/system_stylesheet.h>

#include <agge.text/text_engine.h>
#include <stdexcept>
#include <tchar.h>
#include <windows.h>

using namespace agge;
using namespace std;

namespace micro_profiler
{
	namespace
	{
		font_weight win32_weight_to_agge(long weight)
		{
			switch (weight)
			{
			case FW_THIN: case FW_EXTRALIGHT: return extra_light;
			case FW_LIGHT: return light;
			case FW_MEDIUM: return medium;
			case FW_SEMIBOLD: return semi_bold;
			case FW_BOLD: return bold;
			case FW_EXTRABOLD: return black;
			case FW_BLACK : return extra_black;
			case FW_REGULAR: default: return regular;
			}
		}

		shared_ptr<agge::font> create(wpl::gcontext::text_engine_type &text_engine, const LOGFONTA &native_font)
		{
			return text_engine.create_font(font_descriptor::create(native_font.lfFaceName, -native_font.lfHeight,
				win32_weight_to_agge(native_font.lfWeight), !!native_font.lfItalic, hint_vertical));
		}

		shared_ptr<agge::font> get_system_font(wpl::gcontext::text_engine_type &text_engine)
		{
			NONCLIENTMETRICSA m = {};

			m.cbSize = sizeof(m);
			if (::SystemParametersInfoA(SPI_GETNONCLIENTMETRICS, 0, &m, 0))
				return create(text_engine, m.lfMenuFont);
			throw runtime_error("Cannot retrieve system font!");
		}

		color get_system_color(int color_kind)
		{
			auto c = ::GetSysColor(color_kind);
			return color::make(GetRValue(c), GetGValue(c), GetBValue(c));
		}

		color invert(color original)
		{	return color::make(~original.r, ~original.g, ~original.b, original.a);	}

		color semi(color original, double opacity)
		{	return color::make(original.r, original.g, original.b, static_cast<agge::uint8_t>(opacity * 255));	}
	}

	system_stylesheet::system_stylesheet(const shared_ptr<wpl::gcontext::text_engine_type> &text_engine)
		: _text_engine(text_engine), _notifier_handle(::CreateWindow(_T("#32770"), NULL, WS_OVERLAPPED, 0, 0, 1, 1,
			HWND_MESSAGE, NULL, NULL, NULL), &::DestroyWindow)
	{	synchronize();	}

	system_stylesheet::~system_stylesheet()
	{	}

	void system_stylesheet::synchronize()
	{
		const auto background = get_system_color(COLOR_BTNFACE);

		set_color("background", background);
		set_color("text", get_system_color(COLOR_BTNTEXT));
		set_color("text.selected", get_system_color(COLOR_HIGHLIGHTTEXT));
		set_color("border", semi(get_system_color(COLOR_3DSHADOW), 0.3));
		set_color("separator", semi(get_system_color(COLOR_3DSHADOW), 0.3));

		set_color("background.listview.even", color::make(0, 0, 0, 0));
		set_color("background.listview.odd", semi(invert(background), 0.02));
		set_color("background.selected", get_system_color(COLOR_HIGHLIGHT));

		set_color("background.header.sorted", semi(invert(background), 0.05));

		set_value("border", 1.0f);
		set_value("padding", 3.0f);
		set_value("separator", 1.0f);

		const auto system_font = get_system_font(*_text_engine);
		auto system_font_d = system_font->get_key();

		system_font_d.weight = semi_bold;
		system_font_d.height += 1;
		set_font("text", system_font);
		set_font("text.header", _text_engine->create_font(system_font_d));
	}
}
