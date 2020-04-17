#pragma once
#ifndef SHOSHNIKOV_POOL_ALLOCATOR_INC
#define SHOSHNIKOV_POOL_ALLOCATOR_INC

#define POOL_ALLOC_INSTRUMENTATION 1

// DBJ added
// basically we keep only vector of pointers to blocks
#include <vector>

// DBJ added
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

		using word_t = intptr_t;
		/**
 * Aligns the size by the machine word.
 http://dmitrysoshnikov.com/compilers/writing-a-memory-allocator/#memory-alignment
 */
		constexpr inline size_t align(size_t n) {
			return (n + sizeof(word_t) - 1) & ~(sizeof(word_t) - 1);
		}


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

		/// DBJ added
		std::vector<Chunk*> instrumentation_block_sequence_{};

		/// DBJ added
		~pool_allocator() {
			for (Chunk* block_ : this->instrumentation_block_sequence_)
			{
				free(block_);
				block_ = nullptr;
			}
			this->instrumentation_block_sequence_.clear();
		}

		explicit pool_allocator(size_t chunksPerBlock, size_t chunk_size_arg)
			noexcept
			: chunks_per_block_(chunksPerBlock)
			, chunk_size_(align(chunk_size_arg))
		{
			_ASSERTE(chunk_size_ > sizeof(Chunk));

			// DBJ added
			// Allocating empty block at construction time
			next_free_chunk_ = allocateBlock
			(this->chunk_size_, this->chunks_per_block_);

			// new block is made, save a pointer to it
			Chunk* new_block_ptr = next_free_chunk_;
			instrumentation_block_sequence_.push_back(new_block_ptr);
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
			if (next_free_chunk_ == nullptr) 
			{
				next_free_chunk_ = allocateBlock
				(this->chunk_size_, this->chunks_per_block_ );

				// new block is made, save a pointer to it
				Chunk* new_block_ptr = next_free_chunk_;
				instrumentation_block_sequence_.push_back(new_block_ptr);
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
		void deallocate(void* chunk) 
		{
			// The freed chunk's next pointer points to the
			// current allocation pointer:
			reinterpret_cast<Chunk*>(chunk)->next = next_free_chunk_;
			// And the allocation pointer is moved backwards, and
			// is set to the returned (now free) chunk:
			next_free_chunk_ = reinterpret_cast<Chunk*>(chunk);
		}

	private:
		const size_t block_size() const noexcept {
			return chunks_per_block_;
		}

		const size_t chunk_size() const noexcept {
			return chunk_size_;
		}

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


} // dbj::nanolib

#endif // SHOSHNIKOV_POOL_ALLOCATOR_INC