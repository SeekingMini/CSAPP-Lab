#include "common.h"
#include <inttypes.h>
#include <time.h>

uint32_t cpu_read(uintptr_t addr, int len);
void cpu_write(uintptr_t addr, int len, uint32_t data);
uint32_t cpu_uncache_read(uintptr_t addr, int len);
void cpu_uncache_write(uintptr_t addr, int len, uint32_t data);

void init_mem(void);
void init_cache(int total_size_width, int associativity_width);

void init_rand(uint32_t seed)
{
  printf("random seed = %u\n", seed);
  srand(seed);
}

static inline uint32_t choose(uint32_t n) { return rand() % n; }

static uint64_t cycle_cnt = 0, uncached_cnt = 0;
static uint64_t total_try = 0, total_hit = 0;

void cycle_increase(int n) { cycle_cnt += n; }
void uncache_increase(int n) { uncached_cnt += n; }
void try_increase(int n) { total_try += n; }
void hit_increase(int n) { total_hit += n; }

#define TEST_LOOP 1000000

void random_trace(void)
{
  const int choose_len[] = {1, 2, 4}; // len的含义是什么？

  int i;
  for (i = 0; i < TEST_LOOP; i++)
  {
    int len = choose_len[choose(sizeof(choose_len) / sizeof(choose_len[0]))]; // 随机在[1, 2, 4]中选择一个
    uintptr_t addr = choose(MEM_SIZE) & ~(len - 1);                           // (1)len=1，取高16位为有效位 (2)len=2，取高15位为有效位 (3)len=4，取高14位为有效位
    bool is_read = choose(2);                                                 // 取0或1
#ifdef DEBUG
    printf("loop %d\n", i);
#endif
    if (is_read) // is_read = 1，即命中
    {
      uint32_t ret = cpu_read(addr, len);                 // 通过cache读取内容
      uint32_t ret_uncache = cpu_uncache_read(addr, len); // 直接从内从读取内容
      assert(ret == ret_uncache);                         // 对比读接口的结果
    }
    else // is_read = 0，即未命中
    {
      uint32_t data = rand();
      cpu_write(addr, len, data);
      cpu_uncache_write(addr, len, data);
    }
  }
}

void check_diff(void)
{
  uintptr_t addr = 0;
  for (addr = 0; addr < MEM_SIZE; addr += 4)
  {
    uint32_t ret = cpu_read(addr, 4);
    uint32_t ret_uncache = cpu_uncache_read(addr, 4);
    assert(ret == ret_uncache);
  }
}

int main(int argc, char *argv[])
{
  uint32_t seed;
  if (argc >= 2)
  {
    char *p;
    seed = strtol(argv[1], &p, 0);
    if (!(*argv[1] != '\0' && *p == '\0'))
    {
      printf("invalid seed\n");
      seed = time(0);
    }
  }
  else
  {
    seed = time(0);
  }

  init_rand(seed);
  printf("-------------------------\n");
  init_mem(); // 初始化mem数组

  init_cache(14, 2); // 初始化一个16KB，4路组相联的cache

  random_trace(); // 读取trace

  printf("cached cycle = %" PRId64 "\n", cycle_cnt);
  printf("uncached cycle = %" PRId64 "\n", uncached_cnt);
  printf("cycle ratio = %0.2f %%\n", ((float)cycle_cnt / uncached_cnt) * 100.0f);
  printf("total access = %" PRId64 "\n", total_try);
  printf("cache hit = %" PRId64 "\n", total_hit);
  printf("hit rate = %0.2f %%\n", ((float)total_hit / total_try) * 100.0f);
  printf("-------------------------\n");

  check_diff();

  printf("Random test pass!\n");
  printf("费振环你是最棒的！！！\n");

  return 0;
}
