#pragma once

#ifdef __clang__
#pragma clang system_header
#endif // __clang__

#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif // _CRT_SECURE_NO_WARNINGS

#define strncpy strncpy_s
#define strdup _strdup

#include "kalloc/kalloc.h"

extern "C" {

	void on_exit_release_kmem_pointer();

	inline void* k_memory_()
	{
		static void* k_memory_single_ = km_init();
		static int rez = atexit(on_exit_release_kmem_pointer);
		return k_memory_single_;
	}

	inline void on_exit_release_kmem_pointer() 
	{
		km_destroy(k_memory_());
	}

#define DBJ_ALLOC(T_,CNT_) (T_*)kcalloc( k_memory_(), CNT_, sizeof(T_))
#define DBJ_REALLOC(P_, S_) krealloc( k_memory_() , P_, S_ )
#define DBJ_FREE( P_ ) kfree( k_memory_(), (void *)P_  )

} // "C"

#include "kalloc/kvec.h"

