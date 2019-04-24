/**
 * Buddy Allocator
 *
 * For the list library usage, see http://www.mcs.anl.gov/~kazutomo/list/
 */

/**************************************************************************
 * Conditional Compilation Options
 **************************************************************************/
#define USE_DEBUG 0

/**************************************************************************
 * Included Files
 **************************************************************************/
#include <stdio.h>
#include <stdlib.h>

#include "buddy.h"
#include "list.h"

/**************************************************************************
 * Public Definitions
 **************************************************************************/
#define MIN_ORDER 12
#define MAX_ORDER 20

#define PAGE_SIZE (1<<MIN_ORDER)
/* page index to address */
#define PAGE_TO_ADDR(page_idx) (void *)((page_idx*PAGE_SIZE) + g_memory)

/* address to page index */
#define ADDR_TO_PAGE(addr) ((unsigned long)((void *)addr - (void *)g_memory) / PAGE_SIZE)

/* find buddy address */
#define BUDDY_ADDR(addr, o) (void *)((((unsigned long)addr - (unsigned long)g_memory) ^ (1<<o)) \
									 + (unsigned long)g_memory)

#if USE_DEBUG == 1
#  define PDEBUG(fmt, ...) \
	fprintf(stderr, "%s(), %s:%d: " fmt,			\
		__func__, __FILE__, __LINE__, ##__VA_ARGS__)
#  define IFDEBUG(x) x
#else
#  define PDEBUG(fmt, ...)
#  define IFDEBUG(x)
#endif

/**************************************************************************
 * Public Types
 **************************************************************************/
typedef struct {
	struct list_head list;
	int page_idx;
	int order;
	/* TODO: DECLARE NECESSARY MEMBER VARIABLES */
} page_t;

/**************************************************************************
 * Global Variables
 **************************************************************************/
/* free lists*/
struct list_head free_area[MAX_ORDER+1];

/* memory area */
char g_memory[1<<MAX_ORDER];

/* page structures */
page_t g_pages[(1<<MAX_ORDER)/PAGE_SIZE];

/**************************************************************************
 * Public Function Prototypes
 **************************************************************************/

/**************************************************************************
 * Local Functions
 **************************************************************************/

/**
 * Initialize the buddy system
 */
void buddy_init()
{
	int i;
	int n_pages = (1<<MAX_ORDER) / PAGE_SIZE;
	for (i = 0; i < n_pages; i++) {
		/* TODO: INITIALIZE PAGE STRUCTURES */
		INIT_LIST_HEAD(& g_pages[i].list);

		g_pages[i].page_idx = i;
		g_pages[i].order = -1;
	}

	/* initialize freelist */
	for (i = MIN_ORDER; i <= MAX_ORDER; i++) {
		INIT_LIST_HEAD(&free_area[i]);
	}

	/* add the entire memory as a freeblock */
	list_add(&g_pages[0].list, &free_area[MAX_ORDER]);
	g_pages[0].order = MAX_ORDER;
}

/**
 * Allocate a memory block.
 *
 * On a memory request, the allocator returns the head of a free-list of the
 * matching size (i.e., smallest block that satisfies the request). If the
 * free-list of the matching block size is empty, then a larger block size will
 * be selected. The selected (large) block is then splitted into two smaller
 * blocks. Among the two blocks, left block will be used for allocation or be
 * further splitted while the right block will be added to the appropriate
 * free-list.
 *
 * @param size size in bytes
 * @return memory block address
 */
void *buddy_alloc(int size)
{
	/* TODO: IMPLEMENT THIS FUNCTION */
	void* addr = NULL;
	for(int i=MIN_ORDER; i<=MAX_ORDER; i++)
	{
		if(!list_empty(&free_area[i]) && (1<<i) >= size)
		{
			if(i != MIN_ORDER && (1<<i) >= 2*size)
			{
				//split free_area[i] into 2 lists, put into free_area[i-1]
				split(i);
				//search again
				return buddy_alloc(size);
			}
			else
			{
				//find the page corresponding to free_area[i][head]
				page_t* free_page;
				for(int j=0; j<(1<<MAX_ORDER) / PAGE_SIZE; j++)
				{
					if(free_area[i].next == &g_pages[j].list)
					{
						free_page = &g_pages[j];
						break;
					}
				}
				//find page address
				addr = PAGE_TO_ADDR(free_page->page_idx);
				//move free_area[i] out of free memory
				list_del_init(&free_page->list);
			}
			break;
		}
	}
	return addr;
}

void split(int order)
{
	page_t* free_page = NULL;
	for(int i=0; i<((1<<MAX_ORDER) / PAGE_SIZE); i++)
	{
		if(free_area[order].next == &g_pages[i].list)
		{
			free_page = &g_pages[i];
			break;
		}
	}

	int buddy_page_idx = ADDR_TO_PAGE(BUDDY_ADDR(PAGE_TO_ADDR(free_page->page_idx), order));
	list_move(&free_page->list, &free_area[order-1]);
	list_add_tail(&g_pages[buddy_page_idx].list, &free_area[order-1]);
	free_page->order = order-1;
	g_pages[buddy_page_idx].order = order-1;
}

void merge(void* addr)
{
	int block_page_idx = ADDR_TO_PAGE(addr);
	page_t* block_page = &g_pages[block_page_idx];
	if(block_page->order == MAX_ORDER)
	{
		return;
	}
	int buddy_page_idx = ADDR_TO_PAGE(BUDDY_ADDR(addr,block_page->order));
	page_t* buddy_page = &g_pages[buddy_page_idx];

	if(list_empty(&buddy_page->list))
	{
		return;
	}

	if(block_page_idx > buddy_page_idx)
	{
		list_del_init(&block_page->list);
		buddy_page->order++;
		list_move_tail(&buddy_page->list, &free_area[buddy_page->order]);
		merge(PAGE_TO_ADDR(buddy_page_idx));
	}
	else
	{
		list_del_init(&buddy_page->list);
		block_page->order++;
		list_move_tail(&block_page->list, &free_area[block_page->order]);
		merge(PAGE_TO_ADDR(block_page_idx));
	}
}

/**
 * Free an allocated memory block.
 *
 * Whenever a block is freed, the allocator checks its buddy. If the buddy is
 * free as well, then the two buddies are combined to form a bigger block. This
 * process continues until one of the buddies is not free.
 *
 * @param addr memory block address to be freed
 */
void buddy_free(void *addr)
{
	/* TODO: IMPLEMENT THIS FUNCTION */
	int block_page_idx = ADDR_TO_PAGE(addr);
	list_add(&g_pages[block_page_idx].list, 
		&free_area[g_pages[block_page_idx].order]);
	merge(addr);
}

/**
 * Print the buddy system status---order oriented
 *
 * print free pages in each order.
 */
void buddy_dump()
{
	int o;
	for (o = MIN_ORDER; o <= MAX_ORDER; o++) {
		struct list_head *pos;
		int cnt = 0;
		list_for_each(pos, &free_area[o]) {
			cnt++;
		}
		printf("%d:%dK ", cnt, (1<<o)/1024);
	}
	printf("\n");
}
