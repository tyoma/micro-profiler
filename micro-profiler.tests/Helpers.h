#pragma once

#include <common/primitives.h>

#include <wpl/mt/thread.h>
#include <algorithm>
#include <string>
#include <tchar.h>
#include <vector>

namespace micro_profiler
{
	namespace tests
	{
		struct running_thread
		{
			virtual ~running_thread() throw() { }
			virtual void join() = 0;
			virtual wpl::mt::thread::id get_id() const = 0;
			virtual bool is_running() const = 0;
			
			virtual void suspend() = 0;
			virtual void resume() = 0;
		};

		std::wstring get_current_process_executable();

		bool is_com_initialized();

		namespace this_thread
		{
			void sleep_for(unsigned int duration);
			wpl::mt::thread::id get_id();
			std::shared_ptr<running_thread> open();
		};

		class image : private std::shared_ptr<void>
		{
			std::wstring _fullpath;

		public:
			explicit image(const wchar_t *path);

			long_address_t load_address() const;
			const wchar_t *absolute_path() const;
			const void *get_symbol_address(const char *name) const;
		};

		class vector_adapter
		{
		public:
			vector_adapter();

			void write(const void *buffer, size_t size);
			void read(void *buffer, size_t size);
			void rewind(size_t pos = 0);
			size_t end_position() const;

		private:
			size_t _ptr;
			std::vector<unsigned char> _buffer;
		};



		template <typename T, size_t size>
		inline T *array_end(T (&array_ptr)[size])
		{	return array_ptr + size;	}

		template <typename T, size_t size>
		inline size_t array_size(T (&)[size])
		{	return size;	}

		template <typename KeyT, typename ValueT, typename CompT>
		inline std::vector< std::pair<KeyT, ValueT> > mkvector(const std::unordered_map<KeyT, ValueT, CompT> &from)
		{	return std::vector< std::pair<KeyT, ValueT> >(from.begin(), from.end());	}
	}

	bool operator <(const function_statistics &lhs, const function_statistics &rhs);
	bool operator ==(const function_statistics &lhs, const function_statistics &rhs);
}
