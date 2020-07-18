#pragma once

#include "dbj_shoshnikov_pool_allocator.h"
#include "pool_allocator_instrumentation.h"

#define TEST_SHOSHNIKOV_POOL_ALLOCATOR_CLASS
#ifdef TEST_SHOSHNIKOV_POOL_ALLOCATOR_CLASS

// 4101 -- unreferenced local variable
#pragma warning( push )
#pragma warning( disable : 4101 )


namespace dbj_sampling {

	using namespace dbj::shohnikov;

	//using ::dbj::shoshnikov::dbj_pool_allocator;
	//using ::dbj::shoshnikov::pool_alloc_instrument;
	//using ::dbj::shoshnikov::legal_block_size;


	//  chunk[0], chunk[1], chunk[2]
	constexpr static legal_block_size alokator_block_size
	{ legal_block_size::_4 };

	/**
	 * The `Object` uses custom allocator,
	 * size is two `uint64_t`, 16 bytes.
	 */
	class Object final {

		// Object data
		uint64_t data_[0xFF]{ 42 };

		/// DBJ added
		/// users are unaware of the pool allocator used
		static dbj_pool_allocator& allocator()
		{
			// custom allocator for this structure:
			// note: sizeof Object is alligned size internaly
			// note: probably not necessary using standard C++ 
			static dbj_pool_allocator alokator(alokator_block_size, sizeof(Object));
			return alokator;
		}
	public :
		Object() { data_[0xFF - 1] = 0; }

		static void report()
		{
			pool_alloc_instrument::report( Object::allocator());
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

#if 0
		/// array variants
		static void* operator new [] (size_t size) {

			size_t count = size / sizeof(Object);

			Object** arr_ = (Object**)calloc( count, sizeof(Object) );

			for (int j = 0; j < count; ++j)
			{
				arr_[j] = (Object*)allocator().allocate();
			}

			return arr_;
		}

		static void operator delete [] (void* ptr, size_t size) {

			Object** arr_ = (Object**)ptr;

			size_t obj_size_ = sizeof(Object);
			size_t arr_arr_size_ = sizeof(arr_);
			size_t arr_size_ = sizeof(*(arr_[0]));

			size_t count = sizeof(arr_) / size ;

			for (int j = 0; j < count; ++j)
			{
				return allocator().deallocate( arr_[j]);
			}
		}
#endif // 0
	}; // Object

	/// ---------------------------------------------------------------------

	inline void simple_test () {

		// make sure more than one block will be made
		constexpr int arraySize = size_t(alokator_block_size) + 1;

		// array of pointers to Object
		Object* objects[arraySize]{ 0 };

		// Two `uint64_t`, 16 bytes.
		DBJ_PRINT("Untouched pool");
		Object::report();

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

		DBJ_PRINT("New object is where the next_free was");
		Object::report();

		delete objects[0];
		DBJ_PRINT("Deleted last object. We should have two free blocks.");
		Object::report();
		}

    // TUF_REG( simple_test );

	/// -------------------------------------------------
inline void array_aloc_test () {
	DBJ_PRINT("Array allocation: new [] and delete [] ");
	// make sure more than one block will be made
		constexpr int arraySize = size_t(alokator_block_size) + 1;

		/// Test array of objects allocation
		Object* oarr = new Object[arraySize];
		DBJ_PRINT("Allocated array Object[%d]", arraySize);
		Object::report();

		delete[] oarr;
		DBJ_PRINT("Deleted array Object[%d]", arraySize);
		Object::report();

		}

		// TUF_REG(array_aloc_test);
	/// -------------------------------------------------

	TU_REGISTER_NOT([] {
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

	/// -------------------------------------------------
	TU_REGISTER_NOT ([] {

		// allocator block size is 3 for Object
		Object* a, * b, * c, * d;
		a = new Object;		b = new Object;
		c = new Object;		d = new Object;
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
	/// -------------------------------------------------

} // namespace dbj_sampling 

#pragma warning( push )
#endif // TEST_SHOSHNIKOV_POOL_ALLOCATOR_CLASS
