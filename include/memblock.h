#ifndef __MEMBLOCK_H
#define __MEMBLOCK_H

#include <memory/allocator/memblock.h>

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

void __next_free_memblock_region(u64 *index,
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
	     i != (u64)ULLONG_MAX;					\
	     __next_free_memblock_region(&i,				\
		&memblock.memory, &memblock.reserved, p_start, p_end))

#endif /* __MEMBLOCK_H */

