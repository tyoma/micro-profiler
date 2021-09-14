#pragma once

#include "stream.h"

#include <memory>

typedef struct HWND__ *HWND;

namespace micro_profiler
{
	std::unique_ptr<write_stream> create_file(HWND hparent, const std::string &default_name);
	std::unique_ptr<read_stream> open_file(HWND hparent, std::string& path);
}
