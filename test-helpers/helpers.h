#pragma once

#include <common/primitives.h>
#include <common/range.h>
#include <common/types.h>

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
		struct plural_
		{
			template <typename T>
			std::vector<T> operator +(const T &rhs) const
			{	return std::vector<T>(1, rhs);	}
		} const plural;

		template <typename T>
		inline std::vector<T> operator +(std::vector<T> lhs, const T &rhs)
		{	return lhs.push_back(rhs), lhs;	}

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

		template <typename V>
		struct hack_caster
		{
			hack_caster(V v)
				: _v(v)
			{	}

			template <typename U>
			operator U() const
			{	return address_cast_hack<U>(_v);	}

		private:
			V _v;
		};

		template <typename V>
		inline hack_caster<V> address_cast_hack2(V v)
		{	return hack_caster<V>(v);	}

		guid_t generate_id();

		class image : private std::shared_ptr<void>
		{
		public:
			explicit image(std::string path);

			byte *base_ptr() const;
			long_address_t base() const;
			const char *absolute_path() const;
			void *get_symbol_address(const char *name) const;
			unsigned get_symbol_rva(const char *name) const;
			template <typename T>
			T *get_symbol(const char *name) const;

		private:
			byte *_base;
			std::string _fullpath;
		};

		class vector_adapter
		{
		public:
			vector_adapter();
			vector_adapter(const_byte_range from);

			void write(const void *buffer, size_t size);
			void read(void *buffer, size_t size);
			void skip(size_t size);

			void rewind(size_t pos = 0);
			size_t end_position() const;

		public:
			size_t ptr;
			std::vector<byte> buffer;
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

		template <typename T, size_t size>
		inline std::vector<T> mkvector(T (&array_ptr)[size])
		{	return std::vector<T>(array_ptr, array_ptr + size);	}

		template <typename T, size_t l, size_t m>
		inline std::vector< std::vector<T> > mkvector(T (&array_ptr)[l][m])
		{
			std::vector< std::vector<T> > result;

			for (auto i = std::begin(array_ptr); i != std::end(array_ptr); ++i)
				result.push_back(mkvector(*i));
			return result;
		}

		template <typename T, size_t size>
		inline range<T, size_t> mkrange(T (&array_ptr)[size])
		{	return range<T, size_t>(array_ptr, size);	}

		template <typename T>
		inline range<const T, size_t> mkrange(const std::vector<T> &from)
		{	return range<const T, size_t>(&from[0], from.size());	}

		template <typename T>
		inline bool unique(const std::shared_ptr<T> &p)
		{	return p.use_count() == 1;	}

		inline bool mem_equal(const void *lhs, const void *rhs, size_t length)
		{
			return std::equal(static_cast<const byte *>(lhs), static_cast<const byte *>(lhs) + length,
				static_cast<const byte *>(rhs));
		}

		template <typename CharT>
		inline void toupper(CharT &c)
		{	c = static_cast<CharT>(::toupper(c));	}

		std::shared_ptr<void> occupy_memory(void *start, unsigned int length = 1000);
		std::shared_ptr<void> allocate_edge(); // creates guard + allocated pages, returns a pointer to the boundary.
	}

	inline bool operator ==(const thread_info &lhs, const thread_info &rhs)
	{
		return lhs.native_id == rhs.native_id && lhs.description == rhs.description && lhs.start_time == rhs.start_time
			&& lhs.end_time == rhs.end_time && lhs.cpu_time == rhs.cpu_time && lhs.complete == rhs.complete;
	}
}

extern "C" int setenv(const char *name, const char *value, int overwrite);
extern "C" int unsetenv(const char *name);
