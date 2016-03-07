/*
 * mm.c -  segregated list .
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
#define OVERHEAD    8       /* overhead of header and footer(bytes) */
#define MIN_BSIZE   16      /* minimum size of block */

#define LISTS     20      /* Number of segregated lists */
#define BUFFER  (1<<7)    /* Reallocation buffer */

#define MAX(x, y) ((x) > (y) ? (x) : (y)) /* Maximum of two numbers */
#define MIN(x, y) ((x) < (y) ? (x) : (y)) /* Minimum of two numbers */

/* Pack size and allocation bit into a word */
#define PACK(size, alloc) ((size) | (alloc))

/* Read and write a word at address p */
#define GET(p)         (*(size_t *)(p))
#define PUT(p, val)    (*(size_t *)(p) = (val) | GET_TAG(p))  
#define PUT_NOTAG(p, val) (*(size_t *)(p) = (val))

/* Store prev or next pointer for free blocks */
#define SET_BP(p, bp) (*(size_t *)(p) = (size_t)(bp))

/* Read the size and allocated fields from address p */
#define GET_SIZE(p)    (GET(p) & ~0x7)
#define GET_ALLOC(p)   (GET(p) & 0x1)
#define GET_TAG(p)   (GET(p) & 0x2)

/* Adjust the reallocation tag */
#define SET_TAG(p)   (*(size_t *)(p) = GET(p) | 0x2)
#define UNSET_TAG(p) (*(size_t *)(p) = GET(p) & ~0x2)

/* Given block bp bp, compute address of its header and footer */
#define HDRP(bp)       ((char *)(bp) - WSIZE)  
#define FTRP(bp)       ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

/* Given block bp bp, compute address of next and previous blocks */
#define NEXT_BLKP(bp)  ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))
#define PREV_BLKP(bp)  ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))

/* Address of free block's previous and next entries */
#define PREV_BP(bp) ((char *)(bp))
#define NEXT_BP(bp) ((char *)(bp) + WSIZE)

/* Address of free block's previous and next on the segregated list */
#define NEXT_FREEP(bp) (*(char **)(bp))
#define PREV_FREEP(bp) (*(char **)(NEXT_BP(bp)))

#define LINE_OFFSET   4 /* Line offset for referencing trace files */

/* Global variables */
static char *heap_listp;  /* pointer to prologue block */  
static void *sgrlists[LISTS];  /* pointer to free list */

/* function prototypes for internal helper routines */
static void *extend_heap(size_t size);
static void *coalesce(void *bp);
static void place(void *bp, size_t asize);
static void insert_block(void *bp, size_t size);
static void delete_block(void *bp);

//static void mm_check(char caller, void *bp, int size);

/* 
 * mm_init - Initialize the memory manager 
 */
/* $begin mminit */
int mm_init(void)
{
  int list;         
  
  /* initialize sgrlists to null */
  for (list = 0; list < LISTS; list++) {
    sgrlists[list] = NULL;
  }

  /* create the initial empty heap */
  if ((long)(heap_listp = mem_sbrk(4 * WSIZE)) == -1)
    return -1;
  
  PUT_NOTAG(heap_listp, 0);                            /* alignment padding */
  PUT_NOTAG(heap_listp + (1 * WSIZE), PACK(DSIZE, 1)); /* prologue header */
  PUT_NOTAG(heap_listp + (2 * WSIZE), PACK(DSIZE, 1)); /* prologue footer */
  PUT_NOTAG(heap_listp + (3 * WSIZE), PACK(0, 1));     /* epilogue header */
  heap_listp += DSIZE;
  
  /* Extend the empty heap */
  if (extend_heap(CHUNKSIZE/WSIZE) == NULL)
    return -1;

  return 0;
}
/* $end mminit */

/* 
 * mm_malloc - Allocate a block with at least size bytes of payload 
 */
