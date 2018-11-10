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
#include <collector/dynamic_hooking.h>

#include "assembler_intel.h"

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
	namespace
	{
		void CC_(fastcall) profile_enter(void *instance, const void *callee, timestamp_t timestamp, void **return_address_ptr) _CC(fastcall)
		{	static_cast<calls_collector *>(instance)->profile_enter(callee, timestamp, return_address_ptr);	}

		void *CC_(fastcall) profile_exit(void *instance, timestamp_t timestamp) _CC(fastcall)
		{	return static_cast<calls_collector *>(instance)->profile_exit(timestamp);	}
	}

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
			const unsigned cl = cutoff_length((byte*)location, size, sizeof(intel::jmp_rel_imm32));

			if (!cl)
				throw runtime_error("Unable to identify required length of the cutoff piece!");

			_size = c_thunk_size + cl + sizeof(intel::jmp_rel_imm32);

			if (::IsBadReadPtr(location, cl))
				throw runtime_error("Cannot patch function!");;

			_thunk = executable_memory::allocate(em, _size);
			_em = em;

			// initialize thunk
			byte *cutoff = (byte *)(_thunk + c_thunk_size);
			intel::jmp_rel_imm32 &jmp_back_to_proc = *(intel::jmp_rel_imm32*)(cutoff + cl);

			initialize_hooks(_thunk, cutoff, _location, micro_profiler::calls_collector::instance(),
				&profile_enter, &profile_exit);
			memcpy(cutoff, _location, cl);
			relocate_calls(cutoff, _location, cl);
			jmp_back_to_proc.init(_location + cl);

			// place hooking jump to original body
			intel::jmp_rel_imm32 &jmp_original = *(intel::jmp_rel_imm32 *)(location);
			
			jmp_original.init(_thunk);
			memset(&jmp_original + 1, 0xCC, cl - sizeof(intel::jmp_rel_imm32));
		}

		~patch()
		{
			scoped_unprotect su(_location, sizeof(intel::jmp_rel_imm32));

			memcpy(_location, _thunk + c_thunk_size, sizeof(intel::jmp_rel_imm32));
		}

	private:
		static unsigned cutoff_length(byte * const location0, int size, int required_size)
		{
			byte *location = location0;

			while (required_size > 0 && size > 0)
			{
				if (*location == 0xCC)
					throw runtime_error("Cannot patch function (int 3 met)!");
				else if (*location == 0xEB)
					throw runtime_error("Cannot patch function (jmp rel8 met)!");

				unsigned l = length_disasm(location);

				required_size -= l;
				size -= l;
				location += l;
			}
			if (required_size > 0 && size <= 0)
				throw runtime_error("Cannot patch function (the function is too short)!");
			return location - location0;
		}

		void relocate_calls(byte * const location0, byte * const original0, const int size0)
		{
			intel::dword d = original0 - location0;
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
					byte *target_address = opcode_src + 5 + *(intel::dword *)(opcode_src + 1);

					if (::IsBadReadPtr(target_address, 4))
						throw runtime_error("Cannot patch function (call to invalid address)!");
					*(intel::dword *)(opcode_dest + 1) += d;
				}
				else if (*opcode_dest == 0xE9 /* jmp */)
				{
					byte *target_address = opcode_src + 5 + *(intel::dword *)(opcode_src + 1);

					if (::IsBadReadPtr(target_address, 4))
						throw runtime_error("Cannot patch function (jmp to invalid address)!");
					if (target_address < original0 || target_address > original0 + size0)
						*(intel::dword *)(opcode_dest + 1) += d;
				}
			}
		}

	private:
		shared_ptr<executable_memory> _em;
		byte * const _location;
		unsigned _size;
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
				if (si.size > 40 /*&& si.name[0] != '_'*/)
					_patches.push_back(make_shared<patch>(em, si.location, si.size));
			}
			catch (const exception &e)
			{
				cout << si.name << " - " << e.what() << endl;
			}
		});
	}
}
