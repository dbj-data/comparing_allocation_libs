#pragma once
#ifndef SHOSHNIKOV_POOL_ALLOCATOR_INC
#define SHOSHNIKOV_POOL_ALLOCATOR_INC

#include "dbj_memaligned.h"
#define POOL_ALLOC_INSTRUMENTATION 1

#ifdef  POOL_ALLOC_INSTRUMENTATION
// basically we keep only vector of pointers to blocks
#include <vector>
#endif  POOL_ALLOC_INSTRUMENTATION

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

		// DBJ added sanity constants
		constexpr static auto max_number_of_blocks{ 0xFF };

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

		/// DBJ added all of it
#ifdef  POOL_ALLOC_INSTRUMENTATION
		/// yes I know of better designs :)
		std::vector<Chunk*> instrumentation_block_sequence_{};

		~pool_allocator() {
			for (Chunk* block_ : this->instrumentation_block_sequence_)
			{
				free(block_);
				block_ = nullptr;
			}
			this->instrumentation_block_sequence_.clear();
		}

		void instrumentation_report(FILE* report_, bool show_chunks = true) const noexcept
		{
#undef reporter
#define reporter(FMT, ...) fprintf( report_, FMT, __VA_ARGS__)

			reporter("\n\n%s Chunk size: %d, chunks per block: %d\n",
				"Instrumentation report", this->chunk_size_, this->chunks_per_block_
			);

			auto block_counter = 0U;
			for (Chunk* block_ : this->instrumentation_block_sequence_)
			{
				reporter("\nBlock %3d: %4X ", ++block_counter, block_);

				if (show_chunks == false) continue;

				Chunk* chunk = block_;
				for (int i = 0; i < this->chunks_per_block_ - 1; ++i) {

					if ( chunk != this->next_free_chunk_ )
						reporter("| %4X ", chunk);
					else
						reporter("| " DBJ_FG_GREEN_BOLD "%4X " DBJ_RESET , chunk);

					chunk =
						reinterpret_cast<Chunk*>(reinterpret_cast<char*>(chunk) + this->chunk_size_);
				}
				reporter("\n");
			}
			reporter( "\nNext free chunk:" DBJ_FG_GREEN_BOLD " %4X\n" DBJ_RESET, this->next_free_chunk_);
#undef reporter
		}
		/// ---------------------------------------------------------------------
