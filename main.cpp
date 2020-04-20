#include "common.h"

/// ---------------------------------------------------------------------
#define MEM_ALLOC_COMPARISONS
#ifdef MEM_ALLOC_COMPARISONS

#include "pool_allocator/shoshnikov_pool_allocator.h"
/// ---------------------------------------------------------------------
/// nedmalloc primary purpose is multithreaded applications
/// it is also notoriously difficult to use in its raw form
///
#define  NEDMALLOC_DEBUG 0
#undef   ENABLE_LOGGING  /* 0xffffffff  */
#define  NEDMALLOC_TESTLOGENTRY 0
#define NO_NED_NAMESPACE
#include "nedmalloc/nedmalloc.h"
/// ---------------------------------------------------------------------
constexpr int test_loop_size = 0xF, test_array_size = 1000 * 0xFF ; 
/// ---------------------------------------------------------------------
/// compare memory mechatronics
static void compare_mem_mechanisms() {

	// ----------------------------------------------------------
#ifdef DBJ_KMEM_SAMPLING
	driver("kmem", [&] {
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
#endif // DBJ_KMEM_SAMPLING
	// ----------------------------------------------------------
	{
		// using pool allocator is cheating :)
		// it can deliver only fixed size chunks
		// it is not general purpose allocator

		driver("Shoshnikov", [&] {

			static dbj::nanolib::PoolAllocator  tpa(64 /* chunks per block */);

			DBJ_REPEAT(test_loop_size) {
				int* array_ = (int*)tpa.allocate(test_array_size);
				DBJ_ASSERT(array_);
				// do something so it is not opimized away
				DBJ_REPEAT(test_array_size / 4) {
					array_[dbj_repeat_counter_] = randomizer();
				}
				tpa.deallocate((void*)array_);
			}
			});
	}
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
#ifdef DBJ_TESTING_NED_POOL
	// currently, I probably have no clue 
	// what are the good values here
	{
		static nedpool* pool_ = nedcreatepool(2 * test_array_size, 0);

		driver("NED14 Pool", [&] {
			DBJ_REPEAT(test_loop_size) {
				volatile int* array_ = (volatile int*)::nedpcalloc(pool_, test_array_size, sizeof(int));
				DBJ_ASSERT(array_);
				// do something so it is not opimized away
				DBJ_REPEAT(test_array_size / 4) {
					array_[dbj_repeat_counter_] = randomizer();
				}
				::nedpfree(pool_, (void*)array_);
			}
			});

		neddestroypool(pool_);
		// also not sure if this is necessary
		neddisablethreadcache(0);
	}
#endif // DBJ_TESTING_NED_POOL
}

/// ---------------------------------------------------------------------
static void meta_comparator()
{
	/// repeat the test N times
	/// so far no big differences
	DBJ_REPEAT(test_loop_size) {
		DBJ_PRINT(DBJ_FG_RED "%*d  " DBJ_RESET, 40, 1 + dbj_repeat_counter_);
		compare_mem_mechanisms();
	}
	DBJ_PRINT(" ");
	DBJ_PRINT("Comparing system and 3 other mem allocations ");
	DBJ_PRINT("Allocating/deallocating int * array[%d] with some random data filling", int(test_array_size));
	DBJ_PRINT("Repeating the lot %d times", int(test_loop_size));
	DBJ_PRINT(" ");
}

/// ---------------------------------------------------------------------
/// this form ot TU Function registration works
/// with clang 9, coming with VS 2019
TUF_REG(meta_comparator);
#endif // MEM_ALLOC_COMPARISONS

/// ---------------------------------------------------------------------
int main(int, char**)
{
	return dbj::tu::testing_system::execute();
}