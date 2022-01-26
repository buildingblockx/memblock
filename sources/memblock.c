#include <memblock.h>

static struct memblock_region memblock_memory_init_regions[INIT_MEMBLOCK_MEMORY_REGIONS];
static struct memblock_region memblock_reserved_init_regions[INIT_MEMBLOCK_RESERVED_REGIONS];

static struct memblock memblock = {
	.memory.regions		= memblock_memory_init_regions,
	.memory.index		= 0,
	.memory.max		= INIT_MEMBLOCK_MEMORY_REGIONS,
	.memory.name		= "memory",

	.reserved.regions	= memblock_reserved_init_regions,
	.reserved.index		= 0,
	.reserved.max		= INIT_MEMBLOCK_RESERVED_REGIONS,
	.reserved.name		= "reserved",

	.limit			= PHYS_ADDR_MAX,
};

/**
 * initialize memblock allocator
 */
void memblock_init(void)
{

}

static inline phys_addr_t memblock_resize(phys_addr_t base, phys_addr_t *size)
{
	return *size = min(*size, memblock.limit - base);
}

/*
 * Insert new memblock region [@base, @base + @size) into @type at @index.
 * @type must already have extra room to accommodate the new region.
 */
static void memblock_insert_region(struct memblock_type *type, int index,
				phys_addr_t base, phys_addr_t size)
{
	struct memblock_region *region = &type->regions[index];

	memmove(region + 1, region, (type->index - index) * sizeof(*region));
	region->base = base;
	region->size = size;
	type->index++;
	type->total_size += size;
}

/*
 * Scan memblock type @type and merge neighboring compatible regions.
 */
static void memblock_merge_regions(struct memblock_type *type)
{
	int i = 0;

	/* index never goes below 1 */
	while (i < type->index - 1) {
		struct memblock_region *this = &type->regions[i];
		struct memblock_region *next = &type->regions[i + 1];

		if (this->base + this->size != next->base) {
			i++;
			continue;
		}

		this->size += next->size;
		/* move forward from next + 1, index of which is i + 2 */
		memmove(next, next + 1, (type->index - (i + 2)) * sizeof(*next));
		type->index--;
	}
}

/*
 * Add new memblock region [@base, @base + @size) into @type. The new region
 * is allowed to overlap with existing ones - overlaps don't affect already
 * existing regions. @type is guaranteed to be minimal (all neighbouring
 * compatible regions are merged) after the addition.
 */
static int __memblock_add(struct memblock_type *type,
			phys_addr_t base, phys_addr_t size)
{
	phys_addr_t ibase = base;
	phys_addr_t iend = base + memblock_resize(base, &size);
	bool insert = false;
	int index, insert_num;
	struct memblock_region *region;

	if (!size)
		return 0;

	/* special case for empty array */
	if (type->index == 0) {
		type->regions[0].base = base;
		type->regions[0].size = size;
		type->total_size = size;
		type->index++;
		return 0;
	}

repeat:
	/*
	 * The following is executed twice.
	 * The first counts the number of regions needed
	 * to accommodate the new area. The second actually inserts them.
	 */
	ibase = base;
	insert_num = 0;

	/*
	 * Three maybe condition for insert
	 *
	 *             rbase        rend
	 *               └────────────┘
	 *            iend
	 * (1)  ───────┘
	 *                               ibase
	 * (2)                             └─────
	 *      ibase
	 * (3)   └─────────────
	 */
	for_each_memblock_region(index, type, region) {
		phys_addr_t rbase = region->base;
		phys_addr_t rend = rbase + region->size;

		if (rbase >= iend)
			break;
		if (rend <= ibase)
			continue;

		/*
		 * @region overlaps. If it separates the lower part of
		 * new area, insert that portion.
		 */
		if (rbase > ibase) {
			insert_num++;
			if (insert)
				memblock_insert_region(type, index++,
						ibase, rbase - ibase);
		}

		/* area below @rend is dealt with, forget about it */
		ibase = min(rend, iend);
	}

	/* insert the remaining portion */
	if (ibase < iend) {
		insert_num++;
		if (insert)
			memblock_insert_region(type, index,
					ibase, iend - ibase);
	}

	if (!insert_num)
		return 0;

	/*
	 * If this was the first round, resize array and repeat for actual
	 * insertions; otherwise, merge and return.
	 */
	if (!insert) {
		while (type->index + insert_num > type->max) {
			pr_debug("resize %s region array.\n", type->name);

			//if (memblock_double_array(type, base, size) < 0)
			//	return -ENOMEM;
		}
		insert = true;
		goto repeat;
	} else {
		memblock_merge_regions(type);
		return 0;
	}
}

/**
 * add useable memory region [@base, @base + @size) to memblock.memory
 */
