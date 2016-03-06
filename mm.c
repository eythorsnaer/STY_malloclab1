/*
 * mm.c -  explicit list .
 * 
 *  In our approach, a block is allocated by checking the heap for
 *  the first available free block in an explicit list. An
 *  allocated block consists of a header and a footer that store it's size,
 *  and the payload. A free block consists of a header and a footer that
 *  that store it's size, and it then uses part of it's payload to
 *  keep track of the next and previous free blocks.
 *  When a block is freed we check whether the block before it is also
 *  a free block, if it is we coalesce, and the same goes for the block after it.
 *  We then set the new block's next pointer to
 *  the value of the next block in the list and it's prev pointer to the location of the previous
 *  block in the list.
 *  The value of the next block's previous pointer is changed to the location of the new block and
 *  the value of the previous block's prev pointer is set to the location our new block as well.
 *  Realloc simply frees the block and then allocates it the next available block of the
 *  appropriate size: I.e. it calls mm_malloc and then it calls mm_free.
 *
 *  An example of an allocated block:
 * 
 * |BLK size | payload | BLK size|
 *
 *  An example of a free block:
 *
 * |BLK size | pointer to next | pointer to prev | payload | BLK size|
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in below _AND_ in the
 * struct that follows.
 *
 * Note: This comment is parsed. Please do not change the
 *       Format!
 *
 * === User information ===
 * Group: SWEDENBOYZ
 * User 1: bjartur14
 * SSN: 1012952499
 * User 2: emilg12
 * SSN: 1910903069
 * User 3: eythorst14
 * SSN: 0109943229
 * === End User Information ===
 ********************************************************/
team_t team = {
  /* Group name */
  "SWEDENBOYZ",
  /* First member's full name */
  "Bjartur Hafþórsson",
  /* First member's email address */
  "bjartur14@ru.is",
  /* Second member's full name (leave blank if none) */
  "Emil Gunnar Gunnarsson",
  /* Second member's email address (leave blank if none) */
  "emilg12@ru.is",
  /* Third full name (leave blank if none) */
  "Eyþór Snær Tryggvason",
  /* Third member's email address (leave blank if none) */
    "eythorst14@ru.is"
};

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)

#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

/* $begin mallocmacros */
/* Basic constants and macros */
#define WSIZE       4       /* word size (bytes) */  
#define DSIZE       8       /* doubleword size (bytes) */
#define CHUNKSIZE  (1<<12)  /* initial heap size (bytes) */
#define OVERHEAD    24       /* overhead of header, footer, next and prev pointers(bytes) */

#define MAX(x, y) ((x) > (y)? (x) : (y))  

/* Pack a size and allocated bit into a word */
#define PACK(size, alloc)  ((size) | (alloc))

/* Read and write a word at address p */
#define GET(p)         (*(size_t *)(p))
#define PUT(p, val)    (*(size_t *)(p) = (val))  

/* (which is about 49/100).* Read the size and allocated fields from address p */
#define GET_SIZE(p)    (GET(p) & ~0x7)
#define GET_ALLOC(p)   (GET(p) & 0x1)

/* Given block ptr bp, compute address of its header and footer */
#define HDRP(bp)       ((char *)(bp) - WSIZE)  
#define FTRP(bp)       ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

/* Given block ptr bp, compute address of next and previous blocks */
#define NEXT_BLKP(bp)  ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))
#define PREV_BLKP(bp)  ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))
/* $end mallocmacros */

#define NEXT_FREEP(bp)(*(void **)(bp + DSIZE))
#define PREV_FREEP(bp)(*(void **)(bp))

/* Global variables */
static char *heap_listp;  /* pointer to first block */  
static char *free_listp = 0;/* Pointer to the first free block */

/* function prototypes for internal helper routines */
static void *extend_heap(size_t words);
static void place(void *bp, size_t asize);
static void *find_fit(size_t asize);
static void *coalesce(void *bp);
static void printblock(void *bp); 
static void checkblock(void *bp);
static void removeblock(void *bp);
static void insertblock(void *bp);

/* 
 * mm_init - Initialize the memory manager 
 */
