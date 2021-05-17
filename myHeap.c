///////////////////////////////////////////////////////////////////////////////
//
// Copyright 2020-2021 Deb Deppeler based on work by Jim Skrentny
// Posting or sharing this file is prohibited, including any changes/additions.
// Used by permission Fall 2020, CS354-deppeler
//
///////////////////////////////////////////////////////////////////////////////
// Main File:myHeap.c       (name of file with main function)
// This File: myHeap.c       (name of this file)
// Other Files: None     (name of all other files if any)
// Semester:         CS 354 Spring 2021
//
// Author:  Yashishvin   Pothuri      (your name)
// Email: pothuri@wisc.edu           (your wisc email address)
// CS Login:  yashishvin      (your CS login name)
//
/////////////////////////// OTHER SOURCES OF HELP //////////////////////////////
//                   Fully acknowledge and credit all sources of help,
//                   other than Instructors and TAs.
//
// Persons:None          Identify persons by name, relationship to you, and email.
//                   Describe in detail the the ideas and help they provided.
//
// Online sources:None Avoid web searches to solve your problems, but if you do
//                   search, be sure to include Web URLs and description of
//                   of any information you find.
////////////////////////////////////////////////////////////////////////////////

// DEB'S PARTIAL SOLUTION FOR SPRING 2021 DO NOT SHARE

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <stdio.h>
#include <string.h>
#include "myHeap.h"

/*
 * This structure serves as the header for each allocated and free block.
 * It also serves as the footer for each free block but only containing size.
 */
typedef struct blockHeader {

    int size_status;
    /*
     * Size of the block is always a multiple of 8.
     * Size is stored in all block headers and in free block footers.
     *
     * Status is stored only in headers using the two least significant bits.
     *   Bit0 => least significant bit, last bit
     *   Bit0 == 0 => free block
     *   Bit0 == 1 => allocated block
     *
     *   Bit1 => second last bit
     *   Bit1 == 0 => previous block is free
     *   Bit1 == 1 => previous block is allocated
     *
     * End Mark:
     *  The end of the available memory is indicated using a size_status of 1.
     *
     * Examples:
     *
     * 1. Allocated block of size 24 bytes:
     *    Allocated Block Header:
     *      If the previous block is free      p-bit=0 size_status would be 25
     *      If the previous block is allocated p-bit=1 size_status would be 27
     *
     * 2. Free block of size 24 bytes:
     *    Free Block Header:
     *      If the previous block is free      p-bit=0 size_status would be 24
     *      If the previous block is allocated p-bit=1 size_status would be 26
     *    Free Block Footer:
     *      size_status should be 24
     */
} blockHeader;

/* Global variable - DO NOT CHANGE. It should always point to the first block,
 * i.e., the block at the lowest address.
 */
blockHeader *heapStart = NULL;

/* Size of heap allocation padded to round to nearest page size.
 */
int allocsize;

/*
 * Additional global variables may be added as needed below
 */


//it will return the size of the block given the pointer
int getBlockSize(blockHeader* block) {
    return block->size_status - (block->size_status & 3);
}

//returns the pointer to the next block given the pointer to the current block
blockHeader* getNextBlock(void* block) {
    return (blockHeader*)(block + getBlockSize(block));
}

// returns the pointer to the block footer
blockHeader* getBlockFooter(void* block) {
    return (blockHeader*)(block - sizeof(blockHeader));
}

/*
 * Function for allocating 'size' bytes of heap memory.
 * Argument size: requested size for the payload
 * Returns address of allocated block (payload) on success.
 * Returns NULL on failure.
 *
 * This function must:
 * - Check size - Return NULL if not positive or if larger than heap space.
 * - Determine block size rounding up to a multiple of 8
 *   and possibly adding padding as a result.
 *
 * - Use BEST-FIT PLACEMENT POLICY to chose a free block
 *
 * - If the BEST-FIT block that is found is exact size match
 *   - 1. Update all heap blocks as needed for any affected blocks
 *   - 2. Return the address of the allocated block payload
 *
 * - If the BEST-FIT block that is found is large enough to split
 *   - 1. SPLIT the free block into two valid heap blocks:
 *         1. an allocated block
 *         2. a free block
 *         NOTE: both blocks must meet heap block requirements
 *       - Update all heap block header(s) and footer(s)
 *              as needed for any affected blocks.
 *   - 2. Return the address of the allocated block payload
 *
 * - If a BEST-FIT block found is NOT found, return NULL
 *   Return NULL unable to find and allocate block for desired size
 *
 * Note: payload address that is returned is NOT the address of the
 *       block header.  It is the address of the start of the
 *       available memory for the requesterr.
 *
 * Tips: Be careful with pointer arithmetic and scale factors.
 */
