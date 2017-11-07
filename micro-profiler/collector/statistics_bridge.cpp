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
#include "calls_collector.h"

#include <algorithm>
#include <iterator>
#include <windows.h>

using namespace flatbuffers;
using namespace std;

namespace micro_profiler
{
	namespace
	{
		const uint64_t c_call_offset = 5;
		const __int64 c_ticks_resolution(timestamp_precision());

		string utf8(const wstring &from)
		{
			return string(from.begin(), from.end());
		}

		//class ImageLoadInfo : public ::ImageLoadInfo
		//{
		//public:
		//	ImageLoadInfo(const image_load_queue::image_info &from);
		//	ImageLoadInfo(const ImageLoadInfo &other);
		//	~ImageLoadInfo();

		//	const ImageLoadInfo &operator =(const ImageLoadInfo &rhs);
		//};

		//typedef char static_assertion[sizeof(ImageLoadInfo) == sizeof(::ImageLoadInfo) ? 1 : -1];

		//ImageLoadInfo::ImageLoadInfo(const image_load_queue::image_info &from)
		//{
		//	Address = reinterpret_cast<hyper>(from.first);
		//	Path = allocate_string(from.second.c_str());
		//}

		//ImageLoadInfo::ImageLoadInfo(const ImageLoadInfo &other)
		//{
		//	Path = NULL;
		//	*this = other;
		//}

		//ImageLoadInfo::~ImageLoadInfo()
		//{	delete []Path;	}

		//const ImageLoadInfo &ImageLoadInfo::operator =(const ImageLoadInfo &rhs)
		//{
		//	if (&rhs != this)
		//	{
		//		Address = rhs.Address;
		//		delete []Path;
		//		Path = allocate_string(rhs.Path);
		//	}
		//	return *this;
		//}
	}

	void image_load_queue::load(const void *in_image_address)
	{
		scoped_lock l(_mtx);

		_lqueue.push_back(get_module_info(in_image_address));
	}

	void image_load_queue::unload(const void *in_image_address)
	{
		scoped_lock l(_mtx);

		_uqueue.push_back(get_module_info(in_image_address));
	}

	void image_load_queue::get_changes(vector<image_info> &loaded_modules, vector<image_info> &unloaded_modules)
	{
		loaded_modules.clear();
		unloaded_modules.clear();

		scoped_lock l(_mtx);

		loaded_modules.insert(loaded_modules.end(), _lqueue.begin(), _lqueue.end());
		unloaded_modules.insert(unloaded_modules.end(), _uqueue.begin(), _uqueue.end());
		_lqueue.clear();
		_uqueue.clear();
	}

	image_load_queue::image_info image_load_queue::get_module_info(const void *in_image_address)
	{
		HMODULE image_address = 0;
		wchar_t image_path[MAX_PATH + 1] = { 0 };

		::GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
			reinterpret_cast<LPCTSTR>(in_image_address), &image_address);
		::GetModuleFileNameW(image_address, image_path, sizeof(image_path));

		return make_pair(image_address, image_path);
	}


	statistics_bridge::statistics_bridge(calls_collector_i &collector,
			const function<void (IProfilerFrontend **frontend)> &factory,
			const shared_ptr<image_load_queue> &image_load_queue)
		: _analyzer(collector.profiler_latency()), _collector(collector), _frontend(0),
			_image_load_queue(image_load_queue)
	{
		factory(&_frontend);
		if (_frontend)
		{
			dispatch(CreateProcessInitializationPayload(_fbuilder, c_ticks_resolution,
				_fbuilder.CreateString(utf8(image_load_queue::get_module_info(0).second))));
		}
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
		vector<image_load_queue::image_info> loaded, unloaded;
		
		_image_load_queue->get_changes(loaded, unloaded);
		if (_frontend)
		{
		//	vector<ImageLoadInfo> loaded2(loaded.begin(), loaded.end());
		//	vector<hyper> unloaded2;

		//	if (!loaded2.empty())
		//		_frontend->LoadImages(static_cast<long>(loaded2.size()), &loaded2[0]);

			vector< Offset<FunctionStatisticsDetailed> > offsets;

			for (analyzer::const_iterator i = _analyzer.begin(); i != _analyzer.end(); ++i)
			{
				FunctionStatistics statistics(reinterpret_cast<uint64_t>(i->first) - c_call_offset, i->second.times_called,
					i->second.max_reentrance, i->second.inclusive_time, i->second.exclusive_time, i->second.max_call_time);

				offsets.push_back(CreateFunctionStatisticsDetailed(_fbuilder, &statistics));
			}
			if (_analyzer.begin() != _analyzer.end())
				dispatch(CreateUpdateStatisticsPayload(_fbuilder, _fbuilder.CreateVector(offsets)));

		//	transform(unloaded.begin(), unloaded.end(), back_inserter(unloaded2), xform());
		//	if (!unloaded2.empty())
		//		_frontend->UnloadImages(static_cast<long>(unloaded2.size()), &unloaded2[0]);
		}
		_analyzer.clear();
	}

	template <typename T>
	void statistics_bridge::dispatch(Offset<T> payload)
	{
		SAFEARRAY message = { 0 };

		_fbuilder.Finish(CreateMessage(_fbuilder, PayloadTraits<T>::enum_value, payload.Union()));
		message.pvData = _fbuilder.GetBufferPointer();
		_frontend->Dispatch(&message);
	}
}
