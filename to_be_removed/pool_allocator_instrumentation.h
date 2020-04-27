#pragma once

// #include "../dbj--nanolib/dbj++tu.h"

#include "../dbj--nanolib/vt100win10.h"
#include "dbj_shoshnikov_pool_allocator.h"

#pragma warning( push )
#pragma warning( disable : 4477 4313 )

#define dbj_reporter_file_pointer_ stdout
#undef reporter
#define reporter(FMT, ...) fprintf( dbj_reporter_file_pointer_, FMT, __VA_ARGS__)

// this is not a 100% true pointer view but it is simpler
// for humans to compare
#define DBJFMT "%4X"

namespace dbj::nanolib {
	struct pool_alloc_instrument final
	{
		using Chunk = dbj_pool_allocator::Chunk;

		/// chunk can be in 3.5 states
		/// - free
		/// - taken aka "in use"
		/// - first free to be used aka "next free"; the pool sentinel that is
		/// 
		static void print_address( Chunk * chunk, dbj_pool_allocator const& pool_ )
		{
			if (chunk == nullptr)
			{
				reporter(DBJ_FG_RED_BOLD "| NULL " DBJ_RESET);
				return;
			}

			Chunk* free_sentinel_ = pool_.next_free_chunk_ ;

			if (chunk == free_sentinel_) {
				// first free to be used in entire pool
				reporter(DBJ_FG_YELLOW_BOLD "| "  DBJFMT " " DBJ_RESET, chunk);
				if ( chunk-> in_use  ) 	reporter(DBJ_FG_RED_BOLD " !in_use == true! " DBJ_RESET);
			}
			else if ((chunk->next) != nullptr) {
				// part of a free list
				reporter(DBJ_FG_GREEN "| "  DBJFMT " " DBJ_RESET, chunk);
				if (chunk->in_use == true )	reporter(DBJ_FG_RED_BOLD " !in_use ==true! " DBJ_RESET);
			}
			else {
					// in use
					reporter(DBJ_FG_RED "| " DBJFMT " " DBJ_RESET, chunk);
					if (chunk->in_use == false)	reporter(DBJ_FG_RED_BOLD " !in_use == false! " DBJ_RESET);
			}
		}

		static void report
		(dbj_pool_allocator const & pool_, bool header = false ) noexcept
		{
			if (pool_.block_registry_.next_block_index() == 0)
			{
				reporter("\n\n" DBJ_FG_BLUE_BOLD "Pool unused." DBJ_RESET);
				return;
			}

			if (header)
				reporter("\n" DBJ_FG_BLUE_BOLD "%s Chunk size: %zu, chunks per block: %zu" DBJ_RESET "\n",
					"Instrumentation report", pool_.chunk_size_, pool_.chunks_per_block_
				);
			else
				reporter("\n");

			auto block_counter = 0U;
			size_t chunk_allocation_size_ = chunky::chunk_allocation_size(pool_.chunk_size_);

			auto for_each_chunk = [&] ( char * block_slab_  ) -> void {

				reporter("\n" DBJ_FG_BLUE_BOLD "Block %3d: " DBJ_RESET, ++block_counter);

					/// chunk * is the allocated data pointer
					char* chunk_address = (char*)(block_slab_);

					for (size_t i = 0; i < size_t(pool_.chunks_per_block_); ++i) {
						print_address((Chunk*)chunk_address /*dbj_pool_allocator::chunk_from_data(chunk_address)*/, pool_);
						chunk_address = (char*)(chunk_address + chunk_allocation_size_);
					}
			};

			pool_.block_registry_.for_each(for_each_chunk);

			/// footer
			reporter("\n\n" DBJ_FG_BLUE_BOLD "Next free chunk: " DBJ_RESET);
			print_address( pool_.next_free_chunk_ , pool_);
		}
		/// ---------------------------------------------------------------------


	}; // pool_alloc_instrument
} // namespace dbj::nanolib

#undef reporter
#undef dbj_reporter_file_pointer_ 
#undef DBJFMT

#pragma warning( pop )