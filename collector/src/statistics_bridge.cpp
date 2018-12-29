//	Copyright (c) 2011-2018 by Artem A. Gevorkyan (gevorkyan.org)
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

#include <collector/statistics_bridge.h>

#include <collector/calibration.h>
#include <collector/calls_collector.h>

#include <algorithm>
#include <common/module.h>
#include <common/protocol.h>
#include <common/serialization.h>
#include <common/time.h>
#include <ipc/endpoint.h>
#include <iterator>
#include <strmd/serializer.h>

using namespace std;

namespace micro_profiler
{
	namespace
	{
		class vector_writer
		{
		public:
			vector_writer(pod_vector<byte> &buffer)
				: _buffer(buffer)
			{	_buffer.clear();	}

			void write(const void *data, size_t size)
			{
				if (size == 1)
					_buffer.push_back(*static_cast<const byte *>(data));
				else
					_buffer.append(static_cast<const byte *>(data), static_cast<const byte *>(data) + size);
			}

		private:
			void operator =(const vector_writer &other);

		private:
			pod_vector<byte> &_buffer;
		};
	}

	void image_load_queue::load(const void *in_image_address)
	{
		mt::lock_guard<mt::mutex> l(_mtx);

		_lqueue.push_back(get_module_info(in_image_address));
	}

	void image_load_queue::unload(const void *in_image_address)
	{
		mt::lock_guard<mt::mutex> l(_mtx);

		_uqueue.push_back(get_module_info(in_image_address).load_address);
	}

	void image_load_queue::get_changes(loaded_modules &loaded_modules_, unloaded_modules &unloaded_modules_)
	{
		loaded_modules_.clear();
		unloaded_modules_.clear();

		mt::lock_guard<mt::mutex> l(_mtx);

		loaded_modules_.insert(loaded_modules_.end(), _lqueue.begin(), _lqueue.end());
		unloaded_modules_.insert(unloaded_modules_.end(), _uqueue.begin(), _uqueue.end());
		_lqueue.clear();
		_uqueue.clear();
	}


	statistics_bridge::statistics_bridge(calls_collector_i &collector, const overhead &overhead_, ipc::channel &frontend,
			const std::shared_ptr<image_load_queue> &image_load_queue_)
		: _analyzer(overhead_.external), _collector(collector), _frontend(frontend), _image_load_queue(image_load_queue_)
	{
		initialization_data idata = {
			get_current_executable(),
			ticks_per_second()
		};
		send(init, idata);
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
		vector_writer writer(_buffer);
		strmd::serializer<vector_writer, packer> archive(writer);

		archive(command);
		archive(data);
		_frontend.message(const_byte_range(_buffer.data(), _buffer.size()));
	}
}