/* $begin mminit */
int mm_init(void) 
{
    /* create the initial empty heap */
    if ((heap_listp = mem_sbrk(4*WSIZE)) == NULL) {
	return -1;
    }
    PUT(heap_listp, 0);                        /* alignment padding */

    PUT(heap_listp+WSIZE, PACK(OVERHEAD, 1));  /* prologue header */
    PUT(heap_listp + DSIZE, 0); //prev free block pointer
    PUT(heap_listp + DSIZE+WSIZE, 0); //next free block pointer

    PUT(heap_listp+DSIZE, PACK(OVERHEAD, 1));  /* prologue footer */

    PUT(heap_listp+WSIZE+DSIZE, PACK(0, 1));   /* epilogue header */

    heap_listp += DSIZE;

    /* Extend the empty heap with a free block of CHUNKSIZE bytes */
    if (extend_heap(CHUNKSIZE/WSIZE) == NULL) {
	return -1;
    }
    return 0;
 }
/* $end mminit */

/* 
 * mm_malloc - Allocate a block with at least size bytes of payload 
 */
/* $begin mmmalloc */
void *mm_malloc(size_t size) 
{
  size_t asize;      /* adjusted block size */
  size_t extendsize; /* amount to extend heap if no fit */
  char *bp;      

  /* Ignore spurious requests */
  if (size <= 0) {
    return NULL;
  }

  /* Adjust block size to include overhead and alignment reqs. */
  if (size <= DSIZE) {
    asize = DSIZE + OVERHEAD;
  }
  else {
    asize = DSIZE * ((size + (OVERHEAD) + (DSIZE-1)) / DSIZE);
  }
    
  /* Search the free list for a fit */
  if ((bp = find_fit(asize)) != NULL) {
    place(bp, asize);
    return bp;
  }

  /* No fit found. Get more memory and place the block */
  extendsize = MAX(asize,CHUNKSIZE);
  if ((bp = extend_heap(extendsize/WSIZE)) == NULL) {
    return NULL;
  }
  place(bp, asize);
  return bp;
} 
/* $end mmmalloc */

/* 
 * mm_free - Free a block 
 */
/* $begin mmfree */
void mm_free(void *bp)
{
  if(bp == NULL)
    return;

  size_t size = GET_SIZE(HDRP(bp));

  PUT(HDRP(bp), PACK(size, 0));
  PUT(FTRP(bp), PACK(size, 0));
  coalesce(bp);
}


/* $end mmfree */

/*
 * mm_realloc - naive implementation of mm_realloc
 */
void *mm_realloc(void *ptr, size_t size)
{
  void *newp;
  size_t copySize;

  if ((newp = mm_malloc(size)) == NULL) {
    printf("ERROR: mm_malloc failed in mm_realloc\n");
    exit(1);
  }
  copySize = GET_SIZE(HDRP(ptr));
  if (size < copySize) {
    copySize = size;
  }
  memcpy(newp, ptr, copySize);
  mm_free(ptr);
  return newp;
}

/* 
 * mm_check - Check the heap for consistency 
 */
 int mm_check(void) 
 {
     char *bp;

     char* high = mem_heap_hi();
     char*  low = mem_heap_lo();

     for (bp = heap_listp; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp)) {
	 if(bp > high || bp < low) {
	     printf("Error: pointer not in heap\n");
	     return 0;
	 }
     }

     return 1;
 }

 /* The remaining routines are internal helper routines */

 /* 
  * extend_heap - Extend heap with free block and return its block pointer
  */
 /* $begin mmextendheap */
 static void *extend_heap(size_t words) 
 {
     char *bp;
     size_t size;

     /* Allocate an even number of words to maintain alignment */
     size = (words % 2) ? (words+1) * WSIZE : words * WSIZE;
     //round up so we have sapce for header and footer
     if (size < OVERHEAD)
	 size = OVERHEAD;
     //extend heap 
     if ((long)(bp = mem_sbrk(size)) == -1) 
	 return NULL;

     /* Initialize free block header/footer and the epilogue header */
     PUT(HDRP(bp), PACK(size, 0));         /* free block header */
     PUT(FTRP(bp), PACK(size, 0));         /* free block footer */
     PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1)); /* new epilogue header */

     /* Coalesce if the previous block was free and add the block to 
      * the free list */
     return coalesce(bp);
 }
/* $end mmextendheap */

/* 
 * place - Place block of asize bytes at start of free block bp 
 *         and split if remainder would be at least minimum block size
 */
