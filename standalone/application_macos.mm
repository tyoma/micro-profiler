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

#include "application.h"

#include <common/constants.h>
#include <common/string.h>
#include <frontend/factory.h>
#include <ipc/com/endpoint.h>
#include <wpl/factory.h>
#include <wpl/freetype2/font_loader.h>

using namespace std;
using namespace wpl;

namespace micro_profiler
{
	class application::impl
	{
	public:
		impl();
		~impl();

		void run()
		void stop();

		bool schedule(const queue_task &t, timespan defer_by);

	public:
	private:
		NSAutoreleasePool *_pool;
		NSApplication *_application;
	};


	impl::impl()
	{
		_pool = [[NSAutoreleasePool alloc] init];
		_application = [NSApplication sharedApplication];

		[NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
		[NSApp activateIgnoringOtherApps:YES];
	}

	impl::~impl()
	{	[_pool drain];	}

	void impl::run()
	{	[_application run];	}

	void impl::stop()
	{	[_application stop:nil];	}


	application::application()
		: _impl(new impl)
	{
		const auto text_engine = create_text_engine();
		const factory_context context = {
			make_shared<gcontext::surface_type>(1, 1, 16),
			make_shared<gcontext::renderer_type>(2),
			text_engine,
			make_shared<system_stylesheet>(text_engine),
			nullptr,
			[] {	return timestamp();	},
			[this] (const queue_task &t, timespan defer_by) {	_impl->schedule(t, defer_by);	},
		};

		_factory = wpl::factory::create_default(context);
		setup_factory(*_factory);
	}

	application::~application()
	{	}

	void application::run()
	{	[_application run];	}

	void application::stop()
	{	[_application stop:nil];	}

	void application::clipboard_copy(const string &/*text_utf8*/)
	{	}
}

int main()
{
	application app;

	main(app);
	return 0;
}
catch (const exception &e)
{
	printf("Caught exception: %s...\nExiting!\n", e.what());
	return -1;
}
catch (...)
{
	printf("Caught an unknown exception...\nExiting!\n");
	return -1;
}
