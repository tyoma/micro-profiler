#pragma once

#include <common/primitives.h>

#include <algorithm>
#include <memory>
#include <mt/thread.h>
#include <stdlib.h>
#include <string>
#include <vector>
#include <ut/assert.h>

namespace micro_profiler
{
	namespace tests
	{
		template <typename U, typename V>
		inline U address_cast_hack(V v)
		{
			union {
				U u;
				V v2;
			};
			v2 = v;
			U assertion[sizeof(u) == sizeof(v) ? 1 : 0] = { u };
			return assertion[0];
		}

		std::wstring get_current_process_executable();

		class image : private std::shared_ptr<void>
		{
			std::wstring _fullpath;

		public:
			explicit image(const wchar_t *path);

			long_address_t load_address() const;
			const wchar_t *absolute_path() const;
			void *get_symbol_address(const char *name) const;
			template <typename T>
			T *get_symbol(const char *name) const;
		};

		class vector_adapter
		{
		public:
			vector_adapter();

			void write(const void *buffer, size_t size);
			void read(void *buffer, size_t size);
			void rewind(size_t pos = 0);
			size_t end_position() const;

		public:
			size_t ptr;
			std::vector<unsigned char> buffer;
		};



		template <typename T>
		inline T *image::get_symbol(const char *name) const
		{	return reinterpret_cast<T *>(reinterpret_cast<uintptr_t>(get_symbol_address(name)));	}


		template <typename T, size_t size>
		inline T *array_end(T (&array_ptr)[size])
		{	return array_ptr + size;	}

		template <typename T, size_t size>
		inline size_t array_size(T (&)[size])
		{	return size;	}

		template <typename KeyT, typename ValueT, typename CompT>
		inline std::vector< std::pair<KeyT, ValueT> > mkvector(const std::unordered_map<KeyT, ValueT, CompT> &from)
		{	return std::vector< std::pair<KeyT, ValueT> >(from.begin(), from.end());	}

		template <typename T, size_t size>
		inline std::vector<T> mkvector(T (&array_ptr)[size])
		{	return std::vector<T>(array_ptr, array_ptr + size);	}

		inline bool mem_equal(const void *lhs, const void *rhs, size_t length)
		{
			return std::equal(static_cast<const byte *>(lhs), static_cast<const byte *>(lhs) + length,
				static_cast<const byte *>(rhs));
		}
	}

	bool operator <(const function_statistics &lhs, const function_statistics &rhs);
	bool operator ==(const function_statistics &lhs, const function_statistics &rhs);
}

extern "C" int setenv(const char *name, const char *value, int overwrite);
