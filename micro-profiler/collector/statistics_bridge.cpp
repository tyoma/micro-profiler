//	Copyright (c) 2011-2015 by Artem A. Gevorkyan (gevorkyan.org)
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

#include "statistics_bridge.h"

#include "../_generated/frontend.h"
#include "../common/protocol.h"
#include "../common/serialization.h"
#include "calls_collector.h"

#include <algorithm>
#include <iterator>
#include <strmd/strmd/serializer.h>

using namespace std;

namespace micro_profiler
{
	namespace
	{
		const timestamp_t c_ticks_resolution(timestamp_precision());

		class vector_writer
		{
		public:
			vector_writer(vector<byte> &buffer)
				: _buffer(buffer)
			{	_buffer.clear();	}

			void write(const void *data, size_t size)
			{	_buffer.insert(_buffer.end(), static_cast<const byte *>(data), static_cast<const byte *>(data) + size);	}

		private:
			void operator =(const vector_writer &other);

		private:
			vector<byte> &_buffer;
		};
	}

	void image_load_queue::load(const void *in_image_address)
	{
		scoped_lock l(_mtx);

		_lqueue.push_back(get_module_info(in_image_address));
	}

	void image_load_queue::unload(const void *in_image_address)
	{
		scoped_lock l(_mtx);

		_uqueue.push_back(get_module_info(in_image_address).first);
	}

	void image_load_queue::get_changes(loaded_modules &loaded_modules_, unloaded_modules &unloaded_modules_)
	{
		loaded_modules_.clear();
		unloaded_modules_.clear();

		scoped_lock l(_mtx);

		loaded_modules_.insert(loaded_modules_.end(), _lqueue.begin(), _lqueue.end());
		unloaded_modules_.insert(unloaded_modules_.end(), _uqueue.begin(), _uqueue.end());
		_lqueue.clear();
		_uqueue.clear();
	}

	module_info image_load_queue::get_module_info(const void *in_image_address)
	{
		HMODULE image_address = 0;
		wchar_t image_path[MAX_PATH + 1] = { 0 };

		::GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
			reinterpret_cast<LPCTSTR>(in_image_address), &image_address);
		::GetModuleFileName(image_address, image_path, sizeof(image_path));

		return make_pair(reinterpret_cast<address_t>(image_address), image_path);
	}


	statistics_bridge::statistics_bridge(calls_collector_i &collector,
			const function<void (IProfilerFrontend **frontend)> &factory,
			const std::shared_ptr<image_load_queue> &image_load_queue)
		: _analyzer(collector.profiler_latency()), _collector(collector), _frontend(0),
			_image_load_queue(image_load_queue)
	{
		factory(&_frontend);
		send(init, initializaion_data(image_load_queue::get_module_info(0).second, c_ticks_resolution));
	}

	statistics_bridge::~statistics_bridge()
	{
		if (_frontend)
			_frontend->Release();
	}

	void statistics_bridge::analyze()
	{	_collector.read_collected(_analyzer);	}

	void statistics_bridge::update_frontend()
	{
		loaded_modules loaded;
		unloaded_modules unloaded;
		
		_image_load_queue->get_changes(loaded, unloaded);
		if (!loaded.empty())
			send(modules_loaded, loaded);
		if (_analyzer.size())
			send(update_statistics, _analyzer);
		if (!unloaded.empty())
			send(modules_unloaded, unloaded);
		_analyzer.clear();
	}

	template <typename DataT>
	void statistics_bridge::send(commands command, const DataT &data)
	{
		if (_frontend)
		{
			vector_writer writer(_buffer);
			strmd::serializer<vector_writer> archive(writer);

			archive(command);
			archive(data);
			_frontend->Dispatch(&_buffer[0], static_cast<long>(_buffer.size()));
		}
	}
}
