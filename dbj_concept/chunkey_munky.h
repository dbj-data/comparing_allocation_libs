// https://godbolt.org/z/kHDtLU
#include <stdint.h>
#include <malloc.h>



extern "C" {
 int printf( const char *, ...);
 void * memset( void *, int, size_t );
 char * strcpy( char *, const char *);
}

#define TU(fmt, x ) printf("\n%s => " fmt , #x, x )

// user defined struct to manipulate chunks on
// a level above blocks and chunks
struct user_chunk {
    bool in_use{}; 
    user_chunk *  next{}; 
    char data[0]{};
};

inline void print( user_chunk * xp)
{
    printf("\n[%p] in_use: %s, next %p, data: %s", xp, (xp->in_use ? "true": "false"), xp->next, xp->data );
}
/// --------------------------------------------------------------
int main(int, char **) 
{
    // user wants data size of 8, each
    constexpr auto chunk_size  =  sizeof(user_chunk) + 8 ;
    constexpr auto chunk_count =  4 ;
    constexpr auto block_size  =  chunk_size * chunk_count  ;

    // chunk size must be "alligned"
    // that means divisible with the size of int *
    // without a leftover
    static_assert( 0 == (chunk_size % sizeof( intptr_t )));

    // this is enough to fully describe a block
    char * chunks[chunk_count]{0};

    char * const block = (char*)calloc( chunk_count, chunk_size ) ;
    char * walker = block ;

    for ( int j = 0; j < chunk_count; ++j)
    {
        chunks[j] = walker ;
        walker = (char*)(walker + chunk_size) ;
    }

    // new block 
    // wiring into the free list formation
    TU("%p", block);
    int chunk_index = 0 ;
    for( auto chunk : chunks ) {
           user_chunk * xp = (user_chunk*)chunk ;
           // wraps the index back to 0
           // whend index + 1 equals count
           user_chunk *xp_next = (user_chunk*)chunks[(chunk_index + 1) % chunk_count];

           // payload does not have to be set here
          char buf[2]{ char(chunk_index + '0'), '\0'};
           strcpy(xp->data , buf );
           xp->data[chunk_size] = '\0';
           xp->next = xp_next;
           chunk_index++ ;
    }
    // the last chunk next pointer must be null
    // that is the signal there are no more 
    // chunks when allocating them intialy
    // meaining -- one by one from first
    user_chunk * xp = (user_chunk*)(chunks[chunk_count-1]) ;
    xp->next = nullptr ;

    printf("\nTotal chunks %d", chunk_index );

       for( auto chunk : chunks ) {
           user_chunk * xp = (user_chunk*)chunk ;
           print(xp);
       }

    free(block);

    return 0;
}