#endif //  POOL_ALLOC_INSTRUMENTATION


		explicit pool_allocator(size_t chunksPerBlock, size_t chunk_size_arg)
			noexcept
			: chunks_per_block_(chunksPerBlock)
			, chunk_size_(dbj::align(chunk_size_arg))
		{
			_ASSERTE(chunk_size_ > sizeof(Chunk));
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
		void* allocate() {


			// No chunks left in the current block, or no any block
			// exists yet. Allocate a new one, passing the chunk size:
			if (next_free_chunk_ == nullptr) {
				next_free_chunk_ = allocateBlock
				(this->chunk_size_, this->chunks_per_block_ );

#ifdef  POOL_ALLOC_INSTRUMENTATION
				// new block is made, save a pointer to it
				Chunk* new_block_ptr = next_free_chunk_;
				instrumentation_block_sequence_.push_back(new_block_ptr);
#endif  POOL_ALLOC_INSTRUMENTATION
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
		 DBJ made it const
		 */
		const size_t chunks_per_block_{};
		/*
		DBJ added
		*/
		const size_t chunk_size_{};

		/**
		 * Allocation pointer.
		 */
		Chunk* next_free_chunk_ = nullptr;

		/**
		 * Allocates a new block from OS.
		 *
		 * Returns a Chunk pointer set to the beginning of the block.

		 DBJ made it static
		 */

		static Chunk* allocateBlock(size_t chunk_size_arg_, size_t chunks_per_block_arg_) {

			// The first chunk of the new block.
			// Chunk* blockBegin = reinterpret_cast<Chunk*>(malloc(blockSize));
			// DBJ changed it as below
			Chunk* blockBegin = static_cast<Chunk*>(calloc(chunks_per_block_arg_, chunk_size_arg_));

			// DBJ added no mem check
#ifndef NDEBUG
			_ASSERTE(blockBegin);
#else // release
			if (nullptr == blockBegin) {
				perror("\n\n" __FILE__ "\n\ncalloc() failed");
				exit(errno);
			}
#endif // release 

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
#define TEST_SHOSHNIKOV_POOL_ALLOCATOR_CLASS
#ifdef TEST_SHOSHNIKOV_POOL_ALLOCATOR_CLASS

#include "dbj--nanolib/dbj++tu.h"
	/**
	 * The `Object` uses custom allocator,
	 * size is two `uint64_t`, 16 bytes.
	 */

	struct Object final {

		// Object data, 16 bytes:

		uint64_t data[2];

		//  chunk[0], chunk[1], chunk[2]
		constexpr static auto alokator_block_size{ 3 };

		/// DBJ added
		/// users are unaware of the pool allocator used
		static pool_allocator& allocator()
		{
			// custom allocator for this structure:
			// note: sizeof Object is alligned size internaly
			// note: probably not necessary using standard C++ 
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

		// DBJ added -- currently bottom line naive implementations
		// used to check the inner workings
		static void* operator new [](size_t count)
		{
			// allocate array of pointers using system allocator
			Object** array_ = (Object**)calloc(count, sizeof(Object*));

			for (auto j = 0; j < count; ++j)
				array_[j] = (Object*)allocator().allocate();

			return array_;
		}

			static void operator delete [](void* array_arg_, size_t count)
		{
			Object** array_ = (Object**)array_arg_;

			for (auto j = 0; j < count; ++j) {
				allocator().deallocate(array_[j]);
#ifndef NDEBUG
				array_[j] = nullptr;
#endif
			}

			free(array_);
			array_ = nullptr;
		}
	}; // Object

	inline void testing_allocator_class() {

		// make sure more than one block will be made
		constexpr int arraySize = Object::alokator_block_size + 1;

		// array of pointers to Object
		Object* objects[arraySize]{ 0 };
		
		// Two `uint64_t`, 16 bytes.
		DBJ_PRINT("size(Object) = %d", sizeof(Object));

		// Allocate 10 objects. This causes allocating two larger,
		// blocks since we store only 8 chunks per block:

		for (int i = 0; i < arraySize; ++i) {
			objects[i] = new Object();
		}

		DBJ_PRINT("Allocated %d objects", arraySize);
		Object::allocator().instrumentation_report(stdout);

		for (int i = 0; i < arraySize; ++i) {
			delete objects[i];
		}

		DBJ_PRINT("Deallocated all %d objects", arraySize);
		Object::allocator().instrumentation_report(stdout);

		DBJ_PRINT("Creating next new object ");
		objects[0] = new Object();

		DBJ_PRINT("New object reuses previous block:");
		Object::allocator().instrumentation_report(stdout);

		delete objects[0];
		DBJ_PRINT("Deleted last object. We should have two free blocks.");
		Object::allocator().instrumentation_report(stdout);

		return; // need to sort out the array allocation

		/// -------------------------------------------------
		/// Test array of objects allocation
		Object* oarr = new Object[arraySize];
		DBJ_PRINT("Allocated array Object[%d]", arraySize);
		Object::allocator().instrumentation_report( stdout );

		delete[] oarr;
		DBJ_PRINT("Deleted array Object[%d]", arraySize);
		Object::allocator().instrumentation_report(stdout);

	}

	TUF_REG(testing_allocator_class);

#endif // TEST_SHOSHNIKOV_POOL_ALLOCATOR_CLASS

} // dbj::nanolib

#endif // SHOSHNIKOV_POOL_ALLOCATOR_INC