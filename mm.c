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
  return 0;
}
/* $end mminit */

/* 
 * mm_malloc - Allocate a block with at least size bytes of payload 
 */
/* $begin mmmalloc */
void *mm_malloc(size_t size) 
{
  return NULL;
} 
/* $end mmmalloc */

/* 
 * mm_free - Free a block 
 */
/* $begin mmfree */
void mm_free(void *bp)
{
  
}


/* $end mmfree */

/*
 * mm_realloc - naive implementation of mm_realloc
 */
void *mm_realloc(void *ptr, size_t size)
{
  return NULL;
}

/* 
 * mm_check - Check the heap for consistency 
 */
 int mm_check(void) 
 {
   void *ptr = heap_listp;
   void *ptr2 = heap_listp;
   int lastWasFree = 0;
   int firstLoop = 1;

   // Check if every block in free list is marked free
   while(GET_SIZE(ptr) != 0)
   {
     if (GET_ALLOC(ptr) == 1)
     {
       printf("ERROR: allocated block in freelist!");
       return -1;
     }

     ptr = NEXT_FREEP(ptr);
   }
   
   ptr = heap_listp;

   // Check if there are any free blocks side by side
   while(GET_SIZE(ptr) != 0)
   {
     if (GET_ALLOC(ptr) == 0)
     {
       if (lastWasFree)
	 {
	   printf("ERROR: Two free blcoks side by side!");
	   return -1;
	 }
       lastWasFree = 1;
     }
     else
     {
       lastWasFree = 0;
     }

     ptr = NEXT_BLKP(ptr);
   }
   
   ptr = heap_listp;

   // Check if every free block is in the list
   
   while(GET_SIZE(ptr) != 0)
   {
     while(GET_SIZE(ptr2) != 0)
     {
       if (GET_ALLOC(ptr2) == 0)
       {
	 if (ptr != ptr2)
	 {
	   printf("ERROR: Found free block that is not in freelist!");
	   return 1;
	 }
       }
       
       ptr2 = NEXT_BLKP(ptr);
     }

     ptr = NEXT_FREEP(ptr);
   }

   ptr = heap_listp;
   ptr2 = NEXT_FREEP(ptr);

   // Check if pointers in consecutive blocks point to each other
   
   while(GET_SIZE(ptr) != 0 && GET_SIZE(ptr2) != 0)
   {
     if (NEXT_FREEP(ptr) != ptr2 || PREV_FREEP(ptr2) != ptr)
     {
       printf("ERROR: Pointers in consecutive blocks don't point to each other!");
       return 1;
     }

     ptr = NEXT_FREEP(ptr);
     ptr2 = NEXT_FREEP(ptr);
   }
     
   ptr = heap_listp;
   ptr2 = heap_listp;

   // Check if there are cycles in the list

   while(GET_SIZE(ptr) != 0 && GET_SIZE(ptr2) != 0 && GET_SIZE(NEXT_FREEP(ptr2)) != 0)
   {
     if (!firstLoop && ptr == ptr2)
     {
       printf("ERROR: There is a cycle in the list!");
       return 1;
     }
       
     ptr = NEXT_FREEP(ptr);
     ptr2 = NEXT_FREEP(NEXT_FREEP(ptr));

     if (firstLoop)
     {
       firstLoop = 0;
     }
   }

   ptr = heap_listp;

   // Check each block for errors

   while(GET_SIZE(ptr) != 0)
   {
     checkblock(ptr);

     ptr = NEXT_FREEP(ptr);
   }


   return 0;
 }

 /* The remaining routines are internal helper routines */

 /* 
  * extend_heap - Extend heap with free block and return its block pointer
  */
 /* $begin mmextendheap */
 static void *extend_heap(size_t words) 
 {
   return NULL;
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
  
}
/* $end mmplace */

/* 
 * find_fit - Find a fit for a block with asize bytes 
 */
static void *find_fit(size_t asize)
{
  return NULL;
}

  /*
   * coalesce - boundary tag coalescing. Return ptr to coalesced block
 */
  static void *coalesce(void *bp) 
  {
    return NULL;
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

    if (GET_ALLOC(bp))
      {
	printf("%p: header: [%d:%c] footer: [%d:%c]\n", bp,
	       hsize, (halloc ? 'a' : 'f'),
	       fsize, (falloc ? 'a' : 'f'));
      }
    else
      {
	printf("%p: header: [%d:%c] prev: %p next: %p footer: [%d:%c]\n", bp,
	       hsize, (halloc ? 'a' : 'f'),
	       NEXT_FREEP(bp),
	       PREV_FREEP(bp),
	       fsize, (falloc ? 'a' : 'f'));
      }

  }

static void checkblock(void *bp) 
{
  // are header and footer the same?
  if(GET(HDRP(bp)) != GET(FTRP(bp))) 
    {
      printf("CHECKBLOCK ERROR: header and footer do not match!");
    }

  // is the alignment divisible by 8?
  if((size_t)bp % 8)
    {
      printf("CHECKBLOCK ERROR: bp is not divisible by 8!");
    }

  // is every block within boundries?
  if(bp < mem_heap_lo() || bp > mem_heap_hi())
    {
      printf("CHECKBLOCK ERROR: NEXT_FREEP(bp) is not on heap!");
    }

  // do the pointers point to vlaid addresses?
  if(NEXT_FREEP(bp) < mem_heap_lo() || NEXT_FREEP(bp) > mem_heap_hi())
    {
      printf("CHECKBLOCK ERROR: NEXT_FREEP(bp) is not on heap!");
    }
  if(PREV_FREEP(bp) < mem_heap_lo() || PREV_FREEP(bp) > mem_heap_hi())
    {
      printf("CHECKBLOCK ERROR: PREV_FREEP(bp) is not on heap!"); 
    }

  // is header size from footer and footer size from header?
  if((bp + GET_SIZE(HDRP(bp))) != FTRP(bp) || (bp - GET_SIZE(FTRP(bp))) != HDRP(bp))
    {
      printf("CHECKBLOCK ERROR: payload does not match size!");
    }
  
}

/*
 * remove - Removes a block from free list
 * Function takes block pointer of the block to remove as parameter
 */

  static void removeblock(void *bp)
  {
   
  }
 
/*
 * insert - Inserts a block to the free list
 * block is placed as the 'head' of the free list
 * 
 * Function takes block pointer of the block to insert as parameter
 */

  static void insertblock(void *bp)
  {
    
  }
