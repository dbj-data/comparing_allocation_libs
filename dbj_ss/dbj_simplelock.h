#pragma once
#ifndef DBJ_SIMPLE_LOCK_INC
#define DBJ_SIMPLE_LOCK_INC

#ifdef _MSC_VER

#include <windows.h>
#define inline __inline

#define atom_cas_long(ptr, oval, nval) (InterlockedCompareExchange((LONG volatile *)ptr, nval, oval) == oval)
#define atom_cas_pointer(ptr, oval, nval) (InterlockedCompareExchangePointer((PVOID volatile *)ptr, nval, oval) == oval)
#define atom_inc(ptr) InterlockedIncrement((LONG volatile *)ptr)
#define atom_dec(ptr) InterlockedDecrement((LONG volatile *)ptr)
#define atom_sync() MemoryBarrier()
#define atom_spinlock(ptr) while (InterlockedExchange((LONG volatile *)ptr , 1)) {}
#define atom_spinunlock(ptr) InterlockedExchange((LONG volatile *)ptr, 0)

#else

#define atom_cas_long(ptr, oval, nval) __sync_bool_compare_and_swap(ptr, oval, nval)
#define atom_cas_pointer(ptr, oval, nval) __sync_bool_compare_and_swap(ptr, oval, nval)
#define atom_inc(ptr) __sync_add_and_fetch(ptr, 1)
#define atom_dec(ptr) __sync_sub_and_fetch(ptr, 1)
#define atom_sync() __sync_synchronize()
#define atom_spinlock(ptr) while (__sync_lock_test_and_set(ptr,1)) {}
#define atom_spinunlock(ptr) __sync_lock_release(ptr)

#endif

/* spin lock */
#define spin_lock(Q) atom_spinlock(&(Q)->lock)
#define spin_unlock(Q) atom_spinunlock(&(Q)->lock)

/* read write lock */
typedef struct dbj_lock {
	int write ;
	int read  ;
}   dbj_lock  ;

static inline void dbj_lock_init(struct dbj_lock* lock) 
{
	lock->write = 0;
	lock->read = 0;
}

static inline void dbj_lock_rlock(struct dbj_lock* lock) 
{
	for (;;) {
		while (lock->write) {
			atom_sync();
		}
		atom_inc(&lock->read);
		if (lock->write) {
			atom_dec(&lock->read);
		}
		else {
			break;
		}
	}
}

static inline void dbj_lock_wlock(struct dbj_lock* lock) 
{
	atom_spinlock(&lock->write);
	while (lock->read) {
		atom_sync();
	}
}

static inline void dbj_lock_wunlock(struct dbj_lock* lock) 
{
	atom_spinunlock(&lock->write);
}

static inline void dbj_lock_runlock(struct dbj_lock* lock) 
{
	atom_dec(&lock->read);
}

#endif // DBJ_SIMPLE_LOCK_INC
