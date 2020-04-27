
# DBJ Block Chunk<!-- omit in toc -->
## Memory Pool Allocator Implementation<!-- omit in toc -->

&copy; 2020 APR by dbj@dbj.org CC BY SA 4.0

- [Recap: Why Memory Pool Allocator](#recap-why-memory-pool-allocator)
  - [Roadmap: Array allocation / deallocation](#roadmap-array-allocation--deallocation)
  - [Sanity](#sanity)
- [The Core Concepts](#the-core-concepts)
  - [Bottom Level View](#bottom-level-view)
  - [One Level Up](#one-level-up)
- [Size Alignment](#size-alignment)
  - [What is it?](#what-is-it)
  - [Why](#why)
  - [How to Align](#how-to-align)
- [Caveat Emptor](#caveat-emptor)

## Recap: Why Memory Pool Allocator

Memory Pool Allocator is feasible to be used in situations when memory is allocated and deallocated in equally sized pieces. 
These pieces are called "memory chunks" or simply "chunks".

Obvious use case is object pool allocator. All instances of a same class are of the same size. 
One pool allocator for one class is object pool allocator.

Synopsis of a class with pool allocator

```cpp
class Class {
    static Allocator & pool () {
          static Allocator pool_( sizeof Class ) ;
          return pool_ ;
    }
public:
    static void * new ( size_t ) { 
    // allocate() has no argument
    // chunk size == sizeof Class
      return Class::pool().allocate();  
      }
    static void delete ( void * p ) { 
      // giving it back to the pool
      Class::pool().deallocate(p); 
      }
} ;
```
Users of `Class` are unconcerned  with the existence of the pool allocator.
```cpp
// all using the pool as above
Class *a = new Class ;
Class *b = new Class ;
Class *c = new Class ;
// giving back to the pool
delete a;
delete b;
delete c;
```
The more `Class` objects are made/destroyed on the heap, the more benefits in improved performance will become visible.

### Roadmap: Array allocation / deallocation

```cpp
    static void * new [] ( size_t arr_size_in_bytes ) { 
    // allocate() has no argument
    // chunk size == sizeof Class
      return Class::pool().array_allocate(arr_size_in_bytes);  
      }
    static void delete [] ( void * p ) { 
      // giving it back to the pool
      Class::pool().array_deallocate(p); 
      }

```

### Sanity

Compile time constants defining the capability of the Pool Allocator will have to be provided for users.

## The Core Concepts

The secret sauce

Memory pool allocator is fast because it allocates number of blocks of memory in one call to the system allocator.
It is as simple as that. N calls to OS heap delivery functions are all replaced with one call.

Complexity inevitably raises from the requirement to deallocate/allocate chunks independently.

Pool allocator is also fast because deallocation is actually just marking the deallocated chunk as free. So they can be quickly recycled on next allocation.

### Bottom Level View

Memory pool is list of blocks. There is `1..N` blocks.

`block` is dynamically allocated slab of chars.
`block` is logically divided into number of equally sized chunks

Pool allocator configuration constants are
 - `max_pool_byte_size`
 - `max_blocks`
 - `max_chunk_size`
 - `max_chunk_count`

For the block structure point of view, diagram always helps.

```
char * block[k]   ( k = 0 ... max_blocks per pool )
|
+------------------+------------------+-------------------+---------------------+
|                  |                  |                   |                     |
|  chunk_size      |   chunk_size     |   chunk_size      |   chunk_size        |
|                  |                  |                   |                     |
+------------------+------------------+-------------------+---------------------+
|                  |                  |                   |                     |
chunk[j]    chunk[j+1]          chunk[.]            chunk[.]   chunk[ chunk_count - 1 ]   
( j = 0 .. chunk_count per block )

```
Block char slab size to be allocated is `chunk_count * chunk_size`. In the code:

```cpp
  // we use calloc: it delivers the memory zeroed
  char * const block = (char*)calloc( chunk_count , chunk_size ) ;
```
On the pool level we keep all the blocks allocated so we can free them all at once, or handle them by some other logic.

### One Level Up
 We need to be able to view memory pool blocks as list of user chunks. User chunk is normal structure.
`user_chunk` is part of implementation, not for pool users to use or manipulate.  

We do not maintain separate list of user chunks. We use the same block (char slab) to keep user chunks in memory. The same char slab allocated when a block was created.
We need to be able to implement quick and simple allocate/deallocate method, operating on individual chunks.

When creating a block we need to readjust the chunk_size by adding the user_chunk size to it.

Fully prepared new block, before it can act as the part of the pool is logically this:
```
Level One View aka "Chunk User View"

user_chunk * uchunk   uchunk + 1     uchunk + 2        uchunk + ...    uchunk + (chunk_count - 1)           
|                     |                     |                     |                     |     
+---------------------+---------------------+---------------------+---------------------+
| user_chunk size     | user_chunk size     | user_chunk size     | user_chunk size     |
| + chunk_size        | + chunk_size        | + chunk_size        | + chunk_size        |
|                     |                     |                     |                     |
+---------------------+---------------------+---------------------+---------------------+
|                     |                     |                     |                     |
chunk[0]              chunk[1]        chunk[.]             chunk[.]    chunk[chunk_count - 1]   
|
char * block

Level Zero View aka "Block View"
```
Thus actual block is now allocated as
```cpp
  char * const block = (char*)calloc( chunk_count , user_chunk_size + chunk_size ) ;
```
For that "co habitation" to work in C/C++ , we need to place the user chunks in the same `char` slab that represents the allocated block. We need data part of the `user_chunk` structure implemented like the last field in the struct, and its type as char array of size zero. Example:
```cpp
struct user_chunk final {
       // example of other fields
       bool in_use{};
       user_chunk * next_free{};
       // number and type of the fields
       // above is arbitrary
       // data must be the last field
       char data[0]{};
};
```
To create just the above struct dynamically (at runtime) with required data size one does this:
```cpp
    // allocate user chunk with runtime given payload of size 1024
    user_chunk * uchunk = (user_chunk *)malloc(  
            sizeof(bool) 
          + sizeof(user_chunk*) 
          + (sizeof (char) * 1024)  ) ;

    // this is exactly the same struct as if making it on the stack      
    // at compile time
  struct user_chunk_1024 final {
       bool in_use{};
       user_chunk * next_free{};
       char data[1024]{};
  };  
  user_chunk_1024 uchunk_1024 ;
```
That `malloc()` also makes sure the structure allocated is all in one memory block.
That is the size of each block chunk, if requested memory chunk size is 1024. Hence that statement:
```cpp
  char * const block = (char*)calloc( chunk_count , user_chunk_size + chunk_size ) ;
```
Where example `user_chunk_size` is calculated by adding sizes of its fields:
```cpp
// 1024 is requested chunk size
    user_chunk_size = sizeof(bool) + sizeof(user_chunk*) + (sizeof(char) * 1024);
```
Of course that size has to be aligned.
## Size Alignment

### What is it? 
The whole of the memory can be visualized as floor of square tiles. Each tile size is 
```cpp
   size_t tile_size = sizeof( intptr_t ) ;
```
When we create our blocks they have to cover the tiles. Only full tile visibility is allowed. And, covered tiles have to be fully covered.

Important detail: `sizeof` always returns aligned sizes. 

### Why

Speed 

We need to make sure chunk sizes and allocated block sizes are always aligned. The primary reason is the speed. Aligned memory is always faster to write to and read from. Sometimes very noticeably faster. You do not want your memory pool to slow down the code using it.

Size

When used, is system memory allocation doing the required size re-alignment? It does not. But it does (hidden) padding. 

So you are given newly allocated memory of requested size, but with padding that makes the actual whole block size properly aligned. And that padding is the memory waste. Example:

```cpp
/// 9 it is going to be
/// but system has allocated and is keeping
/// 16, because (16 % sizeof intptr_t) == 0
/// thus 7 is wasted
void * memp = malloc(9);
```
The actual situation is this:
```
user view -- 9 is allocated
memp         memp + 9
|                   |
+-------------------+------------------------+
| 0 1 2 3 4 5 6 7 8 | 9 10 11 12 13 14 15 16 |
+-------------------+------------------------+
|                                            |
begin                               begin + 16
system view -- 16 is allocated
```
Waste is 7 bytes. That `malloc()` is repeated N times, the waste is N * 7. That quickly adds up to a lot of wasted space.

### How to Align
User requested size, and any other size in use, when
divided with the size of `intptr_t` must result in an integer with no leftover.

Always make sure block size covers the tiles occupied properly
```cpp
// loop until size is not aligned
while ( 0 != (tile_size % (user_chunk_size + chunk_size)))
  {
    // adjust the *increased* chunk_size
    // so it covers fully the tiles occupied.
    chunk_size = align( chunk_size )
  }
```
And one fast and short `align` function is this
```cpp
/*
Machine word size. Depending on the architecture,
can be 4 or 8 bytes.
http://dmitrysoshnikov.com/compilers/writing-a-memory-allocator/#memory-alignment 
*/
	using word_t = intptr_t;

	constexpr size_t align(word_t n) {
		return (n + sizeof(word_t) - 1) & ~(sizeof(word_t) - 1);
	}
```
That is a bit manipulation. For a quick start one can jump [head first in here](https://www.geeksforgeeks.org/bitwise-operators-in-c-cpp/?ref=lbp).
## Caveat Emptor
To fully understand these concepts one has to study the code in this repository, too.



