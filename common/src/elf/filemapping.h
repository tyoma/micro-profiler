#pragma once

#include <memory>
#include <utility>

namespace symreader
{
	typedef std::pair<const void *, size_t> mapped_region;

	std::shared_ptr<const mapped_region> map_file(const char *path);
}
