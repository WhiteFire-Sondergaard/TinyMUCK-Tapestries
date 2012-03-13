/*
  balloc(), bfree(), and balloc_fixed()
  Block Memory Management Routines.
  (see header file for docs)
 */


#ifndef HAVE_MALLOC_H
#  include <stdlib.h>
#else
#  include <malloc.h>
#endif

#include "balloc.h"


/* ------------------------------------------------------
   memblock_datum class
   Contains the following methods:
    void *memblock_datum_ptr(memblk *ptr, int item)
    int memblock_datum_alloc(memblk *ptr)
    void memblock_datum_free(memblk *ptr, int item)
   ------------------------------------------------------ */
 

#define memblock_datum_ptr(ptr,item) (&ptr->data[ptr->size * (item)])


int
memblock_datum_alloc(memblk *ptr)
{
    unsigned char *p;
    int i, j;
    if (ptr->count >= ptr->maxitems)
        return -1;
    i = ptr->nextfree;
    p = memblock_datum_ptr(ptr, i);
    ptr->nextfree = (p[0] << 8) | p[1];
    ptr->count++;
    for (j = 0; j < ptr->size; j++)
	p[j] = 0;
    return (i);
}

void
memblock_datum_free(memblk *ptr, int item)
{
    unsigned char *p;
    p = memblock_datum_ptr(ptr, item);
    p[0] = (ptr->nextfree >> 8) & 0xff;
    p[1] = ptr->nextfree & 0xff;
    ptr->nextfree = item;
    ptr->count--;
}



/* ------------------------------------------------------
   memblock class
   Contains the following methods:
    memblk *memblock_new(int size)
    void memblock_free(memblk *ptr)
    void memblock_clear(memblk *ptr)
   ------------------------------------------------------ */
 
void
memblock_clear(memblk *ptr)
{
    int i;
    unsigned char *p;
    for (i = 0; i < ptr->maxitems;) {
        p = memblock_datum_ptr(ptr, i++);
        p[0] = i >> 8;
        p[1] = i & 0xff;
        p[2] = 0;
        p[3] = 0;
    }
    ptr->count = 0;
    ptr->nextfree = 0;
    ptr->next = 0;
    ptr->prev = 0;
    ptr->hnext = 0;
    ptr->hprev = 0;
}


static memblk *free_block_list = 0;

memblk *
memblock_new(int items, int size)
{
    memblk *ptr, *ptr2;
    if (size < 0) return 0;
    if (items == 0) {
        items = MEMBLOCK_PREFERRED_SIZE / size;
    }
    if (items < 1) items = 1;
    if (items > 32767) items = 32765;
    for (ptr2 = ptr = free_block_list; ptr; ptr = ptr->next) {
        if ((ptr->size * ptr->maxitems) == (size * items)) {
            break;
        }
        ptr2 = ptr;
    }
    if (!ptr) {
        ptr = (memblk *) malloc(sizeof(memblk) + (items*size));
        if (!ptr) return 0;
    } else {
        if (ptr == free_block_list) {
            free_block_list = ptr->next;
        } else {
            ptr2->next = ptr->next;
        }
    }
    ptr->maxitems = items;
    ptr->size = size;
    memblock_clear(ptr);
    return ptr;
}

void
memblock_free(memblk *ptr)
{
    ptr->next = free_block_list;
    free_block_list = ptr;
}

void
memblock_purge(void)
{
    memblk *ptr;
    while (free_block_list) {
        ptr = free_block_list->next;
        free(free_block_list);
        free_block_list = ptr;
    }
}

/* ------------------------------------------------
   memhash class
   ------------------------------------------------ */
   
struct mempointer {
    memblk *ptr;
    short item;
};
#define memptr struct mempointer
   
#define memhash_signif(ptr) (((int)ptr) >> 16)
#define memhash_mod(i) (i % MEMHASH_SIZE)
#define memhash_value(ptr) (memhash_mod(memhash_signif(ptr)))
memblk *memhash_table[MEMHASH_SIZE];

void
memhash_add(memblk *ptr)
{
    int i;
    i = memhash_value(ptr);
    ptr->hnext = memhash_table[i];
    memhash_table[i] = ptr;
    if (ptr->hnext)
        ptr->hnext->hprev = &(ptr->hnext);
    ptr->hprev = &(memhash_table[i]);
}

