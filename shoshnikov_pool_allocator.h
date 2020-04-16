#pragma once
#ifndef SHOSHNIKOV_POOL_ALLOCATOR_INC
#define SHOSHNIKOV_POOL_ALLOCATOR_INC

#include "dbj_memaligned.h"

#include <cstdlib> // aligned_alloc

// NOTE: NDEBUG is standard !
#if !defined( _DEBUG ) &&  !defined( DEBUG ) && !defined(NDEBUG) 
#define NDEBUG
#endif // !_DEBUG and !DEBUG and NDEBUG

namespace dbj::nanolib {
	/**
	 * Pool-allocator.
	 *
	 * Details: http://dmitrysoshnikov.com/compilers/writing-a-pool-allocator/
	 *
	 * Allocates a larger block using `malloc`.
	 *
	 * Splits the large block into smaller chunks
	 * of equal size.
	 *
	 * Uses bump-allocated per chunk.
	 *
	 * by Dmitry Soshnikov <dmitry.soshnikov@gmail.com>
	 * MIT Style License, 2019

	  * The allocator class.
	  *
	  * Features:
	  *
	  *   - Parametrized by number of chunks per block
	  *   - Keeps track of the allocation pointer
	  *   - Bump-allocates chunks
	  *   - Requests a new larger block when needed

	  http://dmitrysoshnikov.com/compilers/writing-a-pool-allocator/

	  *
	  */
	struct pool_allocator final {

	   /**
		* A chunk within a larger block.
		  DBJ moved inside pool_allocator
		*/
		struct Chunk {
			/**
			 * When a chunk is free, the `next` contains the
			 * address of the next chunk in a list.
			 *
			 * When it's allocated, this space is used by
			 * the user.
			 */
			Chunk* next;
		};

		explicit pool_allocator(size_t chunksPerBlock, size_t chunk_size_arg ) 
			noexcept
			: chunks_per_block_(chunksPerBlock)
			, chunk_size_(dbj::align(chunk_size_arg))
		{
			_ASSERTE(chunk_size_ > sizeof(Chunk) );
		}

		/// DBJ removed default ctor
		pool_allocator() = delete;
		/// DBJ made no copy no move
		pool_allocator(pool_allocator const&) = delete;
		pool_allocator(pool_allocator&&) = delete;

		/**
		 * Returns the first free chunk in the block.
		 *
		 * If there are no chunks left in the block,
		 * allocates a new block.
		 *
		 * DBJ: all block must contain chunks of the same size
		 *      chunk size is constructor argument 
		 */
		void* allocate( ) {


			// No chunks left in the current block, or no any block
			// exists yet. Allocate a new one, passing the chunk size:
			if (next_free_chunk_ == nullptr) {
				next_free_chunk_ = allocateBlock(this->chunk_size_, this->chunks_per_block_ );
			}

			// The return value is the current position of
			// the allocation pointer:

			Chunk* freeChunk = next_free_chunk_;

			// Advance (bump) the allocation pointer to the next chunk.
			//
			// When no chunks left, the `next_free_chunk_` will be set to `nullptr`, and
			// this will cause allocation of a new block on the next request:
			next_free_chunk_ = next_free_chunk_->next;

			return freeChunk;
		}

		/**
		 * Puts the chunk into the front of the chunks list.
		 */
		void deallocate(void* chunk) {

			// The freed chunk's next pointer points to the
			// current allocation pointer:

			reinterpret_cast<Chunk*>(chunk)->next = next_free_chunk_;

			// And the allocation pointer is moved backwards, and
			// is set to the returned (now free) chunk:

			next_free_chunk_ = reinterpret_cast<Chunk*>(chunk);
		}

		const size_t block_size() const noexcept {
			return chunks_per_block_;
		}

		const size_t chunk_size() const noexcept {
			return chunk_size_;
		}

	private:
		/**
		 * Number of chunks per larger block.
		 */
		size_t chunks_per_block_{};

		size_t chunk_size_{};

		/**
		 * Allocation pointer.
		 */
		Chunk* next_free_chunk_ = nullptr;

		/**
		 * Allocates a new block from OS.
		 *
		 * Returns a Chunk pointer set to the beginning of the block.
		 */

		static Chunk* allocateBlock(size_t chunk_size_arg_ , size_t chunks_per_block_arg_ ) {

			// The first chunk of the new block.
			// Chunk* blockBegin = reinterpret_cast<Chunk*>(malloc(blockSize));
			Chunk* blockBegin = static_cast<Chunk*>(calloc(chunks_per_block_arg_, chunk_size_arg_));

			_ASSERTE(blockBegin);
#ifdef NDEBUG
			if (nullptr == blockBegin) {
				perror("\n\n" __FILE__ "\n\ncalloc() failed");
				exit( errno );
			}
#endif

			// Once the block is allocated, we need to chain all
			// the chunks in this block:
			Chunk* chunk = blockBegin;

			for (int i = 0; i < chunks_per_block_arg_ - 1; ++i) {
				chunk->next =
					reinterpret_cast<Chunk*>(reinterpret_cast<char*>(chunk) + chunk_size_arg_);
				chunk = chunk->next;
			}

			chunk->next = nullptr;

			return blockBegin;
		}

	};

	// -----------------------------------------------------------
#ifdef TEST_SHOSHNIKOV_POOL_ALLOCATOR_CLASS

#include "dbj--nanolib/dbj++tu.h"
	/**
	 * The `Object` structure uses custom allocator,
	 * size is two `uint64_t`, 16 bytes.
	 */
	struct Object;
	struct Object final {

		// Object data, 16 bytes:

		uint64_t data[2];

		// where is this 8 coming from?
		constexpr static auto alokator_block_size{8};

		static pool_allocator & allocator()
		{
			// custom allocator for this structure:
			// note: sizeof Object is alligned size internaly
			static pool_allocator alokator(alokator_block_size, sizeof(Object));
			return alokator;
		}

		/*
		 * overloading `new`, and `delete` operators.
		 */
		static void* operator new(size_t size) {
			return allocator().allocate();
		}

		static void operator delete(void* ptr, size_t size) {
			return allocator().deallocate(ptr);
		}
	}; // Object

	// Instantiate our allocator, using 8 chunks per block:

	// pool_allocator Object::allocator{ 8 };

	inline void testing_allocator_class() {

		// Allocate 10 pointers to our `Object` instances:
		constexpr int arraySize = 10;
		Object* objects[arraySize]{ 0 };
		// Two `uint64_t`, 16 bytes.
		DBJ_PRINT("size(Object) = %d", sizeof(Object));

		// Allocate 10 objects. This causes allocating two larger,
		// blocks since we store only 8 chunks per block:

		DBJ_PRINT("About to allocate %d objects", arraySize);

		for (int i = 0; i < arraySize; ++i) {
			objects[i] = new Object();
			DBJ_PRINT("new [%d] = %p ", i, &(objects[i]));
		}

		DBJ_PRINT(" ");
		DBJ_PRINT("Deallocate all the objects:");

		for (int i = arraySize - 1; i >= 0; --i) {
			DBJ_PRINT("delete [%d] = %p ", i, &(objects[i]));
			delete objects[i];
		}

		DBJ_PRINT(" ");
		DBJ_PRINT("New object reuses previous block:");

		objects[0] = new Object();
		DBJ_PRINT("new [%d] = %p ", 0, &(objects[0]));
	}

	TUF_REG(testing_allocator_class);

#endif // TEST_SHOSHNIKOV_POOL_ALLOCATOR_CLASS

} // dbj::nanolib

#endif // SHOSHNIKOV_POOL_ALLOCATOR_INC