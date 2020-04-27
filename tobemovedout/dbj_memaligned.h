#pragma once
#ifndef DBJ_MEMALIGNED_INC
#define DBJ_MEMALIGNED_INC

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#ifdef _MSC_VER
#include <iso646.h>
#endif

namespace dbj {

#define mistery _HEAP_MAXREQ
	/// ----------------------------------------------------------------------
	/**
	* Machine word size. Depending on the architecture,
	* can be 4 or 8 bytes.
	*/
	using word_t = intptr_t;
	/// ----------------------------------------------------------------------
	/**
	 * Aligns the size by the machine word.
	 http://dmitrysoshnikov.com/compilers/writing-a-memory-allocator/#memory-alignment
	 */
	constexpr inline size_t align(size_t n) {
		return (n + sizeof(word_t) - 1) & ~(sizeof(word_t) - 1);
	}

	/// ----------------------------------------------------------------------
	inline bool is_aligned(void *ptr, size_t alignment) {
		if (((unsigned long long)ptr % alignment) == 0)
			return true;
		return false;
	}

	/// ----------------------------------------------------------------------
	inline void* aligned_malloc(size_t alignment, size_t size)
	{
		void* p;
#ifdef _MSC_VER
		p = _aligned_malloc(size, alignment);
#elif defined(__MINGW32__) || defined(__MINGW64__)
		p = __mingw_aligned_malloc(size, alignment);
#else
		// somehow, if this is used before including "x86intrin.h", it creates an
		// implicit defined warning.
		if (posix_memalign(&p, alignment, size) != 0) {
			return nullptr;
		}
#endif
		return p;
	}

	/// ----------------------------------------------------------------------
	inline void aligned_free(void* mem_block) {
		if (mem_block == nullptr) {
			return;
		}
#ifdef _MSC_VER
		_aligned_free(mem_block);
#elif defined(__MINGW32__) || defined(__MINGW64__)
		__mingw_aligned_free(mem_block);
#else
		free(mem_block);
#endif
	}

} // dbj 

#endif // DBJ_MEMALIGNED_INC
