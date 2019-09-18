#pragma once

#include <../include/link.h>

#ifdef __cplusplus
extern "C"
{
#endif

	int dl_iterate_phdr(int (*__callback)(struct dl_phdr_info*, size_t, void*), void* __data);

#ifdef __cplusplus
}
#endif
