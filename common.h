
#ifndef COMMON_INC
#define COMMON_INC

// NOTE: NDEBUG is standard !
#if !defined( _DEBUG ) &&  !defined( DEBUG ) && !defined(NDEBUG) 
#define NDEBUG
#endif // !_DEBUG and !DEBUG and NDEBUG

#ifdef __clang__
#pragma clang system_header
#endif // __clang__

#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS 1
#endif // _CRT_SECURE_NO_WARNINGS

/// ---------------------------------------------------------------------
#include <assert.h>
#include <malloc.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
/// ---------------------------------------------------------------------

#define DBJ_KMEM_SAMPLING
#ifdef DBJ_KMEM_SAMPLING
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

#define DBJ_KALLOC(T_,CNT_) (T_*)kcalloc( k_memory_(), CNT_, sizeof(T_))
#define DBJ_KREALLOC(P_, S_) krealloc( k_memory_() , P_, S_ )
#define DBJ_KFREE( P_ ) kfree( k_memory_(), (void *)P_  )

} // "C"

/// ---------------------------------------------------------------------
/// this is "macros only" vector in C
/// #include "kalloc/kvec.h"
#endif // DBJ_KMEM_SAMPLING
/// ---------------------------------------------------------------------

#ifdef __cplusplus
#include <vector>
#include <array>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "dbj--nanolib/dbj++tu.h"

/// ---------------------------------------------------------------------
static inline int randomizer(int max_ = 0xFF, int min_ = 1)
{
	/// once
	static auto _ = [] {
		srand((unsigned)time(NULL)); return true;
	}();

	return (rand() % max_ + min_);
}
/// ---------------------------------------------------------------------
inline auto driver = [](auto prompt_, auto specimen)
{
	volatile clock_t time_point_ = clock();
	specimen();
	float rez = (float)(clock() - time_point_) / CLOCKS_PER_SEC;
	DBJ_PRINT("%-20s %.3f sec", prompt_, rez);
};
/// ---------------------------------------------------------------------
#endif // __cplusplus

#endif // !COMMON_INC
