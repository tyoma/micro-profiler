#pragma once

#include <common/primitives.h>

#include <memory>
#include <string>
#include <wpl/base/concepts.h>

namespace micro_profiler
{
	class write_stream : wpl::noncopyable
	{
	public:
		write_stream(const std::wstring &path);

		void write(const byte *buffer, size_t size);

	private:
		std::shared_ptr<void> _file;
	};

	class read_stream : wpl::noncopyable
	{
	public:
		read_stream(const std::wstring &path);

		void read(byte *buffer, size_t size);

	private:
		std::shared_ptr<void> _file;
	};
}