void* myAlloc(int size) {

    if (size <= 0) return NULL;

    // adding the header and the padding size
    int padsize, blocksize;
    size += sizeof(blockHeader);
    padsize = size % 8;
    padsize = (8 - padsize) % 8;
    blocksize = size + padsize;

    blockHeader *best = NULL;

    // finding the free block that satisfies the best fit
    blockHeader *current = heapStart;
    while (current->size_status != 1) {

        int allocated = current->size_status & 1;
        int len = getBlockSize(current);

        //we compare to similar blocks
        if (allocated == 0 && len >= blocksize) {
            if (best == NULL || len < getBlockSize(best)) {
                best = current;
            }
        }

        current = getNextBlock(current);
    }

    // condition to heck if block is not found
    if (best == NULL) return NULL;
    if (getBlockSize(best) < blocksize + 8) {
        best->size_status = best->size_status + 1;//chaning the a bit of the current block
        blockHeader* nxt = getNextBlock(best); // Change p-bit of the next block
        if (nxt->size_status != 1) {
            nxt->size_status = nxt->size_status + 2;
        }
    }

    // Splitting the block
    else {

        int free_size = getBlockSize(best) - blocksize;
        best->size_status = blocksize + (best->size_status & 2) + 1;
        blockHeader* nxt = getNextBlock(best);
        nxt->size_status = free_size + 2;
        blockHeader* footer = getBlockFooter(nxt);
        footer->size_status = free_size;
    }
    return (blockHeader*)((void*)best + sizeof(blockHeader));
}

/*
 * Function for freeing up a previously allocated block.
 * Argument ptr: address of the block to be freed up.
 * Returns 0 on success.
 * Returns -1 on failure.
 * This function should:
 * - Return -1 if ptr is NULL.
 * - Return -1 if ptr is not a multiple of 8.
 * - Return -1 if ptr is outside of the heap space.
 * - Return -1 if ptr block is already freed.
 * - Update header(s) and footer as needed.
 */
int myFree(void *ptr) {

    if (ptr == NULL)
	{
		return -1;
	}
	// checking for multiple of 8
	blockHeader *toRemove = NULL;
	toRemove = (blockHeader *)(ptr - 4);
	int removeSize = toRemove->size_status >> 2 << 2;
	if (removeSize % 8 != 0)
	{
		return -1;
	}
	//checking if it has not been freed
	if (!(toRemove->size_status & 1))
	{
		return -1;
	}
	//outside the headspace
	blockHeader *endOfheap = NULL;
	endOfheap = heapStart;
	while (endOfheap->size_status != 1)
	{
		int endOfHeapsize  = endOfheap->size_status >> 2 << 2;
		//continous update until the next is found
		endOfheap = (blockHeader *)((void *)endOfheap + endOfHeapsize);
	}
	if (endOfheap < toRemove)
	{
		return -1;
	}
	//setting the tbit of the block to 0
	toRemove->size_status = toRemove->size_status - 1;
	// creting the footer
	blockHeader *footer = (blockHeader *)((void *)toRemove + removeSize - 4);
	footer->size_status = removeSize;
	// the pbit of the next block is set to 0
	blockHeader *next = (blockHeader *)((void *)toRemove + removeSize);
	next->size_status = next->size_status - 2;
	return 0;
}

/*
 * Function for traversing heap block list and coalescing all adjacent
 * free blocks.
 *
 * This function is used for delayed coalescing.
 * Updated header size_status and footer size_status as needed.
 */
int coalesce() {

    blockHeader *current = heapStart;
  // looping through
  while(current->size_status != 1){
    // If a free block is found, loop until the next allocated block and coalesce all adjacents free blocks up to then.
    if(current->size_status % 2 == 0){
      int totalSize = 0;
      blockHeader *lastFooter = (blockHeader*)(current + (current->size_status - (current->size_status % 8) - 4) / sizeof(blockHeader));
      blockHeader *next = (blockHeader*)(current + (current->size_status - (current->size_status % 8)) / sizeof(blockHeader));
      while(next->size_status % 2 == 0){
	totalSize += next->size_status - (next->size_status % 8);
	next = (blockHeader*)(next + (next->size_status - (next->size_status % 8)) / sizeof(blockHeader));
	lastFooter = (blockHeader*)(next + (next->size_status - (next->size_status % 8) - 4) / sizeof(blockHeader));
      }
      current->size_status += totalSize;
      lastFooter->size_status = current->size_status - (current->size_status % 8);
    }
    current = (blockHeader*)(current + (current->size_status - (current->size_status % 8)) / sizeof(blockHeader));
  }
  return 0;
}


