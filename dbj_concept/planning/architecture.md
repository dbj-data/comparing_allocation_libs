# DBJ Block Chunk<!-- omit in toc -->
## Memory Pool Allocator<!-- omit in toc -->

&copy; 2020 APR by dbj@dbj.org CC BY SA 4.0

- [Key Concepts](#key-concepts)
  - [The Road Map](#the-road-map)
    - [Overloading new[] and delete[]](#overloading-new-and-delete)
- [dbj_pools](#dbjpools)
- [Why Blocks](#why-blocks)
- [User Code Point of View](#user-code-point-of-view)
- [The Core Algorithm](#the-core-algorithm)
  - [Allocate](#allocate)
  - [Deallocate](#deallocate)
- [Untouched memory pool](#untouched-memory-pool)
- [Appendix](#appendix)
  - [The concept of ID](#the-concept-of-id)

## Key Concepts

This is memory pool allocation, software engine architecture. End user code does not use this. This is used by various pool allocators facing the end user code. And example would be "object pool allocator".

This is not general purpose heap allocation mechanism. System heap allocation is to be used for that. This engine has the advantage when (and if) memory is always allocated in same sized chunks.

The typical scenario.
```
                      heap memory chunks                    
+--------+              +------------+                   +------------+
|        | 1         N  | fixed size | <-- allocate ---- |            |
| user A +------------> | 512 bytes  |                   | pool 1     |
|        |  uses        |            | --- deallocate -->|            |
+--------+              +------------+                   +------------+

+--------+              +------------+                   +------------+
|        | 1         M  | fixed size | <-- allocate ---- |            |
| user B +------------> | 128 bytes  |                   | pool 2     |
|        |  uses        |            | --- deallocate -->|            |
+--------+              +------------+                   +------------+
```
That is example of two users (A and B) using memory pools to deliver memory blocks. One user one size, one pool. One per each user.

Note: "User" in this instance means code that is using the memory pools.

### The Road Map

Currently we repeat the whole mechanism per each thread. It is on the road-map to make pools "thread aware".
```
                      Pools in presence of multiple threads
  +-------------------------------- THREAD 1 ----------------------------------+
  |    +--------+              +------------+                   +------------+ |
  |    |        | 1         N  | fixed size | <-- allocate ---- |            | |
  |    | user A +------------> | 512 bytes  |                   | pool 1     | |
  |    |        |  uses        |            | --- deallocate -->|            | |
  |    +--------+              +------------+                   +------------+ |
  +----------------------------------------------------------------------------+
  
  +-------------------------------- THREAD 2 ----------------------------------+
  |    +--------+              +------------+                   +------------+ |
  |    |        | 1         M  | fixed size | <-- allocate ---- |            | |
  |    | user B +------------> | 128 bytes  |                   | pool 2     | |
  |    |        |  uses        |            | --- deallocate -->|            | |
  |    +--------+              +------------+                   +------------+ |
  +----------------------------------------------------------------------------+
 
```
Note: one thread can contain number of pools. The feasibility of such addition to this engine, might be questionable. But it certainly can be done for customers wishing to do so.

In case of using shared libraries to carry pool allocators that might be mandatory.

#### Overloading new[] and delete[]

By C++ rules, class bound operators new[] and delete []  have this footprint
```cpp
  class ClassName {
     static inline some_allocator( number_of_chunks_per_block, sizeof(ClassName)) ;
  public:

       . . .
		static void* operator new [] (size_t size) {
          /*
          no can do:
          return some_allocator.allocate();

            size argument is byte size for the whole array to be allocated

            We can compute the number of elements and allocate them 
            one by one here and return pointer to the array of them
            But that will be unacceptably slow

            Thus it is much more feasible just to use malloc here.
            But that is not pool allocation  
          */
         return malloc( size ) ;
		}

		static void operator delete [] (void* ptr) {
         /*
         in here it is not possible to compute the number of elements
         in the array to be deallocated.

         it is best to leave it to the system free()
         */
        free();
    }
        . . .
  } ; // eof ClassName

```
For the above problem it is possible to devise some solution based on
[placement new](https://stackoverflow.com/questions/222557/what-uses-are-there-for-placement-new).
But again that would make it inevitably (and noticeably) slower than calling `calloc` and `free`.

Thus one can not feasibly provide fixed size pool allocator for array new and delete class overloads.

So. Currently I am not even sure we should do this.

## dbj_pools

All the polls are kept together in one "place": `dbj_pools`.
`dbj_pools` manages number of `pool` structures.

These are interface diagrams.
```
+-----------------------+                 +---------------------+
|                       |1      max_pools |                     |
| dbj_pools             +---------------->| pool  [id]          |
|                       |                 |                     |
| total_pools_byte_size |                 |  pool_byte_size     |
| max_pools             |                 |  num_blocks         |
| max_pool_byte_size    |                 |  new_block          |
| max_blocks            |                 |  get_block          |
| max_chunk_count       |                 |  foreach_block      |
| foreach_pool          |                 |                     |
+-----------------------+                 +---------------------+
```
Each pool manages number of blocks of the same size.  
```
+---------------+                 +----------------+
|               |1      max_blocks|                |
| pool [id]     +---------------->| block [id]     |
|               |                 |                |
|               |                 | chunk_count    |
|               |                 | chunk_size     |
|               |                 | foreach_chunk  |
+---------------+                 +----------------+
```
Each block is logically divided on chunks of the same size. Block of memory is implemented as array of chars. Called "slab". 

Chunks on the block are an abstraction. Function `foreach_chunk` operates on these abstract chunks. Abstract chunk is represented with a pointer to the start of chunk, on the slab allocated.
```
block (aka "slab") of 4 chunks
|
+------------------+------------------+-------------------+------------------+
|                  |                  |                   |                  |
|  chunk_size      |   chunk_size     |   chunk_size      |   chunk_size     |
|                  |                  |                   |                  |
+------------------+------------------+-------------------+------------------+
|                  |                  |                   |                     
p0                p1                 p2                  p3 
```

## Why Blocks

Using blocks we can minimize the number of expensive system calls to obtain actual heap memory. Obviously that might degenerate into one block per pool scenario which might be ultimately wasteful. The issue is it is impossible to predict the memory consumption in advance. 

The size of each block, has to be fine tuned per application. It is inevitably try -- measure -- try -- process.

## User Code Point of View

The user code sees the pool as one contiguous list of free and allocated chunks. User code does not know about blocks.

Code using this engine (aka the User) , manages "User Chunks"; separate concrete struct's. User chunk is a structure whose fields are required to implement pool allocation functionality.
```
 // an arbitrary user chunk example
 struct user_chunk {
       user_chunk * next ;
       char * data[];
 };
```

## The Core Algorithm

When pool starts its life, the list of free user_chunk's is ready. So called "free list". Hence the `next` field on the specimen above.

### Allocate
When `allocate` request arrives, the first free `user_chunk` is taken of the "free list" and its `data`  is returned as the pointer to the start of newly allocated memory (of predefined size).

`allocate` results in new block allocation, only if free list has no members.

### Deallocate
When `deallocate` request arrives, the user chunk, to which arrived data pointer belongs, is placed back on the free list. Ready to be re-used on one of future allocation calls. 

`deallocate` never result if freeing the block.

It is obvious the whole pool operation results in potentially very small number of block allocated. Which means very small number of expensive heap memory allocation system calls.

## Untouched memory pool 

This is the user code view upon first time pool creation. Pool created has 4 chunks per block. `chunk_size` is 512 bytes.
```
Memory pool free list has 4 "user chunks"

next_free_chunk                          
|                           
+----------------+         +--------------+         +--------------+         +-------------+
|                | next    |              | next    |              |next     |             |
|  user_chunk    +-------->| user_chunk   +-------->| user_chunk   +-------->|user_chunk   +---> NULL
|                |         |              |         |              |         |             |
+---+------------+         +---+----------+         +--+-----------+         +--+----------+
    |                            |                       |                        |
    +-> data[512]                +-> data[512]           +-> data[512]            +-> data[512]
```

For performance reasons this is just a logical view. There is no separate single linked list of user chunks. This view is kept on the same slab that is allocated for a block. Of course with an adjusted size.

For implementation details please proceed to [implementation.md](implementation.md)

## Appendix

### The concept of ID

Each key entity in the engine has an ID. ID's are unique on the entity level, not on the engine level.
ID's are dynamically created upon creation.

Basically (in here) ID's are just indexes in LILO (Last In Last Out) stacks of 2 key entities.
```cpp
typedef int dbj_pools_id_type ;
```
`dbj_pools`  is managing an array of pools
```cpp
/* stack of pools pointers */
lilo_stack_pools pools = { max = max_pools, level = 0, data = {0} }
```
Each pool is managing stack of blocks
```cpp
/* stack of blocks pointers */
lilo_stack_blocks blocks = { max = max_blocks, level = 0, data = {0} }
```
Quick [example here](https://godbolt.org/z/NUJ3p3).