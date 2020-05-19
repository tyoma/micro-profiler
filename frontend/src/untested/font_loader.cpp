#include "font_loader.h"

#include <functional>
#include <windows.h>

using namespace agge;
using namespace std;
using namespace std::placeholders;

namespace micro_profiler
{
	namespace
	{
		real_t fixed2real(FIXED value)
		{	return static_cast<real_t>(value.value + value.fract / 65536.0);	}

		glyph::path_point path_point(int command, real_t x, real_t y)
		{
			glyph::path_point p = { command, x, y };
			return p;
		}

		void qbezier(glyph::outline_storage &outline, real_t x2, real_t y2, real_t x3, real_t y3, real_t d = 0.03f)
		{
			const real_t x1 = (outline.end() - 1)->x, y1 = (outline.end() - 1)->y;

			for (real_t t = d; t < 1.0f; t += d)
			{
				real_t t_ = 1.0f - t;

				outline.push_back(path_point(path_command_line_to,
					t_ * t_ * x1 + 2.0f * t_ * t * x2 + t * t * x3,
					t_ * t_ * y1 + 2.0f * t_ * t * y2 + t * t * y3));
			}
			outline.push_back(path_point(path_command_line_to, x3, y3));
		}

		class dc : agge::noncopyable
		{
		public:
			typedef shared_ptr<void> handle;

		public:
			explicit dc()
				: _dc(::CreateCompatibleDC(NULL))
			{	}

			~dc()
			{	::DeleteDC(_dc);	}

			operator HDC() const
			{	return _dc;	}

			handle select(HGDIOBJ obj)
			{	return handle(::SelectObject(_dc, obj), bind(&::SelectObject, _dc, _1));	}

		private:
			HDC _dc;
			handle _bitmap_selector;
		};
	}

	class font_accessor : public font::accessor
	{
	public:
		font_accessor(shared_ptr<void> hfont, font::key::grid_fit grid_fit)
			: _hfont(hfont), _selection(_ctx.select(hfont.get())), _grid_fit(grid_fit)
		{	}

		virtual font::metrics get_metrics() const /*override*/
		{
			font::metrics fm = {};
			TEXTMETRIC tm = {};

			::GetTextMetrics(_ctx, &tm);
			fm.ascent = static_cast<real_t>(tm.tmAscent);
			fm.descent = static_cast<real_t>(tm.tmDescent);
			fm.leading = static_cast<real_t>(tm.tmExternalLeading);
			return fm;
		}

		virtual uint16_t get_glyph_index(wchar_t character) const /*override*/
		{
			WORD index = 0;

			::GetGlyphIndicesW(_ctx, &character, 1, &index, GGI_MARK_NONEXISTING_GLYPHS);
			return index;
		}

		virtual glyph::outline_ptr load_glyph(uint16_t index, glyph::glyph_metrics &m) const /*override*/
		{
			typedef const void *pvoid;

			const UINT format = GGO_GLYPH_INDEX | GGO_NATIVE | GGO_METRICS
				| (font::key::gf_none == _grid_fit ? GGO_UNHINTED : 0);
			const int xfactor = font::key::gf_vertical == _grid_fit ? 20 : 1;
			const MAT2 c_identity = { { 0, (short)xfactor }, { 0, 0 }, { 0, 0 }, { 0, -1 }, };

			GLYPHMETRICS gm;
			const int size = ::GetGlyphOutline(_ctx, index, format, &gm, 0, 0, &c_identity);
			glyph::outline_ptr o;

			if (size == GDI_ERROR)
				return o;

			if (_grid_fit == font::key::gf_strong)
			{
				ABC abc;

				::GetCharABCWidthsI(_ctx, index, 1, 0, &abc);
				m.advance_x = static_cast<real_t>(abc.abcA + abc.abcB + abc.abcC);
			}
			else
				m.advance_x = static_cast<real_t>(gm.gmCellIncX) / xfactor;
			m.advance_y = static_cast<real_t>(gm.gmCellIncY);
			o.reset(new glyph::outline_storage);
			if (size)
			{
				vector<agge::uint8_t> buffer(size);

				::GetGlyphOutline(_ctx, index, format, &gm, size, &buffer[0], &c_identity);
				for (pvoid p = &buffer[0], end = &buffer[0] + size; p != end; )
				{
					const TTPOLYGONHEADER *header = static_cast<const TTPOLYGONHEADER *>(p);
					const pvoid next_poly = static_cast<const agge::uint8_t *>(p) + header->cb;

					p = header + 1;
					o->push_back(path_point(path_command_move_to,
						fixed2real(header->pfxStart.x) / xfactor, fixed2real(header->pfxStart.y)));

					for (const TTPOLYCURVE *curve; curve = static_cast<const TTPOLYCURVE *>(p), p != next_poly;
						p = static_cast<const agge::uint8_t *>(p) + sizeof(TTPOLYCURVE) + (curve->cpfx - 1) * sizeof(POINTFX))
					{
						switch (curve->wType)
						{
						case TT_PRIM_LINE:
							for (WORD i = 0; i != curve->cpfx; ++i)
							{
								o->push_back(path_point(path_command_line_to,
									fixed2real(curve->apfx[i].x) / xfactor, fixed2real(curve->apfx[i].y)));
							}
							break;

						case TT_PRIM_QSPLINE:
							for (WORD i = 0; i < curve->cpfx - 1; ++i)
							{
								POINTFX pnt_b = curve->apfx[i]; // B is always the current point
								POINTFX pnt_c = curve->apfx[i + 1];

								if (i < curve->cpfx - 2) // If not on last spline, compute C
								{
									// midpoint (x,y)
									*(int*)&pnt_c.x = (*(int*)&pnt_b.x + *(int*)&pnt_c.x) / 2;
									*(int*)&pnt_c.y = (*(int*)&pnt_b.y + *(int*)&pnt_c.y) / 2;
								}
								qbezier(*o, fixed2real(pnt_b.x) / xfactor, fixed2real(pnt_b.y),
									fixed2real(pnt_c.x) / xfactor, fixed2real(pnt_c.y));
							}
							break;

						case TT_PRIM_CSPLINE:
							break;
						}
					}
					(o->end() - 1)->command |= path_flag_close;
					p = next_poly;
				}
			}
			return o;
		}

	private:
		dc _ctx;
		shared_ptr<void> _hfont;
		dc::handle _selection;
		font::key::grid_fit _grid_fit;
	};


	font::accessor_ptr font_loader::load(const wchar_t *typeface, int height, bool bold, bool italic,
		font::key::grid_fit grid_fit)
	{
		shared_ptr<void> hfont(::CreateFontW(height, 0, 0, 0, bold ? FW_BOLD : FW_NORMAL, !!italic, FALSE, FALSE, 0,
			OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY, 0, typeface), &::DeleteObject);

		return shared_ptr<font_accessor>(new font_accessor(hfont, grid_fit));
	}
}
