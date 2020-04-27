#pragma once

/// ---------------------------------------------------------------------
#define MEM_ALLOC_COMPARISONS
#ifdef MEM_ALLOC_COMPARISONS

#define DBJ_KMEM_SAMPLING
#ifdef DBJ_KMEM_SAMPLING
#include "kalloc/dbj_kalloc.h"
#endif // DBJ_KMEM_SAMPLING

#include "nvwa/fixed_mem_pool.h"
#include "nvwa/static_mem_pool.h"

#include "shoshnikov_pool_allocator/shoshnikov_pool_allocator.h"
#include "dbj_pool_allocator/dbj_shoshnikov_pool_allocator.h"
#include "dbj_pool_allocator/pool_allocator_sampling.h"
#include "dbj_concept/is_it_feasible.h"
/// ---------------------------------------------------------------------
/// nedmalloc primary purpose is multithreaded applications
/// it is also notoriously difficult to use in its raw form
///
#define  NEDMALLOC_DEBUG 0
#undef   ENABLE_LOGGING  /* 0xffffffff  */
#define  NEDMALLOC_TESTLOGENTRY 0
#define NO_NED_NAMESPACE
#include "nedmalloc/nedmalloc.h"

namespace comparisons {
	/// ---------------------------------------------------------------------
#ifdef NDEBUG
	constexpr int test_loop_size = 2, test_array_size = 10 * 100000 * 50;
#else
	constexpr int test_loop_size = 2, test_array_size = 40 * 100000;
#endif // NDEBUG

