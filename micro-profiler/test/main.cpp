void x(int a )
{
	throw 0;
}

int main()
{
	try
	{
		x(123);
	}
	catch (...)
	{
	}
}
