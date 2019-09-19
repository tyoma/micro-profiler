extern "C" void bubble_sort(int * volatile begin, int * volatile end)
{
	for (volatile int i = 1, j = 1; i < 1000; j = i, ++i)
		i = i + j;
	for (int *i = begin; i != end; ++i)
		for (int *j = begin; j != end - 1; ++j)
		{
			if (*j > *(j + 1))
			{
				int tmp = *j;

				*j = *(j + 1);
				*(j + 1) = tmp;
			}
		}
	for (int *i = begin; i != end; ++i)
		for (int *j = begin; j != end - 1; ++j)
		{
			if (*j < *(j + 1))
			{
				int tmp = *j;

				*j = *(j + 1);
				*(j + 1) = tmp;
			}
		}
	for (int *i = begin; i != end; ++i)
		for (int *j = begin; j != end - 1; ++j)
		{
			if (*j > *(j + 1))
			{
				int tmp = *j;

				*j = *(j + 1);
				*(j + 1) = tmp;
			}
		}
}
