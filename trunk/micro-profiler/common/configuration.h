#pragma once

#include <memory>
#include <string>

namespace std
{
	using tr1::shared_ptr;
}

namespace micro_profiler
{
	struct hive
	{
		virtual ~hive() { }

		virtual std::shared_ptr<hive> create(const char *name) = 0;
		virtual std::shared_ptr<hive> open(const char *name) const = 0;

		virtual void store(const char *name, int value) = 0;
		virtual void store(const char *name, const wchar_t *value) = 0;

		virtual bool load(const char *name, int &value) const = 0;
		virtual bool load(const char *name, std::wstring &value) const = 0;
	};
}