/*
 * Function used to initialize the memory allocator.
 * Intended to be called ONLY once by a program.
 * Argument sizeOfRegion: the size of the heap space to be allocated.
 * Returns 0 on success.
 * Returns -1 on failure.
 */
int myInit(int sizeOfRegion)
{

	static int allocated_once = 0; //prevent multiple myInit calls

	int pagesize;	// page size
	int padsize;	// size of padding when heap size not a multiple of page size
	void *mmap_ptr; // pointer to memory mapped area
	int fd;

	blockHeader *endMark;

	if (0 != allocated_once)
	{
		fprintf(stderr,
				"Error:mem.c: InitHeap has allocated space during a previous call\n");
		return -1;
	}

	if (sizeOfRegion <= 0)
	{
		fprintf(stderr, "Error:mem.c: Requested block size is not positive\n");
		return -1;
	}

	// Get the pagesize
	pagesize = getpagesize();

	// Calculate padsize as the padding required to round up sizeOfRegion
	// to a multiple of pagesize
	padsize = sizeOfRegion % pagesize;
	padsize = (pagesize - padsize) % pagesize;

	allocsize = sizeOfRegion + padsize;

	// Using mmap to allocate memory
	fd = open("/dev/zero", O_RDWR);
	if (-1 == fd)
	{
		fprintf(stderr, "Error:mem.c: Cannot open /dev/zero\n");
		return -1;
	}
	mmap_ptr = mmap(NULL, allocsize, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
	if (MAP_FAILED == mmap_ptr)
	{
		fprintf(stderr, "Error:mem.c: mmap cannot allocate space\n");
		allocated_once = 0;
		return -1;
	}

	allocated_once = 1;

	// for double word alignment and end mark
	allocsize -= 8;

	// Initially there is only one big free block in the heap.
	// Skip first 4 bytes for double word alignment requirement.
	heapStart = (blockHeader *)mmap_ptr + 1;

	// Set the end mark
	endMark = (blockHeader *)((void *)heapStart + allocsize);
	endMark->size_status = 1;

	// Set size in header
	heapStart->size_status = allocsize;

	// Set p-bit as allocated in header
	// note a-bit left at 0 for free
	heapStart->size_status += 2;

	// Set the footer
	blockHeader *footer = (blockHeader *)((void *)heapStart + allocsize - 4);
	footer->size_status = allocsize;

	return 0;
}


/*
 * Function to be used for DEBUGGING to help you visualize your heap structure.
 * Prints out a list of all the blocks including this information:
 * No.      : serial number of the block
 * Status   : free/used (allocated)
 * Prev     : status of previous block free/used (allocated)
 * t_Begin  : address of the first byte in the block (where the header starts)
 * t_End    : address of the last byte in the block
 * t_Size   : size of the block as stored in the block header
 */
void dispMem() {

    int counter;
    char status[6];
    char p_status[6];
    char *t_begin = NULL;
    char *t_end   = NULL;
    int t_size;

    blockHeader *current = heapStart;
    counter = 1;

    int used_size = 0;
    int free_size = 0;
    int is_used   = -1;

    fprintf(stdout,
	"*********************************** Block List **********************************\n");
    fprintf(stdout, "No.\tStatus\tPrev\tt_Begin\t\tt_End\t\tt_Size\n");
    fprintf(stdout,
	"---------------------------------------------------------------------------------\n");

    while (current->size_status != 1) {
        t_begin = (char*)current;
        t_size = current->size_status;

        if (t_size & 1) {
            // LSB = 1 => used block
            strcpy(status, "alloc");
            is_used = 1;
            t_size = t_size - 1;
        } else {
            strcpy(status, "FREE ");
            is_used = 0;
        }

        if (t_size & 2) {
            strcpy(p_status, "alloc");
            t_size = t_size - 2;
        } else {
            strcpy(p_status, "FREE ");
        }

        if (is_used)
            used_size += t_size;
        else
            free_size += t_size;

        t_end = t_begin + t_size - 1;

        fprintf(stdout, "%d\t%s\t%s\t0x%08lx\t0x%08lx\t%4i\n", counter, status,
        p_status, (unsigned long int)t_begin, (unsigned long int)t_end, t_size);

        current = (blockHeader*)((char*)current + t_size);
        counter = counter + 1;
    }

    fprintf(stdout,
	"---------------------------------------------------------------------------------\n");
    fprintf(stdout,
	"*********************************************************************************\n");
    fprintf(stdout, "Total used size = %4d\n", used_size);
    fprintf(stdout, "Total free size = %4d\n", free_size);
    fprintf(stdout, "Total size      = %4d\n", used_size + free_size);
    fprintf(stdout,
	"*********************************************************************************\n");
    fflush(stdout);

    return;
}


// end of myHeap.c (sp 2021)

