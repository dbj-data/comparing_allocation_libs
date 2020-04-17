#pragma once

#include "shoshnikov_pool_allocator.h"
#include "pool_allocator_instrumentation.h"

#define TEST_SHOSHNIKOV_POOL_ALLOCATOR_CLASS
#ifdef TEST_SHOSHNIKOV_POOL_ALLOCATOR_CLASS

// 4101 -- unreferenced local variable
#pragma warning( push )
#pragma warning( disable : 4101 )


namespace dbj_sampling {

	using dbj::nanolib::pool_allocator;
	using dbj::nanolib::pool_alloc_instrument;
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

		static void report( FILE * fp_ = stdout )
		{
			pool_alloc_instrument::report(fp_, Object::allocator());
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

	/// ---------------------------------------------------------------------

	TU_REGISTER([] {

		// make sure more than one block will be made
		constexpr int arraySize = Object::alokator_block_size + 1;

		// array of pointers to Object
		Object* objects[arraySize]{ 0 };

		// Two `uint64_t`, 16 bytes.
		DBJ_PRINT("size(Object) = %d", sizeof(Object));

		for (int i = 0; i < arraySize; ++i) {
			objects[i] = new Object();
		}

		DBJ_PRINT("Allocated %d objects", arraySize);
		Object::report();

		for (int i = 0; i < arraySize; ++i) {
			delete objects[i];
		}

		DBJ_PRINT("Deallocated all %d objects", arraySize);
		Object::report();

		DBJ_PRINT("Creating next new object ");
		objects[0] = new Object();

		DBJ_PRINT("New object reuses previous block:");
		Object::report();

		delete objects[0];
		DBJ_PRINT("Deleted last object. We should have two free blocks.");
		Object::report();

		return; // need to sort out the array allocation

		/// -------------------------------------------------
		/// Test array of objects allocation
		Object* oarr = new Object[arraySize];
		DBJ_PRINT("Allocated array Object[%d]", arraySize);
		Object::report();

		delete[] oarr;
		DBJ_PRINT("Deleted array Object[%d]", arraySize);
		Object::report();

		});

	TU_REGISTER([] {
		// allocator block size is 3 for Object
		Object* a{};

		// chunk[0] should be free
		// chunk[0] should be next free mem ptr
		DBJ_PRINT("Untouched mem pool alloc");
		Object::report();

		// chunk[0] should be taken
		// chunk[1] should be next free mem ptr
		a = new Object;
		DBJ_PRINT("Allocated one object");
		Object::report();

		// chunk[0] should be free
		// chunk[0] should be next free mem ptr
		delete a;
		DBJ_PRINT("De allocated that one boject");
		Object::report();
		});

		TU_REGISTER([] {

			// allocator block size is 3 for Object
			Object* a, * b, * c, * d;
		a = new Object;
		b = new Object;
		c = new Object;
		d = new Object;
		DBJ_PRINT("Allocated four objects: A, B, C, D");
		Object::report();

		delete b;
		DBJ_PRINT("De allocated B");
		Object::report();

		delete d;
		DBJ_PRINT("De allocated D");
		Object::report();

		delete a;
		DBJ_PRINT("De allocated A");
		Object::report();

		delete c;
		DBJ_PRINT("De allocated C");
		Object::report();
		});

} // namespace dbj_sampling 

#pragma warning( push )
#endif // TEST_SHOSHNIKOV_POOL_ALLOCATOR_CLASS