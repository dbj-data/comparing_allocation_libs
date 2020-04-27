
#ifndef COMMON_CONCEPT_INC
#define COMMON_CONCEPT_INC

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
#ifdef DBJ_USING_KALLOC
/// ---------------------------------------------------------------------
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
/// ---------------------------------------------------------------------
#endif // DBJ_USING_KALLOC
/// ---------------------------------------------------------------------

#ifdef __cplusplus
#include <vector>
#include <array>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "dbj--nanolib/dbj++tu.h"

namespace dbj {

	struct collector final {
		char name_[0xFF]{0};
		float min_{};
		float max_{};
		int count_{};

		explicit collector(const char* newname) noexcept : min_(0), max_(0), count_(0) {
			strncpy_s( name_, newname, strlen(newname) );
		}

		void add(float new_time_) 
		{
			if ( count_ == 0) {
				min_ = new_time_;
				max_ = new_time_;
				count_ += 1;
				return;
			}
			count_ += 1;
			if (new_time_ < min_)
				min_ = new_time_;
			else if (new_time_ > max_)
				max_ = new_time_;
			else {/* no op */}
		}

		static void report( collector & clctr_, void (*cb) ( const char *, float, float, int ) ) {
			cb(clctr_.name_, clctr_.min_, clctr_.max_, clctr_.count_ );
		}

	};
	/// ---------------------------------------------------------------------
	static inline int randomizer(int max_ = 0xFF, int min_ = 1)
	{
		static auto _ = [] {
			srand((unsigned)time(NULL)); return true;
		}();

		return (rand() % max_ + min_);
	}
	/// ---------------------------------------------------------------------
	static inline auto driver = [](collector& clctr_ , auto specimen)
	{
		volatile clock_t time_point_ = clock();
		specimen();
		/// float rez = (float)(clock() - time_point_) / CLOCKS_PER_SEC;
		clctr_.add((float)(clock() - time_point_) / CLOCKS_PER_SEC);
		/// printf("%12s %.3f sec ", prompt_, rez);
	};
}
/// ---------------------------------------------------------------------
#endif // __cplusplus

#endif // !COMMON_CONCEPT_INC
