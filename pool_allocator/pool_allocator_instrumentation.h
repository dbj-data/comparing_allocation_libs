#pragma once
#include "shoshnikov_pool_allocator.h"

#pragma warning( push )
#pragma warning( disable : 4477 4313 )

#undef reporter
#define reporter(FMT, ...) fprintf( report_, FMT, __VA_ARGS__)

// this is not a 100% true pointer view but it is simpler
// for humans to compare
#define DBJFMT "%4X"

namespace dbj::nanolib {
	struct pool_alloc_instrument final
	{
		using Chunk = pool_allocator::Chunk;

		static void print_address( FILE* report_, Chunk * chunk, Chunk * free_sentinel_ ) {
			if (chunk != free_sentinel_)
				reporter(DBJ_RESET "| " DBJFMT " ", chunk);
			else
				reporter(DBJ_FG_GREEN_BOLD "| "  DBJFMT " " DBJ_RESET, chunk);
		}

		static void report
		(FILE* report_, pool_allocator const & pool_, bool header = false ) noexcept
		{
			_ASSERTE( this->pool_ );

			if (header)
				reporter("\n" DBJ_FG_BLUE_BOLD "%s Chunk size: %zu, chunks per block: %zu" DBJ_RESET "\n",
					"Instrumentation report", pool_.chunk_size_, pool_.chunks_per_block_
				);
			else
				reporter("\n");

			auto block_counter = 0U;
			for (Chunk* block_ : pool_.instrumentation_block_sequence_)
			{
				reporter("\n" DBJ_FG_BLUE_BOLD "Block %3d: " DBJ_RESET , ++block_counter );

				Chunk* chunk = block_ ;
				Chunk* next_free = pool_.next_free_chunk_ ;

				for (int i = 0; i < pool_.chunks_per_block_ ; ++i) {

					print_address(report_, chunk, next_free);
					chunk =
						reinterpret_cast<Chunk*>(reinterpret_cast<char*>(chunk) + pool_.chunk_size_);
				}
			}

			if (nullptr == pool_.next_free_chunk_)
				reporter("\n\n" DBJ_FG_BLUE_BOLD "Next free chunk:" DBJ_FG_GREEN_BOLD " NULL\n" DBJ_RESET);
			else
				reporter("\n");
		}
		/// ---------------------------------------------------------------------


	}; // pool_alloc_instrument
} // namespace dbj::nanolib

#undef reporter
#pragma warning( pop )
#undef DBJFMT