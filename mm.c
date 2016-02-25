/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 * 
 *  In our approach, a block is allocated by checking the heap for 
 *  the next available free block in an explicit list. An 
 *  allocated block consists of a header and a footer that store it's size,
 *  and the payload. A free block consists of a header and a footer that
 *  that store it's size, and it then uses part of it's payload to
 *  keep track of the next and previous free blocks.
 *  When a block is freed we check whether the block before it is also 
 *  a free block, if it is we coalesce, and the same goes for the block after it.
 *  We then set it as the "next block to check" which a "next pointer" in our
 *  next fit implementation keeps track of. We then set the new block's next pointer to 
 *  the old value of the "next pointer" and it's prev pointer to the location of the previous
 *  block we checked, which is stored in the prev pointer of the old "next pointer" block.
 *  The value of that block's next pointer is changed to the location of the new block and
 *  the value of our old "next pointer"'s prev pointer is set to the location our new block as well.
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

/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    return 0;
}

/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 * Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    int newsize = ALIGN(size + SIZE_T_SIZE);
    void *p = mem_sbrk(newsize);
    if (p == (void *)-1) {
        return NULL;
    }
    else {
        *(size_t *)p = size;
        return (void *)((char *)p + SIZE_T_SIZE);
    }
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
    void *oldptr = ptr;
    void *newptr;
    size_t copySize;
    
    newptr = mm_malloc(size);
    if (newptr == NULL) {
        return NULL;
    }
    copySize = *(size_t *)((char *)oldptr - SIZE_T_SIZE);
    if (size < copySize) {
        copySize = size;
    }
    memcpy(newptr, oldptr, copySize);
    mm_free(oldptr);
    return newptr;
}
