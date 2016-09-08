#include <charting/win32/display.h>

#include "../helpers.h"

#include <agge/bitmap.h>
#include <agge/blenders_simd.h>
#include <agge/renderer.h>
#include <agge/platform/win32/bitmap.h>
#include <wpl/ui/win32/window.h>

#pragma warning(disable:4355)

namespace std
{
	using namespace tr1;
}

using namespace agge;
using namespace std;
using namespace std::placeholders;

namespace charting
{
	class display_impl : public display
	{
	public:
		display_impl(HWND hwnd);

	private:
		typedef vector< pair<series_ptr, wpl::slot_connection> > series_list;

	private:
		virtual void add_series(const series_ptr &series);

		void update();
		LRESULT on_message(UINT message, WPARAM wparam, LPARAM lparam,
			const wpl::ui::window::original_handler_t &previous);

	private:
		surface _surface;
		renderer _renderer;
		shared_ptr<wpl::ui::window> _window;
		shared_ptr<void> _connection;
		series_list _series;
	};

	display_impl::display_impl(HWND hwnd)
		: _surface(1, 1), _window(wpl::ui::window::attach(hwnd)),
			_connection(_window->advise(bind(&display_impl::on_message, this, _1, _2, _3, _4)))
	{
	}

	void display_impl::add_series(const series_ptr &series)
	{
		_series.push_back(make_pair(series, series->invalidate += bind(&display_impl::update, this)));
	}

	void display_impl::update()
	{
		fill(_surface, solid_color_brush<agge::simd::blender_solid_color>(0, 0, 0, 255));
		for (series_list::const_iterator i = _series.begin(); i != _series.end(); ++i)
			i->first->draw(_surface, _renderer);
		::InvalidateRect(_window->hwnd(), NULL, TRUE);
	}

	LRESULT display_impl::on_message(UINT message, WPARAM wparam, LPARAM lparam,
		const wpl::ui::window::original_handler_t &previous)
	{
		switch (message)
		{
		case WM_SIZE:
			_surface.resize(LOWORD(lparam), HIWORD(lparam));
			update();
			break;

		case WM_PAINT:
			PAINTSTRUCT ps;

			::BeginPaint(_window->hwnd(), &ps);
			_surface.blit(ps.hdc, ps.rcPaint.left, ps.rcPaint.top, ps.rcPaint.right - ps.rcPaint.left,
				ps.rcPaint.bottom - ps.rcPaint.top);
			::EndPaint(_window->hwnd(), &ps);
			return 0;

		case WM_ERASEBKGND:
			return 0;

		default:
			break;
		}
		return previous(message, wparam, lparam);
	}

	shared_ptr<display> create_display(HWND hwnd)
	{
		return shared_ptr<display>(new display_impl(hwnd));
	}
}
