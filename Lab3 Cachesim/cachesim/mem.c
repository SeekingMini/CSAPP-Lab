#include <string.h>
#include "common.h"

static uint8_t mem[MEM_SIZE];      // 每个元素占1B，mem[32768]
static uint8_t mem_diff[MEM_SIZE]; // 每个元素占1B，mem_diff[32768]

/*
 * 初始化内存（在内存中添加内容）
 */
void init_mem(void)
{
  int i;
  for (i = 0; i < MEM_SIZE; i++)
  {
    mem[i] = rand() & 0xff; // mem[i]在[0, 255]之间
  }

  memcpy(mem_diff, mem, MEM_SIZE); // mem_diff为mem的一个副本
}

/*
 * 从内存数组中读取指定块到buf数组
 */
void mem_read(uintptr_t block_num, uint8_t *buf)
{
  memcpy(buf, mem + (block_num << BLOCK_WIDTH), BLOCK_SIZE);
  cycle_increase(6);
#ifdef DEBUG
  printf("mem_read\n");
#endif
}

void mem_write(uintptr_t block_num, const uint8_t *buf)
{
  memcpy(mem + (block_num << BLOCK_WIDTH), buf, BLOCK_SIZE);
  cycle_increase(25);
#ifdef DEBUG
  printf("mem_write\n");
#endif
}

uint32_t mem_uncache_read(uintptr_t addr)
{
  uint32_t *p = (void *)mem_diff + (addr & ~0x3); // (void *)是一个字节一个字节偏移，算式(addr & ~0x3)是保证*p不会溢出
  uncache_increase(6);
#ifdef DEBUG
  printf("mem_uncache_read\n");
#endif
  return *p;
}

void mem_uncache_write(uintptr_t addr, uint32_t data, uint32_t wmask)
{
  uint32_t *p = (void *)mem_diff + (addr & ~0x3);
  *p = (*p & ~wmask) | (data & wmask);
  uncache_increase(25);
#ifdef DEBUG
  printf("mem_uncache_write\n");
#endif
}
