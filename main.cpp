#include "common.h"
//
#include "pool_allocator/pool_allocator_sampling.h"
//
#include "kvec_sampling.h"
// to be moved out 
// #define DBJ_NANOSTRING_TEST
#include "dbj_nanostring.h"

/// ---------------------------------------------------------------------
// #define MEM_ALLOC_COMPARISONS
/// ---------------------------------------------------------------------
/// nedmalloc primary purpose is multithreaded applications
/// it is also notoriously difficult to use in its raw form
///
#define NEDMALLOC_DEBUG 0
// #define ENABLE_LOGGING 0xffffffff 
// NEDMALLOC_TESTLOGENTRY
#define NO_NED_NAMESPACE
#include "nedmalloc/nedmalloc.h"
/// ---------------------------------------------------------------------
constexpr int test_loop_size = 0xF, test_array_size = 2000000; // 200K instead of 2 mil


/// ---------------------------------------------------------------------
/// compare memory mechatronics
static inline void compare_mem_mechanisms() {

	// ----------------------------------------------------------
	driver( "kmem", [&] {
		DBJ_REPEAT(test_loop_size) {
			volatile int* array_ = DBJ_ALLOC(int, test_array_size);
			DBJ_ASSERT(array_);
			// do something so it is not opimized away
			DBJ_REPEAT(test_array_size / 4) {
				array_[dbj_repeat_counter_] = randomizer();
			}
			DBJ_FREE(array_);
		}
	});
	// ----------------------------------------------------------
	// using pool allocator is cheating :)
	// it can deliver only fixed size chunks
	// it is not general purpose allocator
	dbj::nanolib::pool_allocator  tpa(2, test_array_size);

	driver("pool", [&] {
		DBJ_REPEAT(test_loop_size) {
			int* array_ = (int*)tpa.allocate();
			DBJ_ASSERT(array_);
			// do something so it is not opimized away
			DBJ_REPEAT(test_array_size / 4) {
				array_[dbj_repeat_counter_] = randomizer();
			}
			tpa.deallocate((void*)array_);
		}
	});
	// ----------------------------------------------------------
	driver("system", [&] {
		DBJ_REPEAT(test_loop_size) {
			volatile int* array_ = (volatile int*)calloc(test_array_size, sizeof(int));
			DBJ_ASSERT(array_);
			// do something so it is not opimized away
			DBJ_REPEAT(test_array_size / 4) {
				array_[dbj_repeat_counter_] = randomizer();
			}
			free((void*)array_);
		}
	});
	// ----------------------------------------------------------
	driver("NED14", [&] {
		DBJ_REPEAT(test_loop_size) {
			volatile int* array_ = (volatile int*)::nedcalloc(test_array_size, sizeof(int));
			DBJ_ASSERT(array_);
			// do something so it is not opimized away
			DBJ_REPEAT(test_array_size / 4) {
				array_[dbj_repeat_counter_] = randomizer();
			}
			::nedfree((void*)array_);
		}
	});
	// ----------------------------------------------------------
	// currently, I probably have no clue 
	// what are the good values here
	nedpool* pool_ = nedcreatepool(2 * test_array_size, 0);
	driver("NED14 Pool", [&] {
		DBJ_REPEAT(test_loop_size) {
			volatile int* array_ = (volatile int*)::nedpcalloc(pool_,test_array_size, sizeof(int));
			DBJ_ASSERT(array_);
			// do something so it is not opimized away
			DBJ_REPEAT(test_array_size / 4) {
				array_[dbj_repeat_counter_] = randomizer();
			}
			::nedpfree( pool_, (void*)array_);
		}
	});

	neddestroypool(pool_);
	// also not sure if this is necessary
	neddisablethreadcache(0);
}

/// ---------------------------------------------------------------------
static inline void meta_comparator()
{
	/// repeat the test 4 times
	/// so far no big differences
	DBJ_REPEAT(4) {
		DBJ_PRINT(DBJ_BG_RED "%*d  " DBJ_RESET, 40, dbj_repeat_counter_);
		compare_mem_mechanisms();
	}
}

/// ---------------------------------------------------------------------
/// this form ot TU Function registration works
/// with clang 9, coming with VS 2019
#ifdef MEM_ALLOC_COMPARISONS
TUF_REG(meta_comparator);
#endif // MEM_ALLOC_COMPARISONS

/// ---------------------------------------------------------------------
int main(int, char**)
{
	return dbj::tu::testing_system::execute();
}