#pragma once

#include <common/primitives.h>

#include <functional>
#include <memory>
#include <stdio.h>
#include <wpl/base/concepts.h>

namespace micro_profiler
{
	class write_stream : wpl::noncopyable
	{
	public:
		write_stream(const std::wstring &path);

		void write(const byte *buffer, size_t size);

	private:
		std::shared_ptr<FILE> _file;
	};

	class read_stream : wpl::noncopyable
	{
	public:
		read_stream(const std::wstring &path);

		void read(byte *buffer, size_t size);

	private:
		std::shared_ptr<FILE> _file;
	};

	std::auto_ptr<write_stream> create_file(const std::wstring &default_name);
	std::auto_ptr<read_stream> open_file(std::wstring& path);
}
