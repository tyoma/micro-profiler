#pragma once

class noncopyable
{
	noncopyable(const noncopyable &other);
	const noncopyable &operator =(const noncopyable &rhs);
};

struct destructible
{
	virtual ~destructible() throw()	{	}
};
