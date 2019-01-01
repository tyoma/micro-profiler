#include "commands-instance.h"

#include "command-ids.h"
#include "helpers.h"

#include <common/serialization.h>
#include <common/string.h>
#include <frontend/file.h>
#include <frontend/function_list.h>

#include <strmd/serializer.h>
#include <windows.h>

using namespace std;

namespace micro_profiler
{
	namespace integration
	{
		save::save()
			: instance_command(cmdidSaveStatistics)
		{	}

		bool save::query_state(const context_type &/*ctx*/, unsigned /*item*/, unsigned &state) const
		{	return state = visible | supported | enabled, true;	}

		void save::exec(context_type &ctx, unsigned /*item*/)
		{
			shared_ptr<functions_list> model = ctx.model;
			auto_ptr<write_stream> s = create_file(get_frame_hwnd(ctx.shell), ctx.executable);

			if (s.get())
			{
				strmd::serializer<write_stream, packer> ser(*s);
				model->save(ser);
			}
		}


		clear::clear()
			: instance_command(cmdidClearStatistics)
		{	}

		bool clear::query_state(const context_type &/*ctx*/, unsigned /*item*/, unsigned &state) const
		{	return state = enabled | visible | supported, true;	}

		void clear::exec(context_type &ctx, unsigned /*item*/)
		{	ctx.model->clear();	}


		copy::copy()
			: instance_command(cmdidCopyStatistics)
		{	}

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
