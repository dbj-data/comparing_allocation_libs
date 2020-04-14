#include <vector>
#include <array>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "dbj--nanolib/dbj++tu.h"
#include "klib.h"
/// https://godbolt.org/z/fAR6Hd
/// https://github.com/attractivechaos/benchmarks
#define DBJ_ASSERT _ASSERTE

extern "C" {

	enum{ slab_max_size_ = 100 };
	typedef struct slab_rezult {
		char * val;
	} slab_rezult;

	inline slab_rezult slab(size_t size)
	{
		size = size % slab_max_size_ ;
		slab_rezult buf{
		 (char*)calloc(size + 1 ,sizeof(char))
		};
		memset(buf.val, ' ', size);
		buf.val[size] = '\0';
		return buf;
	}
}

static void fprint_magic () {
	auto HASH{ "*********" };
		int amount = 520;
		char number[100];
		sprintf_s(number, 100, "%d.%02d", amount / 100, amount % 100);
		volatile int magick = (int)(sizeof HASH - 1 - strlen(number));
		DBJ_PRINT("$%.*s%s",
			magick,
			HASH,
			number );
	}

TUF_REG(fprint_magic );
/// ---------------------------------------------------------------------
constexpr int test_loop_size = 0xF, test_array_size = 2000000; // 200K instead of 2 mil
/// ---------------------------------------------------------------------
/// compare kv mem and system mem
static void compare_kv_mem_and_system_mem() {
	volatile clock_t time_point_ = clock();
	DBJ_REPEAT(test_loop_size) {
		volatile int* array_ = DBJ_ALLOC(int, test_array_size);
		DBJ_ASSERT(array_);
		DBJ_FREE((void *)array_);
	}
	float rezuldad = (float)(clock() - time_point_) / CLOCKS_PER_SEC;
	DBJ_PRINT(" ");
	DBJ_PRINT("k mem alloc/free: %.3f sec,  %.0f dbj's", rezuldad, 1000 * rezuldad );
	slab_rezult slab_ = slab(1000 * rezuldad);
	DBJ_PRINT(DBJ_BG_GREEN  "%s" DBJ_RESET, slab_.val);
	free( slab_.val );

	time_point_ = clock();
	DBJ_REPEAT(test_loop_size) {
		volatile int* array_ = (volatile int *)calloc(test_array_size, sizeof(int));
		DBJ_ASSERT(array_);
		free((void*)array_);
	}
	rezuldad = (float)(clock() - time_point_) / CLOCKS_PER_SEC;
	DBJ_PRINT(" ");
	DBJ_PRINT("system mem alloc/free: %.3f sec, %.0f dbj's",rezuldad, 1000 * rezuldad );
	slab_ = slab( 1000 * rezuldad );
	DBJ_PRINT(DBJ_BG_GREEN  "%s" DBJ_RESET,slab_.val );
	DBJ_PRINT(" ");
	free(slab_.val);
}

TUF_REG(compare_kv_mem_and_system_mem);

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
	dbj::tu::testing_system::execute();
	/// exit:
	km_destroy(k_memory_());
	return 0;
}