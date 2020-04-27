#ifndef DBJ_BLOCKCHUNK_INC
#define DBJ_BLOCKCHUNK_INC
/*
(c) 2020 by dbj@dbj.org CC BY SA 4.0
*/
extern "C" {

	// wherever feasible strong type concept is deployed
	// STRong Type
	#undef strong_type
	#define strong_type(T,N) typedef struct N { T val; } N

#define DBJ_BLOCKCHUNK "dbj_blockchunk"

	static struct { int major; int minor; int patch; }
	dbj_blockchunk_version = { 0,0,0 };

	typedef enum {
		// total memory taken by all the pools
		dbj_blockchunk_max_num_of_pools = 0xFF,
		//  total pool size
		dbj_blockchunk_max_pool_byte_size = 10 * 0xFFFF,
		// this is theoretycal
		// in practice max pool byte size is an overarching constant
		dbj_blockchunk_max_blocks = 0xFFFF,
		// this defines max block size in bytes
		dbj_blockchunk_max_chunk_size,
		dbj_blockchunk_max_chunk_count
	} dbj_blockchunk_constants;

	strong_type( int, dbj_blockchunk_id);
	strong_type( int, dbj_blockchunk_chunk_size);
	strong_type( int, dbj_blockchunk_chunk_count);

	typedef struct dbj_blockchunk_block
	{
		dbj_blockchunk_id id;
		const int chunk_size;
		const int chunk_count;

		char* (*slab)();

	} dbj_blockchunk_block;

	// one pool is 1 .. N blocks of the same size 
	// with equaly sized chunks
	// there can be [ 1 .. max_num_of_pools ]


	typedef struct dbj_blockchunk_pool dbj_blockchunk_pool;

	typedef struct dbj_blockchunk_pool
	{
		dbj_blockchunk_id id;

		// current number of blocks
		int (*number_of_blocks_fp)();

		dbj_blockchunk_block* (*new_block)();

	} dbj_blockchunk_pool;

	dbj_blockchunk_pool* dbj_blockchunk_get_pool(dbj_blockchunk_id);
	// make a new pool
	dbj_blockchunk_pool* dbj_blockchunk_new_pool();

} /* "C" */
#endif // DBJ_BLOCKCHUNK_INC