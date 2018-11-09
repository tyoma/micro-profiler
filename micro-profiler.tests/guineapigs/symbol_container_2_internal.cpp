void bubble_sort(int * volatile begin, int * volatile end)
{
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
