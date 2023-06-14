//	Copyright (c) 2011-2023 by Artem A. Gevorkyan (gevorkyan.org)
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

#pragma once

#include <common/range.h>
#include <memory>

namespace micro_profiler
{
	class instruction_iterator_
	{
	public:
		instruction_iterator_(range<const byte, size_t> source_range);
		~instruction_iterator_();

		bool fetch();
		byte length() const;
		bool is_rip_based() const;
		const char *mnemonic() const;
		const char *operands() const;

	protected:
		const byte *ptr_() const;

	private:
		struct impl;

	private:
		const std::unique_ptr<impl> _impl;
	};

	template <typename ByteT>
	class instruction_iterator : public instruction_iterator_
	{
	public:
		instruction_iterator(range<ByteT, size_t> source_range);

		ByteT *ptr() const;
	};



	template <typename ByteT>
	inline instruction_iterator<ByteT>::instruction_iterator(range<ByteT, size_t> source_range)
		: instruction_iterator_(source_range)
	{	}

	template <typename ByteT>
	inline ByteT *instruction_iterator<ByteT>::ptr() const
	{	return const_cast<ByteT *>(ptr_());	}
}
