#pragma once

#include <memory>

namespace std
{
	using tr1::shared_ptr;
}

namespace micro_profiler
{
	struct hive
	{
		virtual ~hive() { }
		virtual void store(const char *name, int value) = 0;
		virtual void store(const char *name, const wchar_t *value) = 0;
		virtual std::shared_ptr<hive> create(const char *name) = 0;
	};
}
