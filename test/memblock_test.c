#include <memory/allocator/memblock.h>

#define BUFFER1_SIZE (4096 * 1024) // 4M
#define BUFFER2_SIZE (4096) // 4k

int memblock_test(void)
{
	u_int8_t *buffer1, *buffer2;

	buffer1 = malloc(BUFFER1_SIZE);
	buffer2 = malloc(BUFFER2_SIZE);

	memblock_add((phys_addr_t)buffer1, BUFFER1_SIZE);
	memblock_add((phys_addr_t)buffer2, BUFFER2_SIZE);

	u_int8_t *tmp1 = memblock_alloc(1, 1);
	*tmp1 = 1;
	pr_debug("0x%x\n", *tmp1);

	u64 *tmp2 = memblock_alloc(8, 8);
	*tmp2 = 0x1234;
	pr_debug("0x%llx\n", *tmp2);

	memblock_print_all_region();

	memblock_free(virt_to_phys(tmp1), 1);
	memblock_free(virt_to_phys(tmp2), 8);

	memblock_remove(virt_to_phys(buffer1), BUFFER1_SIZE);
	memblock_remove(virt_to_phys(buffer2), BUFFER2_SIZE);

	memblock_print_all_region();

	free(buffer1);
	free(buffer2);

	return 0;
}