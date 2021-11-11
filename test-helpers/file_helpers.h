#pragma once

#include <string>
#include <vector>

namespace micro_profiler
{
	namespace tests
	{
		class temporary_directory
		{
		public:
			temporary_directory();
			~temporary_directory();

			std::string track_file(const std::string &filename);
			std::string copy_file(const std::string &source);

		private:
			std::string _temp_path;
			std::vector<std::string> _tracked;
		};
	}
}