	/// ----------------------------------------------------------------------------------
	inline void reporter(const char* name, float min, float max, int count) {
		DBJ_PRINT(DBJ_FG_RED_BOLD "%s " DBJ_RESET "has been tested %3d times, test data size was: %d",
			name, count, test_array_size);
		DBJ_PRINT("Rezults are -- min time: %0.3f sec, max time: %0.3f sec, avg mean time: " DBJ_FG_RED_BOLD " %0.3f sec" DBJ_RESET,
			min, max, (min + max) / 2);
	}
	/// ---------------------------------------------------------------------
	inline auto meta_driver = [&](dbj::collector& collector_, auto aloka, auto dealoka) {
		dbj::driver(collector_, [&] {
			DBJ_REPEAT(test_loop_size) {
				int* array_ = aloka(test_array_size);
				DBJ_ASSERT(array_);
				DBJ_REPEAT(test_array_size / 4) {
					array_[dbj_repeat_counter_] = dbj::randomizer();
				}
				dealoka(array_);
			}
			});
	};
	/// compare memory mechatronics
	inline void compare_mem_mechanisms() {

		// ----------------------------------------------------------
#ifdef DBJ_KMEM_SAMPLING
/*
 kalloc appears to be "curiously slower" ... it really shouldn't be
 it is complex C so I had no time to dive in to investigate
*/
		dbj::collector coll_kmem("KMEM");
		meta_driver(coll_kmem,
			[&](size_t sze_) { return DBJ_KALLOC(int, test_array_size); },
			[&](int* array_) { DBJ_KFREE(array_); }
		);
#endif // DBJ_KMEM_SAMPLING
		// ----------------------------------------------------------
		dbj::collector coll_nvwa_static("NVWA Static");
		{
			using nvwa_pool = nvwa::static_mem_pool<test_array_size>;

			meta_driver(coll_nvwa_static,
				[&](size_t sze_) { return (int*)nvwa_pool::instance_known().allocate(); },
				[&](int* array_) { nvwa_pool::instance_known().deallocate(array_); }
			);
		}
		// ----------------------------------------------------------
		dbj::collector coll_nvwa("NVWA");
		{
			using nvwa_pool = nvwa::fixed_mem_pool<int>;
			static nvwa_pool  nvwa;
			nvwa_pool::initialize(test_array_size);
			_ASSERTE(true == nvwa.is_initialized());

			meta_driver(coll_nvwa,
				[&](size_t sze_) { return (int*)nvwa.allocate(); },
				[&](int* array_) { nvwa.deallocate(array_); }
			);
			nvwa_pool::deinitialize();
		}
		// ----------------------------------------------------------
		/*
		this allocator does not free memory taken from OS.
		in this scenario it will allocate a block size = 4 * test_array_size
		4 * 40 * 1000000 = just above 15MB
		which is a lot of heap reserved by one function
		That makes it kind-of-a difficult for other to cohabit with it ...

		I have changed it only to use HeapAlloc / HeapFree
		*/
		dbj::collector coll_shoshnikov("Shoshnikov");
		{
			static dbj::nanolib::PoolAllocator  tpa(4 /* chunks per block */);

			meta_driver(coll_shoshnikov,
				[&](size_t sze_) { return  (int*)tpa.allocate(test_array_size); },
				[&](int* array_) { tpa.deallocate((void*)array_); }
			);
		}
		// ----------------------------------------------------------
		dbj::collector coll_dbj_shoshnikov("DBJ*Shoshnikov");
		{
			static dbj::shohnikov::dbj_pool_allocator  dbj_pool(
				dbj::shohnikov::legal_block_size::_4
				, test_array_size
			);

			meta_driver(coll_dbj_shoshnikov,
				[&](size_t sze_) { return  (int*)dbj_pool.allocate(); },
				[&](int* array_) { dbj_pool.deallocate((void*)array_); }
			);
		}
		// ----------------------------------------------------------
		dbj::collector coll_dbj_heap_alloc_heap_free("HeapAlloc / HeapFree");
		meta_driver(coll_dbj_heap_alloc_heap_free,
			[&](size_t sze_) { return DBJ_NANO_CALLOC(int, test_array_size); },
			[&](int* array_) { DBJ_NANO_FREE(array_); }
		);
		// ----------------------------------------------------------
		dbj::collector coll_new_delete("new [] / delete []");
		meta_driver(coll_new_delete,
			[&](size_t sze_) { return new int[test_array_size]; },
			[&](int* array_) { delete[] array_; }
		);
		// ----------------------------------------------------------
		dbj::collector coll_ned_14("NED14");
		meta_driver(coll_ned_14,
			[&](size_t sze_) { return (int*)::nedcalloc(test_array_size, sizeof(int)); },
			[&](int* array_) { ::nedfree((void*)array_); }
		);

		// ----------------------------------------------------------
#ifdef DBJ_TESTING_NED_POOL
		dbj::collector coll_ned_14_pool("NED14 Pool");
		// currently, I probably have no clue 
		// what are the good values here
		{
			static nedpool* pool_ = nedcreatepool(2 * test_array_size, 0);

			meta_driver(coll_ned_14_pool,
				[&](size_t sze_) { return (int*)::nedpcalloc(pool_, test_array_size, sizeof(int)); },
				[&](int* array_) { ::nedpfree(pool_, (void*)array_); }
			);

			neddestroypool(pool_);
			// also not sure if this is necessary
			neddisablethreadcache(0);
		}
#endif

		DBJ_PRINT(" ");
		dbj::collector::report(coll_kmem, reporter);
		DBJ_PRINT(" ");
		dbj::collector::report(coll_nvwa_static, reporter);
		DBJ_PRINT(" ");
		dbj::collector::report(coll_nvwa, reporter);
		DBJ_PRINT(" ");
		dbj::collector::report(coll_shoshnikov, reporter);
		DBJ_PRINT(" ");
		dbj::collector::report(coll_dbj_shoshnikov, reporter);
		DBJ_PRINT(" ");
		dbj::collector::report(coll_dbj_heap_alloc_heap_free, reporter);
		DBJ_PRINT(" ");
		dbj::collector::report(coll_new_delete, reporter);
		DBJ_PRINT(" ");
		dbj::collector::report(coll_ned_14, reporter);
		DBJ_PRINT(" ");

	}

	/// ---------------------------------------------------------------------
	void meta_comparator()
	{
		DBJ_PRINT(" ");
		DBJ_PRINT("Comparing system and few other mem allocation mechanisms ");
		DBJ_PRINT("Allocating/deallocating int * array[%d] with some random data filling", int(test_array_size));
		DBJ_PRINT("Repeating the lot %d times", int(test_loop_size));
		DBJ_PRINT(" ");
		/// repeat the test N times
		/// so far no big differences
		DBJ_REPEAT(test_loop_size) {
			DBJ_PRINT(DBJ_FG_RED "%*d  " DBJ_RESET, 40, 1 + dbj_repeat_counter_);
			compare_mem_mechanisms();
		}
	}

	/// ---------------------------------------------------------------------
	/// this form ot TU Function registration works
	/// with clang 9, coming with VS 2019
	/// In English: inline lambda can not be used instead of function
	/// It appears clang will remember it (I am using Array) and will
	/// execute it, but as an empty function.
	TUF_REG(meta_comparator);

} // namespace comparisons

#endif // MEM_ALLOC_COMPARISONS
