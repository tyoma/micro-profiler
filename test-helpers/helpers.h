#pragma once

#include <common/primitives.h>

#include <algorithm>
#include <mt/thread.h>
#include <string>
#include <vector>
#include <ut/assert.h>

namespace micro_profiler
{
	namespace tests
	{
		template <typename U, typename V>
		U address_cast_hack(V v)
		{
			union {
				U u;
				V v2;
			};
			v2 = v;
			U assertion[sizeof(u) == sizeof(v) ? 1 : 0] = { u };
			return assertion[0];
		}

		class com_event
		{
		public:
			com_event();

			void signal();
			void wait();

		private:
			std::shared_ptr<void> _handle;
		};

		struct running_thread
		{
			virtual ~running_thread() throw() { }
			virtual void join() = 0;
			virtual mt::thread::id get_id() const = 0;
			virtual bool is_running() const = 0;
			
			virtual void suspend() = 0;
			virtual void resume() = 0;
		};

		std::wstring get_current_process_executable();

		namespace this_thread
		{
			void sleep_for(unsigned int duration);
			std::shared_ptr<running_thread> open();
		};

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
	}

	bool operator <(const function_statistics &lhs, const function_statistics &rhs);
	bool operator ==(const function_statistics &lhs, const function_statistics &rhs);
}

namespace ut
{
	inline void are_equal(double lhs, double rhs, const LocationInfo &i_location)
	{
		const double tolerance = 0.0000001;
		double d = lhs - rhs, s = 0.5 * (lhs + rhs);

		if (s && (d /= s, d < -tolerance || tolerance < d))
			throw FailedAssertion("Values are equal!", i_location);
	}
}