/* $begin mmplace */
/* $begin mmplace-proto */
static void place(void *bp, size_t asize)
/* $end mmplace-proto */
{
  size_t csize = GET_SIZE(HDRP(bp));   

  if ((csize - asize) >= OVERHEAD) { 
    PUT(HDRP(bp), PACK(asize, 1));
    PUT(FTRP(bp), PACK(asize, 1));
    removeblock(bp);
    bp = NEXT_BLKP(bp);
    PUT(HDRP(bp), PACK(csize-asize, 0));
    PUT(FTRP(bp), PACK(csize-asize, 0));
    coalesce(bp);
  }
  else { 
    PUT(HDRP(bp), PACK(csize, 1));
    PUT(FTRP(bp), PACK(csize, 1));
    removeblock(bp);
  }
}
/* $end mmplace */

/* 
 * find_fit - Find a fit for a block with asize bytes 
 */
static void *find_fit(size_t asize)
{
  /* first fit search */
  void *bp;

  for (bp = free_listp; GET_ALLOC(HDRP(bp)) == 0; bp = NEXT_FREEP(bp)) {
    if (asize <= (size_t)GET_SIZE(HDRP(bp))) {
	return bp;
      }
    }
  return NULL; /* no fit */
}

  /*
   * coalesce - boundary tag coalescing. Return ptr to coalesced block
 */
  static void *coalesce(void *bp) 
  {
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));

    if (prev_alloc && next_alloc) {            /* Case 1 */
	return bp;
    }
    else if (prev_alloc && !next_alloc) {      /* Case 2 */
	size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
	removeblock(NEXT_BLKP(bp));
	PUT(HDRP(bp), PACK(size, 0));
	PUT(FTRP(bp), PACK(size,0));
    }
    else if (!prev_alloc && next_alloc) {      /* Case 3 */
	size += GET_SIZE(HDRP(PREV_BLKP(bp)));
	PUT(FTRP(bp), PACK(size, 0));
	PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
	bp = PREV_BLKP(bp);
	removeblock(bp);
    }
    else {                                     /* Case 4 */
	size += GET_SIZE(HDRP(PREV_BLKP(bp))) + 
            GET_SIZE(FTRP(NEXT_BLKP(bp)));
	removeblock(PREV_BLKP(bp));
	removeblock(NEXT_BLKP(bp));
	PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
	PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
	bp = PREV_BLKP(bp);
    }
  
    //insert bp to freelist
    insertblock(bp);
  
    return bp;
  }

  static void printblock(void *bp) 
  {
    size_t hsize, halloc, fsize, falloc;

    hsize = GET_SIZE(HDRP(bp));
    halloc = GET_ALLOC(HDRP(bp));  
    fsize = GET_SIZE(FTRP(bp));
    falloc = GET_ALLOC(FTRP(bp));  
    
    if (hsize == 0) {
      printf("%p: EOL\n", bp);
      return;
    }

    // Allocated block
    if (halloc) {
      printf("%p: header: [%d:%c] footer: [%d:%c]\n", bp, 
	     hsize, (halloc ? 'a' : 'f'), 
	     fsize, (falloc ? 'a' : 'f')); 
    }
    // Free block
    else
      {
        printf("%p: header: [%d:%c] next: %p prev: %p footer: [%d:%c]\n", bp, 
	       hsize, (halloc ? 'a' : 'f'), 
	       NEXT_FREEP(bp),
	       PREV_FREEP(bp),
	       fsize, (falloc ? 'a' : 'f')); 
      }
  }

  static void checkblock(void *bp) 
  {
    if ((size_t)bp % 8) {
      printf("Error: %p is not doubleword aligned\n", bp);
    }
    if (GET(HDRP(bp)) != GET(FTRP(bp))) {
      printf("Error: header does not match footer\n");
    }
  }

/*
 * remove - Removes a block from free list
 * Function takes block pointer of the block to remove as parameter
 */

  static void removeblock(void *bp)
  {
    // If this is the first block, set the next block of the list
    // as the first block, else set next pointer to previous
    // next block.

    if (!PREV_FREEP(bp))
    {
      free_listp = NEXT_FREEP(bp);
    }
    else
    {
      NEXT_FREEP(PREV_FREEP(bp)) = NEXT_FREEP(bp);
    }

    PREV_FREEP(NEXT_FREEP(bp)) = PREV_FREEP(bp);
  }
 
/*
 * insert - Inserts a block to the free list
 * block is placed as the 'head' of the free list
 * 
 * Function takes block pointer of the block to insert as parameter
 */

  static void insertblock(void *bp)
  {
    NEXT_FREEP(bp) = free_listp;
    PREV_FREEP(free_listp) = bp;
    PREV_FREEP(bp) = 0;
    free_listp = bp;
  }
