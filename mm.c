/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 *
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
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
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Team name */
    "solo_team",
    /* First member's full name */
    "Daeell",
    /* First member's email address */
    "daelkdev@gamil.com",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""};

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT - 1)) & ~0x7)
#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

#define WSIZE 4             // word and header footer size (byte)
#define DSIZE 8             // double word size (byte)
#define QSIZE 16            // quad word size (byte)
#define CHUNKSIZE (1 << 12) // initial free block size and default size for expanding the heap (byte)

#define MAX(x, y) ((x) > (y) ? (x) : (y))

#define PACK(size, alloc) ((size) | (alloc)) // size와 alloc 여부를 1word로 return

/* address p위치에 words를 read와 write를 한다. */
#define GET(p) (*(unsigned int *)(p))              // return p값, (unsigned int *) 형변환으로 한 칸당 단위.
#define PUT(p, val) (*(unsigned int *)(p) = (val)) // (unsigned int *) 형변환으로 한 칸당 단위.

#define GET_SIZE(p) (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)

#define HDRP(bp) ((char *)(bp)-WSIZE)
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(((char *)(bp)-WSIZE)))
#define PREV_BLKP(bp) ((char *)(bp)-GET_SIZE(((char *)(bp)-DSIZE)))

// explicit 추가
#define PREV_FREE(bp) (*(void **)(bp))
#define NEXT_FREE(bp) (*(void **)(bp + WSIZE))

static void *heap_listp = NULL;
static void *free_listp = NULL; // explicit 추가 - free list 시작 부분

static void *coalesce(void *bp);           // free block 연결
static void *extend_heap(size_t words);    // heap size 추가
static void *find_fit(size_t asize);       // fit 방식
static void place(void *bp, size_t asize); // block 분할

/* explicit 추가 */
static void removeblock(void *bp);
static void putblock(void *bp);

/*
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    heap_listp = mem_sbrk(6 * WSIZE);
    if (heap_listp == (void *)-1)
        return -1;
    PUT(heap_listp, 0);
    PUT(heap_listp + (WSIZE), PACK(QSIZE, 1));     // prologue header
    PUT(heap_listp + (2 * WSIZE), NULL);           // prev
    PUT(heap_listp + (3 * WSIZE), NULL);           // next
    PUT(heap_listp + (4 * WSIZE), PACK(QSIZE, 1)); // prologue footer
    PUT(heap_listp + (5 * WSIZE), PACK(0, 1));     // epilogue header

    free_listp = heap_listp + DSIZE;

    /*Extend the empty heap with a free blcok if CHUNKING bytes*/
    if (extend_heap(CHUNKSIZE / WSIZE) == NULL)
        return -1;
    return 0;
}

static void *find_fit(size_t asize)
{
    void *bp;

    for (bp = free_listp; GET_ALLOC(HDRP(bp)) != 1; bp = NEXT_FREE(bp))
    {
        if (GET_SIZE(HDRP(bp)) >= asize)
        {
            return bp;
        }
    }
    return NULL;
}

/*
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    size_t asize;      /* 크기 조정 */
    size_t extendsize; /* 맞는 size의 block이 없을 때 확장 */
    char *bp;

    /* 유효하지 않은 요청 */
    if (size <= 0)
        return NULL;
    /* 최소 사이즈보다 작은 block*/
    if (size <= DSIZE)
        asize = 2 * DSIZE;
    else
        asize = ALIGN(size + SIZE_T_SIZE);

    /* first fit */
    if ((bp = find_fit(asize)) != NULL)
    {
        place(bp, asize);
        return bp;
    }

    extendsize = MAX(asize, CHUNKSIZE);
    if ((bp = extend_heap(extendsize / WSIZE)) == NULL)
        return NULL;
    place(bp, asize);
    return bp;
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *bp)
{
    size_t size = GET_SIZE(HDRP(bp));

    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    coalesce(bp);
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *bp, size_t size)
{
    if (size <= 0)
    {
        mm_free(bp);
        return 0;
    }
    if (bp == NULL)
    {
        return mm_malloc(size);
    }
    void *newp = mm_malloc(size);
    if (newp == NULL)
    {
        return 0;
    }
    size_t oldsize = GET_SIZE(HDRP(bp));
    if (size < oldsize)
    {
        oldsize = size;
    }
    // 메모리의 특정한 부분으로부터 얼마까지의 부분을 다른 메모리 영역으로
    // 복사해주는 함수(bp로부터 oldsize만큼의 문자를 newp로 복사해라)
    memcpy(newp, bp, oldsize);
    mm_free(bp);
    return newp;
}

