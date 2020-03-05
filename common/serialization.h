//	Copyright (c) 2011-2020 by Artem A. Gevorkyan (gevorkyan.org)
//
//	Permission is hereby granted, free of charge, to any person obtaining a copy
//	of this software and associated documentation files (the "Software"), to deal
//	in the Software without restriction, including without limitation the rights
//	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//	copies of the Software, and to permit persons to whom the Software is
//	furnished to do so, subject to the following conditions:
//
//	The above copyright notice and this permission notice shall be included in
//	all copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
//	THE SOFTWARE.

#pragma once

#include "module.h"
#include "noncopyable.h"
#include "primitives.h"
#include "protocol.h"
#include "range.h"

#include <strmd/deserializer.h>

namespace strmd
{
	template <typename KeyT, typename T>
	struct container_reader<std::unordered_map<KeyT, T, micro_profiler::address_compare>, true>
	{
		template <typename ArchiveT, typename ContainerT>
		void operator()(ArchiveT &archive, size_t count, ContainerT &data)
		{
			typename ContainerT::key_type key;

			while (count--)
			{
				archive(key);
				deserialize_statistics(archive, data, key, data[key]);
			}
		}
	};

	template <typename ArchiveT>
	inline void serialize(ArchiveT &archive, const void *&data)
	{	archive(reinterpret_cast<size_t &>(data));	}
}

namespace micro_profiler
{
	typedef strmd::varint packer;

	class buffer_reader : noncopyable
	{
	public:
		buffer_reader(const_byte_range payload);

		void read(void *data, size_t size);

	private:
		const byte *_ptr;
		size_t _remaining;
	};

	template <typename BufferT>
	class buffer_writer : noncopyable
	{
	public:
		buffer_writer(BufferT &buffer);

		void write(const void *data, size_t size);

	private:
		BufferT &_buffer;
	};



	template <typename ArchiveT, typename ContainerT, typename AddressT>
	inline void deserialize_statistics(ArchiveT &archive, ContainerT &/*container*/, AddressT /*key*/,
		function_statistics &data)
	{
		function_statistics v;

		archive(v);
		data += v;
	}

	template <typename ArchiveT, typename ContainerT, typename AddressT>
	inline void deserialize_statistics(ArchiveT &archive, ContainerT &container, AddressT key,
		function_statistics_detailed_t<AddressT> &data)
	{
		deserialize_statistics(archive, container, key, static_cast<function_statistics &>(data));
		archive(data.callees);
		update_parent_statistics(container, key, data.callees);
	}

	template <typename ArchiveT>
	inline void serialize(ArchiveT &archive, initialization_data &data)
	{
		archive(data.executable);
		archive(data.ticks_per_second);
	}	

	template <typename ArchiveT>
	inline void serialize(ArchiveT &archive, function_statistics &data)
	{
		archive(data.times_called);
		archive(data.max_reentrance);
		archive(data.inclusive_time);
		archive(data.exclusive_time);
		archive(data.max_call_time);
	}

	template <typename ArchiveT, typename AddressT>
	inline void serialize(ArchiveT &archive, function_statistics_detailed_t<AddressT> &data)
	{
		archive(static_cast<function_statistics &>(data));
		archive(data.callees);
	}

	template <typename ArchiveT>
	inline void serialize(ArchiveT &archive, commands &data)
	{	archive(reinterpret_cast<int &>(data));	}

	template <typename ArchiveT>
	inline void serialize(ArchiveT &archive, mapped_module_identified &data)
	{
		archive(data.instance_id);
		archive(data.persistent_id);
		archive(data.base);
		archive(data.path);
	}

	template <typename ArchiveT>
	inline void serialize(ArchiveT &archive, symbol_info &data)
	{
		archive(data.id);
		archive(data.name);
		archive(data.rva);
		archive(data.size);
		archive(data.file_id);
		archive(data.line);
	}

	template <typename ArchiveT>
	inline void serialize(ArchiveT &archive, module_info_metadata &data)
	{
		archive(data.symbols);
		archive(data.source_files);
	}


	template <typename BufferT>
	inline buffer_writer<BufferT>::buffer_writer(BufferT &buffer)
		: _buffer(buffer)
	{	_buffer.clear();	}

	template <typename BufferT>
	inline void buffer_writer<BufferT>::write(const void *data, size_t size)
	{
		const byte *data_ = static_cast<const byte *>(data);

		if (size == 1)
			_buffer.push_back(*data_);
		else
			_buffer.append(data_, data_ + size);
	}
}
