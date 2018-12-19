#pragma once

#include <common/noncopyable.h>
#include <common/types.h>
#include <memory>
#include <string>

namespace micro_profiler
{
	class write_stream : noncopyable
	{
	public:
		write_stream(const std::wstring &path);

		void write(const byte *buffer, size_t size);

	private:
		const std::shared_ptr<void> _file;
	};

	class read_stream : noncopyable
	{
	public:
		read_stream(const std::wstring &path);

		void read(byte *buffer, size_t size);

	private:
		const std::shared_ptr<void> _file;
	};
}
