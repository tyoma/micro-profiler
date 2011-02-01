#pragma once

class noncopyable
{
	noncopyable(const noncopyable &other);
	const noncopyable &operator =(const noncopyable &rhs);

public:
	noncopyable()	{	}
};

struct destructible
{
	virtual ~destructible() throw()	{	}
};
