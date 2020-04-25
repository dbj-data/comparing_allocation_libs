#ifndef DBJ_SHOSHNIKOV_POOL_ALLOCATOR_INC
#define DBJ_SHOSHNIKOV_POOL_ALLOCATOR_INC

#define POOL_ALLOC_INSTRUMENTATION 1

// DBJ added
// NOTE: NDEBUG is standard !
#if !defined( _DEBUG ) &&  !defined( DEBUG ) && !defined(NDEBUG) 
#define NDEBUG
#endif // !_DEBUG and !DEBUG and NDEBUG

namespace dbj::nanolib {

	// DBJ added
	enum class legal_block_size : size_t {
		_4 = 4, _8 = 8, _16 = 16, _32 = 32, _64 = 64, _128 = 128
	};

	// DBJ added sanity constants
	constexpr static auto max_number_of_blocks{ 0xFF };

   /**
	* Machine word size. Depending on the architecture,
	* can be 4 or 8 bytes.
      http://dmitrysoshnikov.com/compilers/writing-a-memory-allocator/#memory-alignment 
	*/
	using word_t = intptr_t;

	constexpr size_t align(word_t n) {
		return (n + sizeof(word_t) - 1) & ~(sizeof(word_t) - 1);
	}

	struct unaligned_size { size_t val{}; };

	namespace chunky {

		/// ------------------------------------------------------------------------
		/// pool has to own it, called from pools destructor
		/// why this? std::vector is overkill
		/// NOTE: in here there is no knowledge of Chunk
		class block_registry final {
			constexpr static int max_blocks{ 0xFF };
			char* blocks_[max_blocks]{ 0 };
			int level_{ 0 };
		public:

			int next_block_index() const noexcept { return level_; };

			/// return index of the block appended
			int append(char* const new_block_)
			{
				blocks_[level_] = new_block_;
				level_ += 1;
				return level_ - 1;
			}

			template<typename CB_ >
			void for_each(CB_ call_back_) const noexcept
			{
				for (int j = 0; j < max_blocks; ++j) {
					if (blocks_[j] == nullptr) break;
					call_back_(blocks_[j]);
				}
			}

			/// this is where memory is actually freed
			/// this is called when  block_registry host goes out of scope
			/// and that host is pool allocator
			~block_registry() {
				for (int j = 0; j < max_blocks; ++j) {
					if (blocks_[j] == nullptr) break;
					// free(blocks_[j]);
					::HeapFree(::GetProcessHeap(), 0, (void*)blocks_[j]);
#ifndef NDEBUG
					blocks_[j] = nullptr;
#endif
				}
				level_ = 0;
			}

			explicit block_registry() noexcept = default;
			block_registry(block_registry const&) = delete;
			block_registry& operator = (block_registry const&) = delete;
			block_registry(block_registry&&) = delete;
			block_registry& operator = (block_registry&&) = delete;
		};

		struct Chunk {
			/**
			 * When a chunk is free, the `next` contains the
			 * address of the next chunk in a list.
			 *
			 DBJ added next is ALSO null when a chunk is last c hunk in the last block in the pool
			 */
			Chunk* next{};
			/*
			 * DBJ added
			 */
			bool in_use{};
			/*
			 * When it's allocated, this space is used by
			 * the user.
			 */
			word_t data[1]{};
		};

		/**
		* Returns total allocation size, reserving in addition the space for
		* the Chunk structure (object header + first data word).
		*
		* Since the `word_t data[1]` already allocates one word inside the Block
		* structure, we decrease it from the size request: if a user allocates
		* only one word, it's fully in the Block struct.
		*/
		constexpr size_t chunk_allocation_size(unaligned_size size)
		{
			return align(size.val) + sizeof(Chunk) - sizeof(std::declval<Chunk>().data);
		}

		/// DBJ added
		/// Also: https://godbolt.org/z/cCIgA_
		constexpr static const size_t chunk_header_size_ = align(sizeof(Chunk*) + sizeof(bool) + sizeof(word_t[1]));
		constexpr static const size_t chunk_struct_size_ = align(sizeof(Chunk));

		static_assert(sizeof(Chunk) == chunk_struct_size_);

		Chunk* chunk_from_data(char* data) {
			return (Chunk*)((char*)data + sizeof(std::declval<Chunk>().data) -
				sizeof(Chunk));
		}

		inline size_t compute_raw_chunk_size(size_t chunk_size_arg)
		{
			// aligned size of what user has requested
			chunk_size_arg = align(chunk_size_arg);
			// 
			size_t size_ = chunk_header_size_ + chunk_size_arg;
			// should be equal
			_ASSERTE(align(size_) == size_);
			return size_;
		}