int memblock_add(phys_addr_t base, phys_addr_t size)
{
	pr_debug("%s: memory range: [0x%lx-0x%lx)\n",
			__func__, base, base + size);

	return __memblock_add(&memblock.memory, base, size);
}

/*
 * add reserve memory region [@base, @base + @size) to memblock.reserved
 */
int memblock_add_reserve(phys_addr_t base, phys_addr_t size)
{
	pr_debug("%s: reserved range: [0x%lx-0x%lx)\n",
			__func__, base, base + size);

	return __memblock_add(&memblock.reserved, base, size);
}

/*
 * isolate given range into disjoint memblocks
 *
 * Walk memblock type @type and ensure that regions don't cross the boundaries defined by
 * [@base, @base + @size).  Crossing regions are split at the boundaries,
 * which may create at most two more regions.  The index of the first
 * region inside the range is returned in @start_region and end in @end_region.
 */
static int memblock_isolate_range(struct memblock_type *type,
				phys_addr_t base, phys_addr_t size,
				int *start_region, int *end_region)
{
	phys_addr_t end = base + memblock_resize(base, &size);
	struct memblock_region *region;
	int index;

	*start_region = *end_region = 0;

	if (!size)
		return 0;

	/* we'll create at most two more regions */
	while (type->index + 2 > type->max) {
		pr_debug("resize %s region array.\n", type->name);

		//if (memblock_double_array(type, base, size) < 0)
		//	return -ENOMEM;
	}

	/*
	 * Four maybe condition for isolate
	 *
	 *          rbase           rend
	 *            └──────────────┘
	 *         end
	 * (1) ─────┘
	 *                            base
	 * (2)                          └─────
	 *               base
	 * (3)            └───
	 *                       end
	 * (4)                 ───┘
	 */
	for_each_memblock_region(index, type, region) {
		phys_addr_t rbase = region->base;
		phys_addr_t rend = rbase + region->size;

		if (rbase >= end)
			break;
		if (rend <= base)
			continue;

		if (rbase < base) {
			/*
			 * @region intersects from below.  Split and continue
			 * to process the next region - the new top half.
			 */
			region->base = base;
			region->size -= base - rbase;
			type->total_size -= base - rbase;
			memblock_insert_region(type, index, rbase, base - rbase);
		} else if (rend > end) {
			/*
			 * @region intersects from above.  Split and redo the
			 * current region - the new bottom half.
			 */
			region->base = end;
			region->size -= end - rbase;
			type->total_size -= end - rbase;
			memblock_insert_region(type, index--, rbase, end - rbase);
		} else {
			/* @region is fully contained, record it */
			if (!*end_region)
				*start_region = index;

			*end_region = index + 1;
		}
	}

	return 0;
}

static void memblock_remove_region(struct memblock_type *type,
					unsigned long index)
{
	type->total_size -= type->regions[index].size;
	memmove(&type->regions[index], &type->regions[index + 1],
		(type->index - (index + 1)) * sizeof(type->regions[index]));
	type->index--;

	/* Special case for empty arrays */
	if (type->index == 0) {
		type->regions[0].base = 0;
		type->regions[0].size = 0;
	}
}

static int __memblock_remove(struct memblock_type *type,
			phys_addr_t base, phys_addr_t size)
{
	int start_region, end_region;
	int i, ret;

	ret = memblock_isolate_range(type, base, size,
			&start_region, &end_region);
	if (ret)
		return ret;

	for (i = end_region - 1; i >= start_region; i--)
		memblock_remove_region(type, i);
	return 0;
}

/**
 * remove useable memory region [@base, @base + @size) from memblock.memory
 */
int memblock_remove(phys_addr_t base, phys_addr_t size)
{
	pr_debug("%s: memory range: [0x%lx-0x%lx)\n",
			__func__, base, base + size);

	return __memblock_remove(&memblock.memory, base, size);
}

/*
 * remove reserve memory region [@base, @base + @size) from memblock.reserved
 */
int memblock_remove_reserve(phys_addr_t base, phys_addr_t size)
{
	pr_debug("%s: reserved range: [0x%lx-0x%lx)\n",
			__func__, base, base + size);

	return __memblock_remove(&memblock.reserved, base, size);
}

/*
 * Find the first region in between (@type_in, !@type_ex) from @*index,
 * fill the out parameters, and update *@index for the next iteration.
 */
