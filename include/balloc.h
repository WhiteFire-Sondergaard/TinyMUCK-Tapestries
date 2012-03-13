/*
  balloc(), bfree(), and balloc_fixed()
 
  Block Memory Management Routines.
    balloc() and bfree() are snap-in replacements for malloc() and free().
    These routines are tuned to be efficient for applications that use
    a lot of similar sized small chunks of data.  This package is more
    memory efficient in this area, because it allocates blocks of similar
    sized data, and doles them out when they are requested.  The overhead
    of the management of this data is small, and only requires about 36
    bytes per block of data fields, including malloc() overhead.
        This package will be very inefficient with memory, if you use a lot
    of data that is larger in size than the BLOCKSIZE that you specify.
        With data that is small and similar sized (such as data structure
    nodes), this package can be a winner, since it cuts down on memory
    fragmentation and overhead a lot.
        With truly huge processes that are swapping in and out of virtual
    memory, this package might be bad, since this tends to put related,
    but different sized data in seperate areas of memory, making for more
    page-swapping, slowing down the process. However, if you just use this
    package on similar sized data, and use regular malloc()/free() on
    strings and variable sized data, this might still be a winner.  Also,
    this package might bring the size of the process down within the size
    of real RAM memory, and this might decrease the paging that would
    otherwise ensue.
        Since this  package uses malloc() to allocate the blocks of memory,
    these routines are completely compatible with malloc()/free() and may
    be used in the same program without problems.
 */

/*  Functions:
    void *balloc(int size)
        This will return a pointer to a space in memory that is at least
        size bytes long.
    
    void *balloc_fixed(memblk **ptr, int items, int size)
        This will return a pointer to a space in memory that is exactly
        size bytes long.  ptr is the address of a memblk pointer, that
        is used to keep track of what blocks have already been allocated.
        If a new block needs to be allocated to provide you with a space,
        then the block will try to be allocated with the given number of
        items, for later balloc_fixed() calls.  You can request different
        sized blocks of data, but for efficiency, it is suggested that you
        use different ptr's for each data size.
    
    int bfree(void *)
        This will mark a space in memory, that was previously allocated
        by balloc() or balloc_fixed(), to be unused, and available to be
        allocated again by the balloc() or balloc_fixed() function.
        This will return 1 if the data was successfully freed, or 0 if it
        did not recognize that memory space as having been allocated by
        balloc() or balloc_fixed().
 */
 

/* Following are some tuning parameters you can set. */
 
/* The GRAIN specifies the quantum sizes of blocks that balloc() will create.
 * ie: a grain of 16 will only allocate chunks sized in multiples of 16.
 * Grains larger than 24 will probably waste memory. Always set GRAIN in
 * multiples of the machine's word size (usually 4), or else you'll get a
 * lot of SIGBUS errors whenever you try to use a pointer that is located
 * in that balloc()ed memory space. Basically this means that your grain
 * choices are 4, 8, 12, 16, 20, or 24.  The default grain value is 8 bytes.
 */
#define MEMBLOCK_GRAIN 8

/* This is the amount of bytes preferred in a block.  Clusters of data will
 * be allocated together to reach this size.  balloc()ed data with a size
 * greater than half this value, will be significantly less efficiently
 * allocated than malloc() would allocate it.  Large values of this, however,
 * lead to a lot of allocated but unused space in memory.  The default
 * preferred size is 8192 bytes.
 */
#define MEMBLOCK_PREFERRED_SIZE 16384

/* Memory hash table size, used for bfree().  Large values of this make
 * programs that use HUGE amounts of memory do bfree()s a bit faster.
 * Small values of this will make bfree()s slow.  The default value is
 * probably just fine for most circumstances, with a value of 256 entries.
 */
#define MEMHASH_SIZE 1024

/* When a block is completely cleared of data, it is stuck on a stack of
 * blocks that are ready to me free()d.  If something comes along and needs
 * a new block, though, of a size that already exists, then one of these is
 * used, without having to malloc() it again.  This helps prevent thrashing
 * when something keeps balloc()ing and bfree()ing when all the other blocks
 * are already filled. Large values of this probably just waste memory.  The
 * default value is probably just fine for most circumstances, with a value
 * of 20 blocks.
 */
#define FREE_BLOCK_POOL 20



struct memblock {
    struct memblock *next;
    struct memblock **prev;
    struct memblock *hnext;
    struct memblock **hprev;
    short nextfree;
    short maxitems;
    short size;
    short count;
    unsigned char data[1];
};
#define memblk struct memblock


extern void *balloc_fixed(memblk **ptr, int items, int size);
extern void *balloc(int size);
extern void *reballoc(void *ptr, int newsize);
extern void *cballoc(int nelems, int elsize);
extern int bfree(void *ptr);

