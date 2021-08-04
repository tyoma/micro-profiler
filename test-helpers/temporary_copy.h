#pragma once

#include <common/noncopyable.h>
#include <string>

namespace micro_profiler
{
	namespace tests
	{
		class temporary_copy : noncopyable
		{
		public:
			temporary_copy(const std::string &source);
			~temporary_copy();

			std::string path() const;

		private:
			std::string _path;
		};



		inline std::string temporary_copy::path() const
		{	return _path;	}
	}
}
