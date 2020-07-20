#pragma once
#ifndef DBJ_SL_STORAGE_INC
#define DBJ_SL_STORAGE_INC

#ifdef __clang__
#pragma clang system_header
#endif

/*
(c)2020 by dbj@dbj.org CC BY SA 4.0

usage:

asserts on overflow

size_t index = sl_storage_store("STRING ONE") ;

returns NULL or asserts in debug build if invalid index 

const char * string_ = sl_storage_get( index ) ;

so what is good about this?
index is unique, thus there is no cache search on get
there is no heap usage
*/

#include <assert.h>
#include "dbj_simplelock.h"

#ifdef __cpplang
extern "C" {
#endif

#define DBJ_STRING_STORAGE_MAX_LEVEL 0xFF 

    static dbj_lock* dbj_padlock_holder_for_ss() 
    {
        static  dbj_lock padlock_ ;
        static  dbj_lock * padlock_ptr = 0;

        if (padlock_ptr == NULL ) {
            padlock_ptr = &padlock_;
            dbj_lock_init(padlock_ptr);
        }

        return padlock_ptr;
    }

typedef struct sl_storage_struct 
{
    dbj_lock * padlock;
   const size_t end ;
   size_t level ;
   const char * storage[DBJ_STRING_STORAGE_MAX_LEVEL] ;
} sl_storage_struct ;

/* single instance */
static sl_storage_struct * dbj_sl_storage () {

static sl_storage_struct sl_storage_ = 
{
   .padlock = dbj_padlock_holder_for_ss(),
   .end = DBJ_STRING_STORAGE_MAX_LEVEL,
   .level = 0,
    {0}
};
        return & sl_storage_;
}


static inline size_t sl_storage_capacity()
{
    return DBJ_STRING_STORAGE_MAX_LEVEL - 1;
}

static inline size_t sl_storage_left(sl_storage_struct * sl_storage )
{
    assert(sl_storage);
    return sl_storage_capacity() - sl_storage->level ;
}

// asserts on overflow
static inline size_t sl_storage_store (sl_storage_struct* sl_storage , const char * new_sl )
{
    assert(sl_storage);
     assert( new_sl != 0 ) ;

     dbj_lock_rlock(sl_storage->padlock);

     sl_storage->storage[sl_storage->level] = new_sl ;
     sl_storage->level += 1;
     assert( sl_storage->level < sl_storage->end );

     dbj_lock_runlock(sl_storage->padlock);

     return sl_storage->level - 1 ;
}

// asserts on invalid index_
// returns 0 on invalid index_, in release builds
static inline const char * sl_storage_get (sl_storage_struct* sl_storage, const size_t index_ )
{
    assert(sl_storage);

    dbj_lock_wlock(sl_storage->padlock);

    char* rezult_ = 0;

     if ( index_ <= sl_storage->level ) {
         assert( sl_storage->storage[ index_ ] != 0 ) ;
         const char* rezult_ = sl_storage->storage[ index_ ] ;
         dbj_lock_wunlock(sl_storage->padlock);
         return rezult_;
     }
     assert(0); // signal not in storage in debug builds
     dbj_lock_wunlock(sl_storage->padlock);
     rezult_ = (char*)0;
     return rezult_;
}

#ifdef __cpplang
} // extern "C" 
#endif

#endif // DBJ_SL_STORAGE_INC

