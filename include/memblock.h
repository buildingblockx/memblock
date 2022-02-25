#ifndef __MEMBLOCK_H
#define __MEMBLOCK_H

#include <types.h>
#include <limits.h>

#define INIT_MEMBLOCK_MEMORY_REGIONS	128
#define INIT_MEMBLOCK_RESERVED_REGIONS	INIT_MEMBLOCK_MEMORY_REGIONS

struct memblock_region {
	phys_addr_t base; /* base physical address of the region */
	phys_addr_t size; /* size of the region */
};

struct memblock_type {
	unsigned long index;
	unsigned long max; /* size of the allocated regions array */
	struct memblock_region *regions;

	phys_addr_t total_size; /* size of all regions */
	char *name;
};

struct memblock {
	phys_addr_t limit; /* limit of physical address */

	struct memblock_type memory;   /* usabe memory regions */
	struct memblock_type reserved; /* reserved memory regions */
};

extern struct memblock memblock;

int memblock_isolate_range(struct memblock_type *type,
			phys_addr_t base, phys_addr_t size,
			int *start_region, int *end_region);

void __next_free_memblock_region(uint64_t *index,
		struct memblock_type *type_in, /* include memblock type */
		struct memblock_type *type_ex, /* exclude mmeblock type */
		phys_addr_t *out_start,
		phys_addr_t *out_end);

#define for_each_memblock_region(i, type, region)	\
	for (i = 0, region = &(type)->regions[0];	\
	     i < (type)->index;				\
	     i++, region = &(type)->regions[i])

#define for_each_free_memblock_region(i, p_start, p_end)		\
	for (i = 0, __next_free_memblock_region(&i,			\
		&memblock.memory, &memblock.reserved, p_start, p_end);	\
	     i != (uint64_t)ULLONG_MAX;					\
	     __next_free_memblock_region(&i,				\
		&memblock.memory, &memblock.reserved, p_start, p_end))

#define MEMBLOCK_START_OF_DRAM	(memblock.memory.regions[0].base)
#define MEMBLOCK_END_OF_DRAM	(memblock.memory.regions[0].base + \
				memblock.memory.total_size)

#define min_pfn	(MEMBLOCK_START_OF_DRAM >> PAGE_SHIFT)
#define max_pfn	(MEMBLOCK_END_OF_DRAM >> PAGE_SHIFT)

#endif /* __MEMBLOCK_H */
