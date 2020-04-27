#pragma once

#include "../common.h"
#include "../dbj--nanolib/dbj_heap_alloc.h"
#include "../dbj_pool_allocator/dbj_shoshnikov_pool_allocator.h"

namespace feasibility {

	struct Data {
		constexpr static const int size{0xFF};
		char payload[size];

		void populate() {
			DBJ_REPEAT(size) {
				payload[dbj_repeat_counter_] = char(dbj::randomizer( 64 + 25, 64)) ;
			}
		}
	};

	using dbj::shohnikov::dbj_pool_allocator;
	using dbj::shohnikov::legal_block_size;

#define test_data_size 0xFFFF 
#define loop_each 0xF

	/// this is very favourable manual setup
	/// tuned so that two blocks only are allocated per session
	constexpr static legal_block_size alokator_block_size{ legal_block_size::_16384 };

	/**
	 * The `Pooled` uses custom allocator,
	 * size is two `uint64_t`, 16 bytes.
	 */
	template<typename ALLOCATOR_TYPE, typename DATA_TYPE>
	struct Pooled final {

		// Object data
		DATA_TYPE data;

		/// DBJ added
		/// users are unaware of the pool allocator used
		static ALLOCATOR_TYPE& allocator()
		{
			// this is fixed size allocator
			// it deliver heap chunks which are all of the 
			// same size == sizeof(Pooled)
			// internaly it heap allocates alokator_block_size
			// chunks in one go, keeping them together in block's
			static ALLOCATOR_TYPE alokator(alokator_block_size, sizeof(Pooled));
			return alokator;
		}

		static void* operator new(size_t size) {
			// there is no size argument 
			// this is fixed size allocator
			// size == sizeof(Pool)
			return allocator().allocate();
		}

		static void operator delete(void* ptr, size_t size) {
			/// that chunk will be recycled, not freed
			return allocator().deallocate(ptr);
		}

		/// fixed size pool allocator can not do this
		/// size provided  to new [] is the total size of the array
		static void* operator new [](size_t size) {
			// there is no size argument 
			// this is fixed size allocator
			// this is unusable --> return allocator().allocate();
			return malloc(size);
		}

			static void operator delete [](void* ptr) {
			 // can not use this --> return allocator().deallocate(ptr);
			free(ptr);
		}

	}; // Pooled

/// -------------------------------------------------------------

	struct NOTPooled final {

		Data data;

		/// by using overloaded new and delete
		/// we can choose to use WIN32 memory allocation
		/// faster than malloc/free
		static void* operator new(size_t size) {
			return ::HeapAlloc(GetProcessHeap(), 0, size);
		}

		static void operator delete(void* ptr) {
			::HeapFree(GetProcessHeap(), 0, (void*)ptr);
		}

		/// fixed size pool allocator can not do this
		/// size provided  to new [] is the total size of the array
		static void* operator new [] (size_t size) {
			return ::HeapAlloc(GetProcessHeap(), 0, size);
		}

		static void operator delete [] (void* ptr) {
			::HeapFree(GetProcessHeap(), 0, (void*)ptr);
		}

	}; // NOTPooled


	/// ----------------------------------------------------------------------------------
	template< typename OBJTYPE>
	inline auto meta_driver( dbj::collector & collector_ ) {

		dbj::driver(collector_,
			[&] {
				OBJTYPE* test_data[test_data_size]{ 0 };
				// make them one by one
				// use  them one by one
				DBJ_REPEAT(test_data_size) {
					test_data[dbj_repeat_counter_] = new OBJTYPE;
					// use it to avoid optimizing out 
					test_data[dbj_repeat_counter_]->data.populate();
				}
				// remove them one by one
				DBJ_REPEAT(test_data_size) {
					delete test_data[dbj_repeat_counter_];
				}
			}
		);
	}
	
	/// ----------------------------------------------------------------------------------
	/// if I need to make OBJTYPE * obj_ptr_array[test_data_size]
	/// why would I do them one by one in a loop? 
	/// why not all at once
	/// OBJTYPE* test_data = new OBJTYPE[test_data_size];
	template< typename OBJTYPE>
	inline auto array_meta_driver( dbj::collector & collector_ ) {

		dbj::driver(collector_,
			[&] {
				// make them 
				OBJTYPE* test_data = new OBJTYPE[test_data_size];
				// use them 
				DBJ_REPEAT(test_data_size) {
					test_data[dbj_repeat_counter_].data.populate();
				}
				// remove them
				delete [] test_data;
			}
		);
	}
	/// ----------------------------------------------------------------------------------
	static inline void reporter (const char* name, float min, float max, int count) {
		DBJ_PRINT( DBJ_FG_RED_BOLD "%s " DBJ_RESET "has been tested %3d times, test data size was: %d", 
			name, count, test_data_size);
		DBJ_PRINT("Rezults are -- min time: %0.3f sec, max time: %0.3f sec, avg mean time: " DBJ_FG_RED_BOLD " %0.3f sec" DBJ_RESET,
			min, max, (min + max) / 2);
	}
	/// ----------------------------------------------------------------------------------
	static inline void compare_individual_pool_and_system() {

		const auto test_loop_size = 0xF;

		dbj::collector coll_pooled("Pooled");
		dbj::collector coll_not_pooled("NOT Pooled");

		DBJ_PRINT( DBJ_FG_BLUE_BOLD "Comparing indiviaul allocation using new/delete"  DBJ_RESET);
		DBJ_PRINT("Please wait, test loop count is: %d ", test_loop_size);
		DBJ_REPEAT(test_loop_size)
		{
			printf(" . ");
			meta_driver< Pooled<dbj::shohnikov::dbj_pool_allocator, Data> >(coll_pooled);
			meta_driver<NOTPooled>(coll_not_pooled);
		}

		DBJ_PRINT(" ");
		dbj::collector::report(	coll_pooled, reporter );
		DBJ_PRINT(" ");
		dbj::collector::report(coll_not_pooled, reporter );
		DBJ_PRINT(" ");
	}

	/// ----------------------------------------------------------------------------------
	static inline void compare_array_pool_and_system () {

		const auto test_loop_size = 0xF;

		dbj::collector coll_pooled("Pooled Array");
		dbj::collector coll_not_pooled("NOT Pooled Array");

		DBJ_PRINT( DBJ_FG_BLUE_BOLD "Comparing array allocation using new[]/delete[]"  DBJ_RESET);
		DBJ_PRINT( DBJ_FG_BLUE_BOLD "Fixed pool actually can not be used inside class new[]/delete[], so this comparison has no meaning..."  DBJ_RESET);

		DBJ_PRINT("Please wait, test loop count is: %d ", test_loop_size);
		DBJ_REPEAT(test_loop_size)
		{
			printf(" . ");
			array_meta_driver< Pooled<dbj::shohnikov::dbj_pool_allocator, Data> >(coll_pooled);
			array_meta_driver<NOTPooled>(coll_not_pooled);
		}

		DBJ_PRINT(" ");
		dbj::collector::report(	coll_pooled, reporter );
		DBJ_PRINT(" ");
		dbj::collector::report(coll_not_pooled, reporter );
		DBJ_PRINT(" ");
	}


	static inline void compare_pool_and_system() {

		DBJ_PRINT(DBJ_FG_BLUE_BOLD "Is it feasible to invest in developing object pool allocator?"  DBJ_RESET);

		compare_individual_pool_and_system();
		compare_array_pool_and_system();
	}

	TUF_REG(compare_pool_and_system);

#undef test_data_size
#undef loop_each

} // feasibility