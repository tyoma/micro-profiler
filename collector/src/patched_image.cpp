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

#include <collector/allocator.h>
#include <collector/binary_translation.h>
#include <collector/calls_collector.h>
#include <collector/dynamic_hooking.h>
#include <collector/binary_image.h>

#include "assembler_intel.h"

#include <common/module.h>
#include <common/symbol_resolver.h>
#include <iostream>
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

	class patched_image::patch : wpl::noncopyable
	{
	public:
			enum { jmp_size = sizeof(intel::jmp_rel_imm32) };

	public:
		patch(executable_memory_allocator &a, const function_body &fn)
			: _target_function(static_cast<byte *>(fn.effective_address())),
				_chunk_length(calculate_function_length(fn.body(), jmp_size))
		{
			scoped_unprotect su(_target_function, fn.size());

			size_t size = c_thunk_size + _chunk_length + jmp_size;

			if (size & 0x0F)
				size &= ~0xF, size += 0x10;

			_memory = a.allocate(size);

			byte *thunk = static_cast<byte *>(_memory.get());

			// initialize thunk
			initialize_hooks(thunk, thunk + c_thunk_size, _target_function, micro_profiler::calls_collector::instance(),
				&profile_enter, &profile_exit);
			move_function(thunk + c_thunk_size, _target_function,
				const_byte_range(fn.body().begin(), _chunk_length));
			reinterpret_cast<intel::jmp_rel_imm32 *>(thunk + c_thunk_size + _chunk_length)
				->init(_target_function + _chunk_length);

			// place hooking jump to original body
			intel::jmp_rel_imm32 &jmp_original = *(intel::jmp_rel_imm32 *)(_target_function);
			
			memcpy(_saved, _target_function, _chunk_length);
			jmp_original.init(thunk);
			memset(_target_function + jmp_size, 0xCC, _chunk_length - jmp_size);
			::FlushInstructionCache(::GetCurrentProcess(), _target_function, fn.size());
		}

		~patch()
		{
			scoped_unprotect su(_target_function, _chunk_length);

			memcpy(_target_function, _saved, _chunk_length);
		}

	private:
		shared_ptr<void> _memory;
		byte * const _target_function;
		const unsigned _chunk_length;
		byte _saved[40];
	};

	void patched_image::patch_image(void *in_image_address)
	{
		std::shared_ptr<binary_image> image = load_image_at((void *)get_module_info(in_image_address).load_address);
		executable_memory_allocator em;
		int n = 0;

		image->enumerate_functions([this, &em, &n] (const function_body &fn) {
			try
			{
				if (fn.size() >= 20)
				{
					_patches.push_back(make_shared<patch>(em, fn));
					++n;
				}
				else
				{
					cout << fn.name() << " - the function is too short!" << endl;
				}
			}
			catch (const exception &e)
			{
				cout << fn.name() << " - " << e.what() << endl;
			}
		});
	}
}
