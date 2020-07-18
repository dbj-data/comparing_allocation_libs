#ifndef DBJ_KMEM_INC
#define DBJ_KMEM_INC
/*
dbj kalloc front
*/



#include "kalloc.h"

extern "C" {

	void on_exit_release_kmem_pointer();

	inline void* k_memory_()
	{
		static void* k_memory_single_ = km_init();
		/*static int rez =*/ atexit(on_exit_release_kmem_pointer);
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

#endif // DBJ_KMEM_INC