/* $begin mmmalloc */
void *mm_malloc(size_t size)
{
  size_t asize;      /* Adjusted block size */
  size_t extendsize; /* Amount to extend heap if no fit */
  void *bp = NULL;  
  int list = 0;      /* List counter */      
  
  /* Ignore spurious requests */
  if (size <= 0) {
    return NULL;
  }

  /* Adjust block size to include overhead and alignment reqs. */
  if (size <= DSIZE) {
    asize = 2*DSIZE;
  }
  else {
    asize = DSIZE * ((size + (DSIZE) + (DSIZE-1)) / DSIZE);
  }
  
  /* Search the sgrlists for a fit */
  size = asize;
  while (list < LISTS) {
    
    if (((size <= 1) && (sgrlists[list] != NULL)) || (list == LISTS - 1))  {
      bp = sgrlists[list];
      
      while ((bp != NULL) && ((asize > GET_SIZE(HDRP(bp))) || (GET_TAG(HDRP(bp)))))
      {
        bp = NEXT_FREEP(bp);
      }
      if (bp != NULL)
      {
        break;
      }
    }
    
    size >>= 1;
  }
  
  /* No fit found. Get more memory and place the block */
  if (ptr == NULL) {
    extendsize = MAX(asize,CHUNKSIZE);
    if ((bp = extend_heap(extendsize/WSIZE)) == NULL) {
      return NULL;
    }
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
  size_t size = GET_SIZE(HDRP(bp));

  UNSET_TAG(HDRP(NEXT_BLKP(bp)));

  PUT(HDRP(bp), PACK(size, 0));
  PUT(FTRP(bp), PACK(size, 0));

  insert_block(bp, size);
  coalesce(bp);
}
/* $end mmfree */


void *mm_realloc(void *bp, size_t size)
{
  void *new_BP = bp;    /* Pointer to be returned */
  size_t new_size = size; /* Size of new block */
  int remainder;          /* Adequacy of block sizes */
  int extendsize;         /* Size of heap extension */
  int block_buffer;       /* Size of block buffer */
  
  /* Filter invalid block size */
  if (size == 0)
    return NULL;
  
  /* Adjust block size to include boundary tag and alignment requirements */
  if (new_size <= DSIZE) {
    new_size = 2 * DSIZE;
  } else {
    new_size = DSIZE * ((new_size + (DSIZE) + (DSIZE - 1)) / DSIZE);
  }
  
  /* Add overhead requirements to block size */
  new_size += BUFFER;
  
  /* Calculate block buffer */
  block_buffer = GET_SIZE(HDRP(bp)) - new_size;
  
  /* Allocate more space if overhead falls below the minimum */
  if (block_buffer < 0) {
    /* Check if next block is a free block or the epilogue block */
    if (!GET_ALLOC(HDRP(NEXT_BLKP(bp))) || !GET_SIZE(HDRP(NEXT_BLKP(bp)))) {
      remainder = GET_SIZE(HDRP(bp)) + GET_SIZE(HDRP(NEXT_BLKP(bp))) - new_size;
      if (remainder < 0) {
        extendsize = MAX(-remainder, CHUNKSIZE);
        if (extend_heap(extendsize) == NULL)
          return NULL;
        remainder += extendsize;
      }
      
      delete_block(NEXT_BLKP(bp));
      
      // Do not split block
      PUT_NOTAG(HDRP(bp), PACK(new_size + remainder, 1)); /* Block header */
      PUT_NOTAG(FTRP(bp), PACK(new_size + remainder, 1)); /* Block footer */
    } else {
      new_BP = mm_malloc(new_size - DSIZE);
      //line_count--;
      memmove(new_BP, bp, MIN(size, new_size));
      mm_free(bp);
      //line_count--;
    }
    block_buffer = GET_SIZE(HDRP(new_BP)) - new_size;
  }  

  /* Tag the next block if block overhead drops below twice the overhead */
  if (block_buffer < 2 * BUFFER)
    SET_TAG(HDRP(NEXT_BLKP(new_BP)));

  /*
  // Check heap for consistency
  line_count++;
  if (CHECK && CHECK_REALLOC) {
    mm_check('r', bp, size);
  }
  */
  
  /* Return reallocated block */
  return new_BP;
}

 /* 
  * extend_heap - Extend heap with free block and return its block pointer
  */
 /* $begin mmextendheap */
static void *extend_heap(size_t words)
{
  void *bp;                   
  size_t size;               
  
  /* Allocate an even number of words to maintain alignment */
  size = (words % 2) ? (words + 1) * WSIZE : words * WSIZE;
  
  if (size < OVERHEAD)
    size = OVERHEAD;

  //extend heap 
  if ((long)(bp = mem_sbrk(size)) == -1) 
    return NULL;
  
  /* Initialize free block header/footer and the epilogue header */
  PUT_NOTAG(HDRP(bp), PACK(size, 0));         /* free block header */
  PUT_NOTAG(FTRP(bp), PACK(size, 0));         /* free block footer */
  PUT_NOTAG(HDRP(NEXT_BLKP(bp)), PACK(0, 1)); /* new epilogue header */
  
  /* Coalesce if the previous block was free and add the block to 
     the free list */
  insert_block(bp, size);
  return coalesce(bp);
}
  /*
   * insert_block - boundary tag coalescing. Return ptr to coalesced block
 */
static void insert_block(void *bp, size_t size) {
  int list = 0;
  void *search_BP = bp;
  void *insert_BP = NULL;
  
  /* Select segregated list */
  while ((list < LISTS - 1) && (size > 1)) {
    size >>= 1;
    list++;
  }
  
  /* Select location on list to insert pointer while keeping list
     organized by byte size in ascending order. */
  search_BP = sgrlists[list];
  while ((search_BP != NULL) && (size > GET_SIZE(HDRP(search_BP)))) {
    insert_BP = search_BP;
    search_BP = NEXT_FREEP(search_BP);
  }
  
  /* Set previous and next */
  if (search_BP != NULL) {
    if (insert_BP != NULL) {
      SET_BP(PREV_BP(bp), search_BP); 
      SET_BP(NEXT_BP(search_BP), bp);
      SET_BP(NEXT_BP(bp), insert_BP);
      SET_BP(PREV_BP(insert_BP), bp);
    } else {
      SET_BP(PREV_BP(bp), search_BP); 
      SET_BP(NEXT_BP(search_BP), bp);
      SET_BP(NEXT_BP(bp), NULL);
      
      /* Add block to appropriate list */
      sgrlists[list] = bp;
    }
  } else {
    if (insert_BP != NULL) {
      SET_BP(PREV_BP(bp), NULL);
      SET_BP(NEXT_BP(bp), insert_BP);
      SET_BP(PREV_BP(insert_BP), bp);
    } else {
      SET_BP(PREV_BP(bp), NULL);
      SET_BP(NEXT_BP(bp), NULL);
      
      /* Add block to appropriate list */
      sgrlists[list] = bp;
    }
  }

  return;
}

/*
 * delete_block: Remove a free block pointer from a segregated list. If
 *              necessary, adjust pointers in previous and next blocks
 *              or reset the list head.
 */
static void delete_block(void *bp) {
  int list = 0;
  size_t size = GET_SIZE(HDRP(bp));
  
  /* Select segregated list */
  while ((list < LISTS - 1) && (size > 1)) {
    size >>= 1;
    list++;
  }
  
  if (NEXT_FREEP(bp) != NULL) {
    if (PREV_FREEP(bp) != NULL) {
      SET_BP(NEXT_BP(NEXT_FREEP(bp)), PREV_FREEP(bp));
      SET_BP(PREV_BP(PREV_FREEP(bp)), NEXT_FREEP(bp));
    } else {
      SET_BP(NEXT_BP(NEXT_FREEP(bp)), NULL);
      sgrlists[list] = NEXT_FREEP(bp);
    }
  } else {
    if (PREV_FREEP(bp) != NULL) {
      SET_BP(PREV_BP(PREV_FREEP(bp)), NULL);
    } else {
      sgrlists[list] = NULL;
    }
  }
  
  return;
}

/*
 * coalesce - Coalesce adjacent free blocks. Sort the new free block into the
 *            appropriate list.
 */
static void *coalesce(void *bp)
{
  size_t prev_alloc = GET_ALLOC(HDRP(PREV_BLKP(bp)));
  size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
  size_t size = GET_SIZE(HDRP(bp));
  
  /* Return if previous and next blocks are allocated */
  if (prev_alloc && next_alloc) {
    return bp;
  }
  
  /* Do not coalesce with previous block if it is tagged */
  if (GET_TAG(HDRP(PREV_BLKP(bp))))
    prev_alloc = 1;
  
  /* Remove old block from list */
  delete_block(bp);
  
  /* Detect free blocks and merge, if possible */
  if (prev_alloc && !next_alloc) {
    delete_block(NEXT_BLKP(bp));
    size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
  } else if (!prev_alloc && next_alloc) {
    delete_block(PREV_BLKP(bp));
    size += GET_SIZE(HDRP(PREV_BLKP(bp)));
    PUT(FTRP(bp), PACK(size, 0));
    PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
    bp = PREV_BLKP(bp);
  } else {
    delete_block(PREV_BLKP(bp));
    delete_block(NEXT_BLKP(bp));
    size += GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(HDRP(NEXT_BLKP(bp)));
    PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
    PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
    bp = PREV_BLKP(bp);
  }
  
  /* Adjust segregated linked lists */
  insert_block(bp, size);
  
  return bp;
}

/*
 * place - Set headers and footers for newly allocated blocks. Split blocks
 *         if enough space is remaining.
 */
static void place(void *bp, size_t asize)
{
  size_t bp_size = GET_SIZE(HDRP(bp));
  size_t remainder = bp_size - asize;
  
  /* Remove block from list */
  delete_block(bp);
  
  if (remainder >= MIN_BSIZE) {
    /* Split block */
    PUT(HDRP(bp), PACK(asize, 1)); /* Block header */
    PUT(FTRP(bp), PACK(asize, 1)); /* Block footer */
    PUT_NOTAG(HDRP(NEXT_BLKP(bp)), PACK(remainder, 0)); /* Next header */
    PUT_NOTAG(FTRP(NEXT_BLKP(bp)), PACK(remainder, 0)); /* Next footer */  
    insert_block(NEXT_BLKP(bp), remainder);
  } else {
    /* Do not split block */
    PUT(HDRP(bp), PACK(bp_size, 1)); /* Block header */
    PUT(FTRP(bp), PACK(bp_size, 1)); /* Block footer */
  }
  return;
}
