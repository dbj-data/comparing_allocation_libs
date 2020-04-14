#pragma once

#ifdef __clang__
#pragma clang system_header
#endif // __clang__

#define _CRT_SECURE_NO_WARNINGS

#define strncpy strncpy_s
#define strdup _strdup

#include "../klib/kalloc.h"

inline void* k_memory_()
{
	static void* k_memory_single_ = km_init();
	return k_memory_single_;
}

#define DBJ_ALLOC(T_,CNT_) (T_*)kcalloc( k_memory_(), CNT_, sizeof(T_))
#define DBJ_REALLOC(P_, S_) krealloc( k_memory_() , P_, S_ )
#define DBJ_FREE( P_ ) kfree( k_memory_(), P_  )


#include "../klib/kvec.h"