static void __next_free_memblock_region(u64 *index,
		struct memblock_type *type_in, /* include memblock type */
		struct memblock_type *type_ex, /* exclude mmeblock type */
		phys_addr_t *out_start,
		phys_addr_t *out_end)
{
	struct memblock_region *region;
	phys_addr_t start_in, end_in;   /* include region [start, end) */
	phys_addr_t start_exp, end_exp; /* expect region [start, end) */
	int index_in;			/* include region index */
	int index_ex;			/* exclude region index */

	index_in = *index & 0xffffffff;
	index_ex = *index >> 32;

	/* scan each include region  */
	for (; index_in < type_in->index; index_in++) {
		region = &type_in->regions[index_in];
		start_in = region->base;
		end_in = region->base + region->size;

		if (!type_ex) {
			if (out_start)
				*out_start = start_in;
			if (out_end)
				*out_end = end_in;

			index_in++;
			*index = (u32)index_in | (u64)index_ex << 32;
			return;
		}

		/* scan each exclude region  */
		for (; index_ex < type_ex->index + 1; index_ex++) {
			region = &type_ex->regions[index_ex];

			/*
			 * The expect region comes from
			 * the opposite exclude region
			 */
			start_exp = index_ex ?
				region[-1].base + region[-1].size : 0;
			end_exp = index_ex < type_ex->index ?
				region->base : PHYS_ADDR_MAX;

			/*
			 * Two maybe condition for alloc memory
			 *
			 * region[-1].base region[-1].end
			 *                               region.base region.end
			 *                   start_exp      end_exp
			 *    └──────────────────┴─────────────┴───────────┘
			 *         end_in
			 * (1) ───────┘
			 *                start_in
			 * (2)               └───────
			 */
			if (start_exp >= end_in)
				break;
			if (start_in < end_exp) {
				if (out_start)
					*out_start = max(start_in, start_exp);
				if (out_end)
					*out_end = min(end_in, end_exp);
				/*
				 * The region which ends first is
				 * advanced for the next iteration.
				 */
				if (end_in <= end_exp)
					index_in++;
				else
					index_ex++;

				*index = (u32)index_in | (u64)index_ex << 32;
				return;
			}
		}
	}

	/* signal end of iteration */
	*index = ULLONG_MAX;
}

static phys_addr_t memblock_find_bottom_up(phys_addr_t start,
		phys_addr_t end, phys_addr_t size, phys_addr_t align)
{
	phys_addr_t this_start, this_end, addr;
	u64 i;

	for_each_free_memblock_region(i, &this_start, &this_end) {
		this_start = clamp(this_start, start, end);
		this_end = clamp(this_end, start, end);

		addr = round_up(this_start, align);
		if (addr < this_end && this_end - addr >= size)
			return addr;
	}

	return 0;
}

static void *__memblock_alloc(phys_addr_t size, phys_addr_t align,
			phys_addr_t start, phys_addr_t end)
{
	phys_addr_t addr;
	//phys_addr_t kernel_end = __pa_symbol(_end);
	phys_addr_t kernel_end = 0;

	/*
	 * make sure we will allocate above the kernel image and
	 * below the memblock max address
	 */
	start = max(start, kernel_end);
	end = min(end, memblock.limit);

	if(!align) {
		align = sizeof(phys_addr_t);
		pr_warn("Must be aligned, default %ldB align\n", align);
	}

	addr = memblock_find_bottom_up(start, end, size, align);
	if (addr) {
		memblock_add_reserve(addr, size);
		return phys_to_virt(addr);
	} else {
		return NULL;
	}
}

/**
 * alloc @size bytes memory and @align bytes align, return the start address
 */
void *memblock_alloc(phys_addr_t size,  phys_addr_t align)
{
	phys_addr_t min_addr = 0;
	phys_addr_t max_addr = memblock.limit;
	void *ptr;

	pr_debug("%lu bytes, align=0x%lx, min_addr=0x%lx, max_addr=0x%lx\n",
		     size, align, min_addr, max_addr);
	ptr = __memblock_alloc(size, align, min_addr, max_addr);
	if (ptr)
		memset(ptr, 0, size);

	return ptr;
}

/**
 * free @size bytes memory from the start address @base
 */
int memblock_free(phys_addr_t base, phys_addr_t size)
{
	return memblock_remove_reserve(base, size);
}

/**
 * print all region, include memblock.memory and memblock.reserved
 */
void memblock_print_all_region(void)
{
	struct memblock_region *region;
	phys_addr_t start, end;
	u64 i;

	pr_debug("%s\n", __func__);

	pr_debug("\tmemblock.memory: \n");
	for_each_memblock_region(i, &memblock.memory, region) {
		start = region->base;
		end = region->base + region->size;
		pr_debug("\t[0x%lx-0x%lx)\n", start, end);
	}

	pr_debug("\tmemblock.reserved: \n");
	for_each_memblock_region(i, &memblock.reserved, region) {
		start = region->base;
		end = region->base + region->size;
		pr_debug("\t[0x%lx-0x%lx)\n", start, end);
	}
}