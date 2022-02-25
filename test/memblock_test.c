#include <memory/allocator/memblock.h>
#include <memory/memory_model.h>
#include <print.h>

#include <stdlib.h>

#define BUFFER2_SIZE (4096) // 4k
uint8_t *buffer2;

int memblock_test(void)
{
	uint8_t *tmp1;
	uint64_t *tmp2;
	phys_addr_t size, align;

	pr_info("\nmemblock allocator init\n");
	pr_info("Add xxx bytes to memblock memory region\n");
	memblock_init();

	pr_info("Add 4KB to memblock memory region\n");
	buffer2 = malloc(BUFFER2_SIZE);
	memblock_add((phys_addr_t)buffer2, BUFFER2_SIZE);

	size = 1;
	align = 1;
	pr_info("allocate memory, size %ld, align %ld\n", size, align);
	tmp1 = memblock_alloc(size, align);
	*tmp1 = 1;
	pr_info("tmp1 = 0x%x\n", *tmp1);

	size = 8;
	align = 8;
	pr_info("allocate memory, size %ld, align %ld\n", size, align);
	tmp2 = memblock_alloc(size, align);
	*tmp2 = 0x1234;
	pr_info("tmp2 = 0x%lx\n", *tmp2);

	memblock_print_all_region();

	size = 1;
	pr_info("free memory, size %ld\n", size);
	memblock_free(virt_to_phys(tmp1), size);

	size = 8;
	pr_info("free memory, size %ld\n", size);
	memblock_free(virt_to_phys(tmp2), size);

	pr_info("remove memblock memory region buffer2\n");
	memblock_remove(virt_to_phys(buffer2), BUFFER2_SIZE);

	memblock_print_all_region();

	free(buffer2);

	return 0;
}
