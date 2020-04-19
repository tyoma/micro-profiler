#include "commands-instance.h"

#include "command-ids.h"
#include "helpers.h"

#include <common/string.h>
#include <frontend/file.h>
#include <frontend/function_list.h>
#include <frontend/persistence.h>

#include <strmd/serializer.h>
#include <windows.h>

using namespace std;

namespace micro_profiler
{
	namespace integration
	{
		bool pause_updates::query_state(const context_type &ctx, unsigned /*item*/, unsigned &state) const
		{	return state = (ctx.model->updates_enabled ? enabled : 0) | supported | visible, true;	}

		void pause_updates::exec(context_type &ctx, unsigned /*item*/)
		{	ctx.model->updates_enabled = false;	}


		bool resume_updates::query_state(const context_type &ctx, unsigned /*item*/, unsigned &state) const
		{	return state = (ctx.model->updates_enabled ? 0 : enabled) | supported | visible, true;	}

		void resume_updates::exec(context_type &ctx, unsigned /*item*/)
		{	ctx.model->updates_enabled = true;	}


		bool save::query_state(const context_type &/*ctx*/, unsigned /*item*/, unsigned &state) const
		{	return state = visible | supported | enabled, true;	}

		void save::exec(context_type &ctx, unsigned /*item*/)
		{
			shared_ptr<functions_list> model = ctx.model;
			auto_ptr<write_stream> s = create_file(get_frame_hwnd(ctx.shell), ctx.executable);

			if (s.get())
			{
				strmd::serializer<write_stream, packer> ser(*s);
				snapshot_save<scontext::file_v4>(ser, *model);
			}
		}


		bool clear::query_state(const context_type &/*ctx*/, unsigned /*item*/, unsigned &state) const
		{	return state = enabled | visible | supported, true;	}

		void clear::exec(context_type &ctx, unsigned /*item*/)
		{	ctx.model->clear();	}


		bool copy::query_state(const context_type &/*ctx*/, unsigned /*item*/, unsigned &state) const
		{	return state = enabled | visible | supported, true;	}

		void copy::exec(context_type &ctx, unsigned /*item*/)
		{
			string result_utf8;

			ctx.model->print(result_utf8);

			wstring result = unicode(result_utf8);

			if (::OpenClipboard(NULL))
			{
				if (HGLOBAL gtext = ::GlobalAlloc(GMEM_MOVEABLE, (result.size() + 1) * sizeof(wchar_t)))
				{
					wchar_t *gtext_memory = static_cast<wchar_t *>(::GlobalLock(gtext));

					std::copy(result.c_str(), result.c_str() + result.size() + 1, gtext_memory);
					::GlobalUnlock(gtext_memory);
					::EmptyClipboard();
					::SetClipboardData(CF_UNICODETEXT, gtext);
				}
				::CloseClipboard();
			}
		}
	}
}
