#include <vector>
#include <array>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "klib.h"

/// https://godbolt.org/z/fAR6Hd

/// https://github.com/attractivechaos/benchmarks

#define DBJ_ASSERT _ASSERTE

int main(int, char**)
{
	constexpr int M = 10, N = 2000000; // 200K instead of 2 mil
	int i{}, j{};
	volatile clock_t t{};
	///-----------------------------------------------
	t = clock();
	for (i = 0; i < M; ++i) {
		int* array_ = DBJ_ALLOC(int, N); //  (int*)malloc(N * sizeof(int));
		DBJ_ASSERT(array_);
		for (j = 0; j < N; ++j) {
			DBJ_ASSERT(j < N);
			array_[j] = j;
		}
		DBJ_FREE(array_);
	}
	printf("C array_, preallocated: %.3f sec\n",
		(float)(clock() - t) / CLOCKS_PER_SEC);
	///-----------------------------------------------
	t = clock();
	for (i = 0; i < M; ++i) {
		int* tmp_, * array_ = 0, max = 0;
		for (j = 0; j < N; ++j) {
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
	printf("C array_, dynamic: %.3f sec\n",
		(float)(clock() - t) / CLOCKS_PER_SEC);
	///-----------------------------------------------
	t = clock();
	for (i = 0; i < M; ++i) {
		kvec_t(int) array_;
		kv_init(array_);
		kv_resize(int, array_, N);
		for (j = 0; j < N; ++j) kv_a(int, array_, j) = j;
		kv_destroy(array_);
	}
	printf("C vector, dynamic(kv_a): %.3f sec\n",
		(float)(clock() - t) / CLOCKS_PER_SEC);
	///-----------------------------------------------
	t = clock();
	for (i = 0; i < M; ++i) {
		kvec_t(int) array_;
		kv_init(array_);
		for (j = 0; j < N; ++j)
			kv_push(int, array_, j);
		kv_destroy(array_);
	}
	printf("C vector, dynamic(kv_push): %.3f sec\n",
		(float)(clock() - t) / CLOCKS_PER_SEC);
	///-----------------------------------------------
	t = clock();
	for (i = 0; i < M; ++i) {
		std::vector<int> array_(size_t(N), int(0));
		// std::array<int, N> array_;
		// array_.reserve(N);
		for (j = 0; j < N; /*++*/j++) array_[j] = j;
	}
	printf("C++ vector, preallocated: %.3f sec\n",
		(float)(clock() - t) / CLOCKS_PER_SEC);
	///-----------------------------------------------
	t = clock();
	for (i = 0; i < M; ++i) {
		std::vector<int> array_{};
		for (j = 0; j < N; ++j) array_.push_back(j);
	}
	printf("C++ vector, dynamic: %.3f sec\n",
		(float)(clock() - t) / CLOCKS_PER_SEC);
	///-----------------------------------------------
/// exit:
	km_destroy(k_memory_());
	return 0;
}