		/// ------------------------------------------------------------------------
		inline Chunk* allocateBlock(
			legal_block_size number_of_chunks_arg_,
			unaligned_size chunk_size_arg_,
			block_registry& registry_
		) {
			_ASSERTE(chunk_size_arg_.val > 0);

			const size_t number_of_chunks_ = size_t(number_of_chunks_arg_);
			const size_t chunk_allocation_size_ = chunk_allocation_size(chunk_size_arg_);
#ifndef NDEBUG
			const size_t block_size_ = number_of_chunks_ * chunk_allocation_size_;
			_ASSERTE(block_size_ > 0);
#endif

			/// block is slab of chars
			/// DBJ reminder: constant pointer is a pointer that can only point to single object throughout the program.
			/// Syntax to declare constant pointer
			/// <pointer - type>* const <pointer - name> = <memory - address>;
			/// Note! This is NOT a pointer to a const.
			///
			char* const start_address =
				//(char*)calloc(number_of_chunks_, chunk_allocation_size_);
				(char*)::HeapAlloc(::GetProcessHeap(), 0, number_of_chunks_ * chunk_allocation_size_);

			_ASSERTE(start_address);

			registry_.append(start_address);

			/// DBJ: also a const pointer...
			Chunk* const retval_ = reinterpret_cast<Chunk*>(start_address);

			char* chunk_walker = start_address;
			Chunk* chunk{};

			for (size_t i = 0; i < number_of_chunks_; ++i)
			{
				chunk = reinterpret_cast<Chunk*>(chunk_walker);
				chunk->in_use = false; // DBJ added
				chunk->next = reinterpret_cast<Chunk*>(chunk_walker + chunk_allocation_size_);
				chunk_walker = chunk_walker + chunk_allocation_size_;
			}
			// ! the last chunk next field
			chunk->next = nullptr;
			return  retval_;
		}

	} // chunky

	/**
	 * Based on
	 * http://dmitrysoshnikov.com/compilers/writing-a-pool-allocator/
	 */
	struct dbj_pool_allocator final
	{
#ifdef POOL_ALLOC_INSTRUMENTATION
		friend struct pool_alloc_instrument;
#endif // POOL_ALLOC_INSTRUMENTATION

		chunky::block_registry block_registry_{};

		using Chunk = chunky::Chunk;

		explicit dbj_pool_allocator(legal_block_size chunksPerBlock, size_t chunk_size_arg)
			noexcept
			: chunks_per_block_(chunksPerBlock)
			, chunk_size_(
				unaligned_size{
					(align(chunk_size_arg) < chunky::chunk_struct_size_
					  ? chunky::chunk_struct_size_
					  : chunk_size_arg)
				}
			)
			, next_free_chunk_(nullptr)

		{
		}

		/// DBJ added
		~dbj_pool_allocator() = default;

		/// DBJ removed default ctor
		dbj_pool_allocator() = delete;
		/// DBJ made no copy no move
		dbj_pool_allocator(dbj_pool_allocator const&) = delete;
		dbj_pool_allocator& operator = (dbj_pool_allocator const&) = delete;
		dbj_pool_allocator(dbj_pool_allocator&&) = delete;
		dbj_pool_allocator& operator = (dbj_pool_allocator&&) = delete;

		/**
		 * DBJ: all block must contain chunks of the same size
		 *      chunk size is constructor argument
		 */
		void* allocate() {
			if (next_free_chunk_ == nullptr)
			{
				next_free_chunk_ = chunky::allocateBlock(
					this->chunks_per_block_, this->chunk_size_, this->block_registry_
				);
			}

			Chunk* allocated_chunk = next_free_chunk_;

			next_free_chunk_ = next_free_chunk_->next;

			// DBJ added
			// mark the chunk as "used"
			allocated_chunk->in_use = true;
			// take it out from a free list
			allocated_chunk->next = nullptr;

			return allocated_chunk->data;
		}

		void deallocate(void* chunk_data_)
		{
			Chunk* chunk = chunky::chunk_from_data((char*)chunk_data_);
			// DBJ added
			// used chunk must have been marked as such
			_ASSERTE(true == chunk->in_use);
			// and the next must have been set to null
			_ASSERTE(nullptr == chunk->next);
			// DBJ added
			chunk->in_use = false;
			chunk->next = next_free_chunk_;
			next_free_chunk_ = chunk;
		}

#if 0
		/// ------------------------------------------------------------
		/// DBJ added
		/// remove all blocks with all chunk's
		/// chunk->next NOT pointing to null
		/// this is potentially expensive operation
		/// with speculative usefulness
		/// users have a choice of not calling it
		void sweep_blocks() const noexcept {
			// 1. remove all free blocks
			// 2. do free list rewiring
		}
#endif // 0

	private:

		/**
		 DBJ made it const
		 */
		const legal_block_size chunks_per_block_{};
		/*
		DBJ added
		*/
		const unaligned_size chunk_size_{};
		Chunk* next_free_chunk_{ nullptr };
	}; // pool_allocators
	// -----------------------------------------------------------
} // dbj::nanolib

#endif // DBJ_SHOSHNIKOV_POOL_ALLOCATOR_INC