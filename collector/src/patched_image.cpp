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

			::FlushInstructionCache(::GetCurrentProcess(), location, sizeof(x86::jmp_rel_imm32));
			::FlushInstructionCache(::GetCurrentProcess(), _thunk, thunk_length);
		}

		~patch()
		{
			scoped_unprotect su(_location, sizeof(x86::jmp_rel_imm32));

			memcpy(_location, _thunk + sizeof(x86::decorator_msvc), sizeof(x86::jmp_rel_imm32));
		}

	private:
		void relocate_calls(byte * const location0, byte * const original0, const int size0)
		{
			x86::dword d = original0 - location0;
			int opcode_size, size = size0;

			for (byte *opcode_src = original0, *opcode_dest = location0; size > 0;
				size -= opcode_size, opcode_src += opcode_size, opcode_dest += opcode_size)
			{
				opcode_size = length_disasm(opcode_dest);
				
				if (opcode_size > size)
				{
					throw runtime_error("Cannot patch function (instruction crosses function boundary)!");
				}
				if (*opcode_dest == 0xCC)
				{
					throw runtime_error("Cannot patch function (read as 0xCC)!");
				}
				else if (*opcode_dest == 0xE8 /* call */ )
				{
					byte *target_address = opcode_src + 5 + *(x86::dword *)(opcode_src + 1);

					if (::IsBadReadPtr(target_address, 4))
						throw runtime_error("Cannot patch function (call to invalid address)!");
					*(x86::dword *)(opcode_dest + 1) += d;
				}
				else if (*opcode_dest == 0xE9 /* jmp */)
				{
					byte *target_address = opcode_src + 5 + *(x86::dword *)(opcode_src + 1);

					if (::IsBadReadPtr(target_address, 4))
						throw runtime_error("Cannot patch function (jmp to invalid address)!");
					if (target_address < original0 || target_address > original0 + size0)
						*(x86::dword *)(opcode_dest + 1) += d;
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


		r->add_image(mi.path.c_str(), mi.load_address);
		r->enumerate_symbols(mi.load_address, [this, &em] (const symbol_info &si) {
			try
			{

				if (si.size > 10 /*&& si.name[0] != '_'*/)
					_patches.push_back(make_shared<patch>(em, si.location, si.size));
			}
			catch (const exception &e)
			{
				cout << si.name << " - " << e.what() << endl;
			}
		});
	}
}
