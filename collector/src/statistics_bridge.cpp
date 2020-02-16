//	Copyright (c) 2011-2019 by Artem A. Gevorkyan (gevorkyan.org)
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
#include <collector/module_tracker.h>

#include <common/module.h>
#include <common/serialization.h>
#include <common/time.h>
#include <ipc/endpoint.h>
#include <strmd/serializer.h>

using namespace std;

namespace micro_profiler
{
	statistics_bridge::statistics_bridge(calls_collector_i &collector, const overhead &overhead_, ipc::channel &frontend,
			const std::shared_ptr<module_tracker> &module_tracker_)
		: _analyzer(overhead_), _collector(collector), _frontend(frontend), _module_tracker(module_tracker_)
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
		
		_module_tracker->get_changes(loaded, unloaded);
		if (!loaded.empty())
			send(modules_loaded, loaded);
		if (_analyzer.size())
			send(update_statistics, _analyzer);
		if (!unloaded.empty())
			send(modules_unloaded, unloaded);
		_analyzer.clear();
	}

	void statistics_bridge::send_module_metadata(unsigned int persistent_id)
	{
		module_tracker::metadata_ptr metadata = _module_tracker->get_metadata(persistent_id);
		module_info_metadata md;

		metadata->enumerate_functions([&] (const symbol_info &symbol) {
			md.symbols.push_back(symbol);
		});

		metadata->enumerate_files([&] (const pair<unsigned, string> &file) {
			md.source_files.push_back(file);
		});

		mt::lock_guard<mt::mutex> lock(_mutex);
		buffer_writer< pod_vector<byte> > writer(_buffer);
		strmd::serializer<buffer_writer< pod_vector<byte> >, packer> archive(writer);

		archive(module_metadata);
		archive(persistent_id);
		archive(md);
		_frontend.message(const_byte_range(_buffer.data(), _buffer.size()));
	}

	template <typename DataT>
	void statistics_bridge::send(commands command, const DataT &data)
	{
		mt::lock_guard<mt::mutex> lock(_mutex);
		buffer_writer< pod_vector<byte> > writer(_buffer);
		strmd::serializer<buffer_writer< pod_vector<byte> >, packer> archive(writer);

		archive(command);
		archive(data);
		_frontend.message(const_byte_range(_buffer.data(), _buffer.size()));
	}
}
