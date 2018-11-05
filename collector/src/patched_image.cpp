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

#include <collector/patched_image.h>
#include <collector/calls_collector.h>

#include "profiler_patch_x86.h"

#include <common/module.h>
#include <common/symbol_resolver.h>
#include <iostream>
extern "C" {
	#include <ld32.h>
}
#include <windows.h>

using namespace std;

namespace micro_profiler
{
	class executable_memory : wpl::noncopyable
	{
	public:
		~executable_memory()
		{	::VirtualFree(_start, 0, MEM_RELEASE); }

		static byte *allocate(shared_ptr<executable_memory> &em, unsigned size)
		{
			byte *block = 0;

			if (!em || !(block = em->allocate(size)))
				em.reset(new executable_memory);
			if (!block)
				block = em->allocate(size);
			return block;
		}

	private:
		executable_memory(unsigned max_size = 0x00010000)
			: _start((byte *)::VirtualAlloc(0, max_size, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE)),
				_next(_start), _end(_start + max_size)
		{
			if (!_start)
				throw bad_alloc();
		}

		byte *allocate(unsigned size)
		{
			if (size <= _end - _next)
			{
				byte *block = (byte *)_next;
				_next += size;
				return block;
			}
			return 0;
		}

	private:
		byte * const _start, *_next, * const _end;
	};

	class scoped_unprotect : wpl::noncopyable
	{
	public:
		scoped_unprotect(void *address, unsigned size)
		{
			if (!::VirtualProtect(address, size, PAGE_EXECUTE_WRITECOPY, &_access))
				throw runtime_error("Cannot change protection mode!");
		}

		~scoped_unprotect()
		{
			DWORD dummy;
			::VirtualProtect(_address, _size, PAGE_EXECUTE/*_access*/, &dummy);
		}

	private:
		void *_address;
		unsigned _size;
		DWORD _access;
	};

	template <typename T>
	void *address_cast_hack(T p)
	{
		union {
			T pp;
			void *value;
		};
		pp = p;
		return value;
	}

	class patched_image::patch : wpl::noncopyable
	{
	public:
		patch(shared_ptr<executable_memory> &em, void *location, unsigned size)
			: _location(static_cast<byte *>(location)), _size(size)
		{
			scoped_unprotect su(location, size);
			const unsigned thunk_length = sizeof(x86::decorator_msvc) + _size;

			if (::IsBadReadPtr(location, size))
				throw runtime_error("Cannot patch function!");;

			_thunk = executable_memory::allocate(em, thunk_length);
			_em = em;

			// initialize thunk
			x86::decorator_msvc &d = *(x86::decorator_msvc *)(_thunk);
			byte *displaced_body = (byte *)(&d + 1);

			d.init(_location, micro_profiler::calls_collector::instance(),
				address_cast_hack(&micro_profiler::calls_collector::profile_enter),
				address_cast_hack(&micro_profiler::calls_collector::profile_exit));
			memcpy(displaced_body, _location, _size);
			relocate_calls(displaced_body, _location, _size);

			// place hooking jump to original body
			x86::jmp_rel_imm32 &jmp_original = *(x86::jmp_rel_imm32 *)(location);

			jmp_original.init(&d);
		}

		~patch()
		{
			scoped_unprotect su(_location, sizeof(x86::jmp_rel_imm32));

			memcpy(_location, _thunk + sizeof(x86::decorator_msvc), sizeof(x86::jmp_rel_imm32));
		}

	private:
		void relocate_calls(byte *location, byte * const original, int size)
		{
			byte * const location0 = location;
			const int size0 = size;
			x86::dword d = original - location;

			while (size > 0)
			{
				if (*location == 0xCC)
				{
					throw runtime_error("Cannot patch function!");
				}
				else if (*location == 0xE8)
				{
					byte *target_address = original + 5 + *(x86::dword *)(location + 1);

					if (::IsBadReadPtr(target_address, 4))
						throw runtime_error("Cannot patch function!");
					*(x86::dword *)(location + 1) += d;
					size -= 5;
					location += 5;
				}
				else if (*location == 0xE9)
				{
					// check for tail optimized calls
					byte *target_address = original + 5 + *(x86::dword *)(location + 1);

					if (::IsBadReadPtr(target_address, 4))
						throw runtime_error("Cannot patch function!");
					if (target_address < location0 || target_address > location0 + size0)
						*(x86::dword *)(location + 1) += d;
					size -= 5;
					location += 5;
				}
				else
				{
					int l = length_disasm(location);
					size -= l;
					location += l;
				}
			}
		}

	private:
		shared_ptr<executable_memory> _em;
		byte * const _location;
		const unsigned _size;
		byte *_thunk;
	};

	void patched_image::patch_image(void *in_image_address)
	{
		shared_ptr<symbol_resolver> r = symbol_resolver::create();
		module_info mi = get_module_info(in_image_address);
		shared_ptr<executable_memory> em;
		int n = 0;

		r->add_image(mi.path.c_str(), mi.load_address);
		r->enumerate_symbols(mi.load_address, [this, &em, &n] (const symbol_info &si) {
			++n;
			try
			{
				if (20 <= si.size && si.name[0] != '_')
					_patches.push_back(make_shared<patch>(em, si.location, si.size));
			}
			catch (const exception &e)
			{
				cout << si.name << " - " << e.what() << endl;
			}
		});
	}
}
