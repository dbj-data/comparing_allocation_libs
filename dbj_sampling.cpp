#include <vector>
#include <array>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "dbj--nanolib/dbj++tu.h"
#include "trivial_pool_allocator_class.h"
#include "dbj_nanostring.h"
#include "klib.h"

// #define NEDMALLOC_DEBUG 1
// #define ENABLE_LOGGING 0xffffffff 
// NEDMALLOC_TESTLOGENTRY
#define NO_NED_NAMESPACE
#include "nedmalloc/nedmalloc.h"

/// https://godbolt.org/z/fAR6Hd
/// https://github.com/attractivechaos/benchmarks
#define DBJ_ASSERT _ASSERTE

static void fprint_magic(int padding, int amount ) 
{
	static const char MASK[]{ "-------------------------------------------------------" };
	// int amount = 520;
	// char number[100]{ 0 };
	// sprintf_s(number, 100, "%d.%02d", amount / 100, amount % 100);
	// sprintf_s(number, 100, "%d", amount );
	// volatile int magick = (int)(sizeof MASK - 1 - strlen(number));
	DBJ_PRINT("%.*s%d", padding, MASK, amount);
}

// TUF_REG(fprint_magic );
/// ---------------------------------------------------------------------
constexpr int test_loop_size = 0xF, test_array_size = 2000000; // 200K instead of 2 mil
/// ---------------------------------------------------------------------
static inline int randomizer(int max_ = 0xFF, int min_ = 1)
{
	static auto _ = [] {
		srand((unsigned)time(NULL)); return true;
	}();

	return (rand() % max_ + min_);
}
/// ---------------------------------------------------------------------
static inline auto driver = [](auto prompt_, auto specimen)
{
	volatile clock_t time_point_ = clock();
	specimen();
	float rez = (float)(clock() - time_point_) / CLOCKS_PER_SEC;
	DBJ_PRINT("%-20s %.3f sec,  %.0f dbj's", prompt_, rez, 1000 * rez);
};

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
	// using pool allocator is cheating
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
	// currently, I have no clue what are the good values here
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

static inline void meta_comparator() 
{
	DBJ_REPEAT(4) {
		DBJ_PRINT(DBJ_BG_RED "%*d  " DBJ_RESET, 40, dbj_repeat_counter_);
		compare_mem_mechanisms();
	}
}
TUF_REG(meta_comparator);

#ifdef COMPARE_KVEC_AND_STD_VEC
///-----------------------------------------------
TU_REGISTER([] {
	int i{}, j{};
	volatile clock_t t = clock();
	DBJ_REPEAT(test_loop_size) {
		int* array_ = DBJ_ALLOC(int, test_array_size);
		DBJ_ASSERT(array_);
		DBJ_REPEAT(test_array_size)
		{
			array_[dbj_repeat_counter_] = dbj_repeat_counter_;
		}
		DBJ_FREE(array_);
	}
	DBJ_PRINT("C array_, preallocated: %.3f sec",
		(float)(clock() - t) / CLOCKS_PER_SEC);
	});
///-----------------------------------------------
TU_REGISTER([] {
	int i{}, j{};
	volatile clock_t t = clock();
	DBJ_REPEAT(test_loop_size) {
		int* tmp_, * array_ = 0, max = 0;
		DBJ_REPEAT(test_array_size)
		{
			int const& j = dbj_repeat_counter_;
			if (j == max) {
				max = !max ? 1 : max << 1;
				tmp_ = (int*)DBJ_REALLOC(array_, sizeof(int) * max);
				DBJ_ASSERT(tmp_);
				array_ = tmp_;
			}
			array_[j] = j;
		}
		DBJ_FREE(array_);
	}
	DBJ_PRINT("C array_, dynamic: %.3f sec",
		(float)(clock() - t) / CLOCKS_PER_SEC);
	});
///-----------------------------------------------
TU_REGISTER([] {
	int i{}, j{};
	volatile clock_t t = clock();
	DBJ_REPEAT(test_loop_size) {
		kvec_t(int) array_;
		kv_init(array_);
		kv_resize(int, array_, test_array_size);
		DBJ_REPEAT(test_array_size)
			kv_a(int, array_, dbj_repeat_counter_) = dbj_repeat_counter_;
		kv_destroy(array_);
	}
	DBJ_PRINT("C vector, dynamic(kv_a): %.3f sec",
		(float)(clock() - t) / CLOCKS_PER_SEC);
	});
///-----------------------------------------------
TU_REGISTER([] {
	int i{}, j{};
	volatile clock_t t = clock();
	DBJ_REPEAT(test_loop_size) {
		kvec_t(int) array_;
		kv_init(array_);
		DBJ_REPEAT(test_array_size)
			kv_push(int, array_, dbj_repeat_counter_);
		kv_destroy(array_);
	}
	DBJ_PRINT("C vector, dynamic(kv_push): %.3f sec",
		(float)(clock() - t) / CLOCKS_PER_SEC);
	});
///-----------------------------------------------
TU_REGISTER([] {
	volatile clock_t t = clock();
	DBJ_REPEAT(test_loop_size) {
		std::vector<int> array_(size_t(test_array_size), int(0));
		DBJ_REPEAT(test_array_size) array_[dbj_repeat_counter_] = dbj_repeat_counter_;
	}
	DBJ_PRINT("C++ vector, preallocated: %.3f sec",
		(float)(clock() - t) / CLOCKS_PER_SEC);
	});
///-----------------------------------------------
TU_REGISTER([] {
	volatile clock_t t = clock();
	DBJ_REPEAT(test_loop_size) {
		std::vector<int> array_{};
		DBJ_REPEAT(test_array_size) array_.push_back(dbj_repeat_counter_);
	}
	DBJ_PRINT("C++ vector, dynamic: %.3f sec",
		(float)(clock() - t) / CLOCKS_PER_SEC);
	///-----------------------------------------------
	});

#endif // COMPARE_KVEC_AND_STD_VEC

int main(int, char**)
{
	// wow, this is deep namespacing :(
	dbj::nanolib::logging::config::nanosecond_timestamp();
	dbj::tu::testing_system::execute();
	/// exit:

	return 0;
}