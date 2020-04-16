#pragma once

#ifdef COMPARE_KVEC_AND_STD_VEC

/// https://godbolt.org/z/fAR6Hd
/// https://github.com/attractivechaos/benchmarks

#include "klib.h"

///-----------------------------------------------
/// NOTE: due to clang deffect using lambdas
/// like bellow will work only in MSVC
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