void
memhash_kill(memblk *ptr)
{
    *(ptr->hprev) = ptr->hnext;
    if (ptr->hnext)
        ptr->hnext->hprev = ptr->hprev;
    ptr->hprev = 0;
    ptr->hnext = 0;
}

memptr
memhash_find(void *ptr)
{
    memptr mp;
    memblk *p = 0;
    int i = memhash_signif(ptr);
    int limit = MEMHASH_SIZE;
    unsigned long siz;
    unsigned char *where;

    while (!p) {
        p = memhash_table[memhash_mod(i)];
        while (p) {
	    where = (unsigned char *)p;
	    siz = sizeof(memblk) + (p->size * p->maxitems) - 1;
            if ((unsigned char *)ptr >= where &&
		    (unsigned char *)ptr < (where + siz))
                break;
            p = p->hnext;
        }
        if (i-- <= 0 || limit-- <= 0) break;
    }
    mp.ptr = p;
    if (p) {
        mp.item = ((unsigned char *)ptr - p->data) / p->size;
    } else {
        mp.item = 0;
    }
    return mp;
}



/* ------------------------------------------------------
   balloc class
   Contains the following methods:
        void *balloc_fixed(memblk **ptr, int items, int size);
        int bfree(void *);
        void *balloc(int size);
   ------------------------------------------------------ */
 
void *
balloc_fixed(memblk **ptr, int items, int size)
{
    int i;
    memblk *p;
    
    if (!*ptr) {
        *ptr = memblock_new(items, size);
        memhash_add(*ptr);
        (*ptr)->prev = ptr;
    }
    p = *ptr;
    while (p->next && (p->count == p->maxitems || p->size != size))
        p = p->next;
    if (p->count < p->maxitems) {
        i = memblock_datum_alloc(p);
        return (memblock_datum_ptr(p, i));
    }
    p = *ptr;
    *ptr = memblock_new(items, size);
    memhash_add(*ptr);
    (*ptr)->next = p;
    (*ptr)->prev = ptr;
    p->prev = &((*ptr)->next);
    i = memblock_datum_alloc(*ptr);
    return (memblock_datum_ptr((*ptr), i));
}

static int purge_countdown = FREE_BLOCK_POOL;

void
bfree_memptr(memptr mp)
{
    if (!mp.ptr) return;
    memblock_datum_free(mp.ptr, mp.item);
    if (mp.ptr->count == 0) {
        *(mp.ptr->prev) = mp.ptr->next;
        if (mp.ptr->next) {
            mp.ptr->next->prev = mp.ptr->prev;
        }
        memhash_kill(mp.ptr);
        memblock_free(mp.ptr);
        if (purge_countdown-- <= 0) {
            memblock_purge();
            purge_countdown = FREE_BLOCK_POOL;
        }
    }
}


int
bfree(void *ptr)
{
    memptr mp;

    mp = memhash_find(ptr);
    if (!mp.ptr) return 0;
    bfree_memptr(mp);
    return 1;
}

static memblk *varmem_table[((MEMBLOCK_PREFERRED_SIZE/2)/MEMBLOCK_GRAIN)+1];

void *
balloc(int size)
{
    int slot;
    if (size < 0) return 0;
    slot = size / MEMBLOCK_GRAIN;
    if (size % MEMBLOCK_GRAIN) slot++;
    if (slot) {
        size = slot * MEMBLOCK_GRAIN;
    } else {
        size = MEMBLOCK_GRAIN;
        slot = 1;
    }
    if (slot > ((MEMBLOCK_PREFERRED_SIZE/2)/MEMBLOCK_GRAIN)) slot = 0;
    return balloc_fixed(&varmem_table[slot], 0, size);
}

void *
reballoc(void *ptr, int newsize)
{
    memptr mp;
    unsigned char *p, *q;
    int i;

    mp = memhash_find(ptr);
    if (!mp.ptr) return 0;
    if (mp.ptr->size > newsize) {
	return ptr;
    }
    p = balloc(newsize);
    q = ptr;
    for (i = 0; i < mp.ptr->size; i++)
	p[i] = q[i];
    bfree_memptr(mp);
    return p;
}


void *
cballoc(int nelems, int elsize)
{
    return balloc(nelems * elsize);
}


