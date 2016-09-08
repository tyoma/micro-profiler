#pragma once

#include <memory>
#include <wpl/base/signals.h>

namespace agge
{
	struct pixel32;
	class renderer;
	template <typename PixelT, typename RawBitmapT>
	class bitmap;

	namespace platform
	{
		class raw_bitmap;
	}
}

namespace std
{
	using namespace tr1;
}

namespace charting
{
	struct series;

	typedef agge::bitmap<agge::pixel32, agge::platform::raw_bitmap> surface;
	typedef agge::renderer renderer;
	typedef std::shared_ptr<series> series_ptr;

	struct series
	{
		virtual void draw(surface &surface, renderer &renderer) = 0;

		wpl::signal<void()> invalidate;
	};

	struct display
	{
		virtual void add_series(const series_ptr &series) = 0;
	};

	std::shared_ptr<display> create_display(void *window);
}
