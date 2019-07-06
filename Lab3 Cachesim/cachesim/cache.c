#include "common.h"

void mem_read(uintptr_t block_num, uint8_t *buf);
void mem_write(uintptr_t block_num, const uint8_t *buf);

// TODO: implement the following functions

typedef struct
{
	bool is_valid;		// 有效位
	bool is_dirty;		// 脏位
	uint32_t tag;		// 标记
	uint32_t block_num; // 主存块号
	uint8_t block[64];  // 块
} cache_line;
cache_line cache[64][4]; // 根据计算，cache有2^6组，每组2^2行，一共2^8行

/*
 * 从cache中读取内容
 */
uint32_t cache_read(uintptr_t addr)
{
	try_increase(1);
	uint32_t *p = NULL;

	uint32_t group_num = (addr >> 6) % 64;
	bool is_hit = false;

	for (int i = 0; i < 4; i++)
	{
		if (cache[group_num][i].tag == (addr >> 12) && cache[group_num][i].is_valid == 1)
		{
			// 命中
			hit_increase(1);
			is_hit = true;
			p = (void *)cache[group_num][i].block + ((addr & 0x3f) & 0x3c);
			break;
		}
	}
	if (is_hit == false) // 未命中
	{
		uint8_t buf[64];
		bool is_full = true; // 假设cache的该组中没有空闲行
		for (int i = 0; i < 4; i++)
		{
			if (cache[group_num][i].is_valid == 0) // 找到空闲行
			{
				is_full = false;
				cache[group_num][i].is_valid = 1;
				cache[group_num][i].tag = addr >> 12;
				cache[group_num][i].block_num = (addr >> 6);
				mem_read(addr >> 6, buf);
				memcpy(cache[group_num][i].block, buf, BLOCK_SIZE);
				p = (void *)buf + ((addr & 0x3f) & 0x3c);
				break;
			}
		}
		if (is_full == true) // 没有空闲行
		{
			int ran_row = rand() % 4;
			if (cache[group_num][ran_row].is_dirty == 0) // 脏位为0，不需要把cache内容写回到主存
			{
				cache[group_num][ran_row].is_valid = 1;
				cache[group_num][ran_row].tag = addr >> 12;
				cache[group_num][ran_row].block_num = (addr >> 6);
				mem_read(addr >> 6, buf);
				memcpy(cache[group_num][ran_row].block, buf, BLOCK_SIZE);
				p = (void *)buf + ((addr & 0x3f) & 0x3c);
			}
			else // 脏位为1，先把cache内容写回主存再进行替换
			{
				memcpy(buf, cache[group_num][ran_row].block, BLOCK_SIZE);
				mem_write(cache[group_num][ran_row].block_num, buf);

				cache[group_num][ran_row].is_valid = 1;
				cache[group_num][ran_row].is_dirty = 0;
				cache[group_num][ran_row].tag = addr >> 12;
				cache[group_num][ran_row].block_num = (addr >> 6);
				mem_read(addr >> 6, buf);
				memcpy(cache[group_num][ran_row].block, buf, BLOCK_SIZE);
				p = (void *)buf + ((addr & 0x3f) & 0x3c);
			}
		}
	}

	return *p;
}

/*
 * 把内容写入cache（回写法）
 */
void cache_write(uintptr_t addr, uint32_t data, uint32_t wmask)
{
	try_increase(1);
	uint32_t group_num = (addr >> 6) % 64;
	bool is_hit = false;

	for (int i = 0; i < 4; i++)
	{
		if (cache[group_num][i].tag == (addr >> 12) && cache[group_num][i].is_valid == 1)
		{
			// 命中
			hit_increase(1);
			is_hit = true;
			// 直接修改cache
			uint32_t *p = (void *)cache[group_num][i].block + ((addr & 0x3f) & 0x3c);
			*p = (*p & ~wmask) | (data & wmask);
			cache[group_num][i].is_dirty = 1; // 脏位变为1
			break;
		}
	}
	if (is_hit == false) // 没有命中
	{
		uint8_t buf[64];
		bool is_full = true; // 假设cache的该组中没有空闲行
		for (int i = 0; i < 4; i++)
		{
			if (cache[group_num][i].is_valid == 0) // 找到空闲行，先更新主存内容，再更新cache内容
			{
				is_full = false;
				// 先更新主存内容
				mem_read(addr >> 6, buf);
				uint32_t *p = (void *)buf + ((addr & 0x3f) & 0x3c);
				*p = (*p & ~wmask) | (data & wmask);
				mem_write(addr >> 6, buf);
				// 再更新cache
				cache[group_num][i].is_valid = 1;
				cache[group_num][i].is_dirty = 0;
				cache[group_num][i].tag = addr >> 12;
				cache[group_num][i].block_num = addr >> 6;
				memcpy(cache[group_num][i].block, buf, 64);
				break;
			}
		}
		if (is_full == true)
		{
			int ran_row = rand() % 4;
			if (cache[group_num][ran_row].is_dirty == 0) // 脏位为0，不需要把cache内容写回到内存
			{
				cache[group_num][ran_row].is_valid = 1;
				cache[group_num][ran_row].tag = addr >> 12;
				cache[group_num][ran_row].block_num = (addr >> 6);
				// 先更新主存内容
				mem_read(addr >> 6, buf);
				uint32_t *p = (void *)buf + ((addr & 0x3f) & 0x3c);
				*p = (*p & ~wmask) | (data & wmask);
				mem_write(addr >> 6, buf);
				// 再更新cache
				memcpy(cache[group_num][ran_row].block, buf, 64);
			}
			else // 脏位为1，需要先把cache内容写回到内存
			{
				// 先把cache内容写回到内存
				memcpy(buf, cache[group_num][ran_row].block, BLOCK_SIZE);
				mem_write(cache[group_num][ran_row].block_num, buf);

				cache[group_num][ran_row].is_valid = 1;
				cache[group_num][ran_row].is_dirty = 0;
				cache[group_num][ran_row].tag = addr >> 12;
				cache[group_num][ran_row].block_num = (addr >> 6);
				// 先更新主存内容
				mem_read(addr >> 6, buf);
				uint32_t *p = (void *)buf + ((addr & 0x3f) & 0x3c);
				*p = (*p & ~wmask) | (data & wmask);
				mem_write(addr >> 6, buf);
				// 再更新cache
				memcpy(cache[group_num][ran_row].block, buf, 64);
			}
		}
	}
}

/*
 * 初始化cache
 */
void init_cache(int total_size_width, int associativity_width)
{
	int group_num = ((1 << total_size_width) / (1 << BLOCK_WIDTH)) / (1 << associativity_width);
	// 初始化cache数组
	for (int g = 0; g < group_num; g++)
	{
		for (int r = 0; r < (1 << associativity_width); r++)
		{
			cache[g][r].is_valid = 0;
			cache[g][r].is_dirty = 0;
			cache[g][r].tag = 0;
			cache[g][r].block_num = 0;
			memset(cache[g][r].block, 0, sizeof(cache[g][r].block));
		}
	}
}
