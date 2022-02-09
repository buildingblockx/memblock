#include <memory/allocator/memblock.h>

#define BUFFER2_SIZE (4096) // 4k
u_int8_t *buffer2;

int memblock_test(void)
{
	u_int8_t *tmp1;
	u64 *tmp2;
	phys_addr_t size, align;

	pr_debug("\nmemblock allocator init\n");
	pr_debug("Add xxx bytes to memblock memory region\n");
	memblock_init();

	pr_debug("Add 4KB to memblock memory region\n");
	buffer2 = malloc(BUFFER2_SIZE);
	memblock_add((phys_addr_t)buffer2, BUFFER2_SIZE);

	size = 1;
	align = 1;
	pr_debug("allocate memory, size %ld, align %ld\n", size, align);
	tmp1 = memblock_alloc(size, align);
	*tmp1 = 1;
	pr_debug("tmp1 = 0x%x\n", *tmp1);

	size = 8;
	align = 8;
	pr_debug("allocate memory, size %ld, align %ld\n", size, align);
	tmp2 = memblock_alloc(size, align);
	*tmp2 = 0x1234;
	pr_debug("tmp2 = 0x%llx\n", *tmp2);

	memblock_print_all_region();

	size = 1;
	pr_debug("free memory, size %ld\n", size);
	memblock_free(virt_to_phys(tmp1), size);

	size = 8;
	pr_debug("free memory, size %ld\n", size);
	memblock_free(virt_to_phys(tmp2), size);

	pr_debug("remove memblock memory region buffer2\n");
	memblock_remove(virt_to_phys(buffer2), BUFFER2_SIZE);

	memblock_print_all_region();

	free(buffer2);

	return 0;
}