/*
 * Util functions
 */

static void *coalesce(void *bp)
{
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp))); // previous block status
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp))); // next block status
    size_t size = GET_SIZE(HDRP(bp));                   // current block size

    /* 할당 해제 과정
        1. 할당을 해제할 block의 주소를 받는다.
        2. 할당을 해제할 block의 prev, next block을 확인한다.
        3. 기존의 free block이 있다면 해당 free block을 free list에서 제거하고 할당을 해제할 block과 결합하여 free list의 앞에 추가한다.
    */
    if (prev_alloc && next_alloc)
    {
        /* Case 1 : both of two allocated */
        putblock(bp);
        return bp;
    }

    else if (prev_alloc && !next_alloc)
    { /* Case 2 : prev block allocated only */
        removeblock(NEXT_BLKP(bp));
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
    }

    else if (!prev_alloc && next_alloc)
    { /* Case 3 : next block allocated only */
        removeblock(PREV_BLKP(bp));
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        bp = PREV_BLKP(bp);
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
    }

    else
    { /* Case 4 : both of two free */
        removeblock(PREV_BLKP(bp));
        removeblock(NEXT_BLKP(bp));
        size += GET_SIZE(HDRP(PREV_BLKP(bp))) +
                GET_SIZE(FTRP(NEXT_BLKP(bp)));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }
    putblock(bp);
    return bp;
}

/*
 * extend_heap - extens the heap with a new free block
 */
static void *extend_heap(size_t words)
{
    char *bp;
    size_t size;

    /* 정렬 기준을 유지하기 위해 짝수 word로 정렬한다. */
    size = (words % 2) ? (words + 1) * WSIZE : words * WSIZE;
    if ((bp = mem_sbrk(size)) == (void *)-1)
        return NULL;

    PUT(HDRP(bp), PACK(size, 0));         // Free block header
    PUT(FTRP(bp), PACK(size, 0));         // Free block footer
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1)); // New epilogue header

    /* 이전에 free block이 있다면 연결한다. */
    return coalesce(bp);
}

static void place(void *bp, size_t asize)
{
    size_t csize = GET_SIZE(HDRP(bp));
    removeblock(bp);
    if ((csize - asize) >= (2 * DSIZE))
    {
        PUT(HDRP(bp), PACK(asize, 1));
        PUT(FTRP(bp), PACK(asize, 1));
        // split block
        bp = NEXT_BLKP(bp);
        PUT(HDRP(bp), PACK(csize - asize, 0));
        PUT(FTRP(bp), PACK(csize - asize, 0));
        putblock(bp);
    }
    else
    {
        PUT(HDRP(bp), PACK(csize, 1));
        PUT(FTRP(bp), PACK(csize, 1));
    }
}

/* free list에 block을 배치한다. */
void putblock(void *bp)
{
    NEXT_FREE(bp) = free_listp;
    PREV_FREE(bp) = NULL;
    PREV_FREE(free_listp) = bp;
    free_listp = bp;
}

/* free list에 block을 제거한다. */
void removeblock(void *bp)
{
    if (bp == free_listp)
    {
        PREV_FREE(NEXT_FREE(bp)) = NULL;
        free_listp = NEXT_FREE(bp);
    }
    else
    {
        PREV_FREE(NEXT_FREE(bp)) = PREV_FREE(bp);
        NEXT_FREE(PREV_FREE(bp)) = NEXT_FREE(bp);
    }
}