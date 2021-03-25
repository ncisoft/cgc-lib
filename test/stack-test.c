#include <stdio.h>
#define __STDC_LIMIT_MACROS
#include <stdint.h>
#include <limits.h>


#if INTPTR_MAX == INT64_MAX
typedef int64_t m_int_t;
typedef uint64_t m_uint_t;
#define BITS 64
#elif INTPTR_MAX == INT32_MAX
typedef int32_t m_int_t;
typedef uint32_t m_uint_t;
#define BITS 32
#endif

#define  MAGIC_NUM 0x12345

void foo(int i);
void foo2(void *p)
{
  m_int_t padding=0x7777;
  m_int_t *pstack = &padding;

  printf("%-10s = %p %p\n", "foo2.stack", pstack-1, p);
  for (int i=1; i < 68; i++)
    {
      if (pstack[i] == MAGIC_NUM)
        {
          printf("%d matched %p\n", i,pstack+i);
        }
    }

}

void foo(int i)
{
  m_int_t padding = MAGIC_NUM;
  void *pstack = &padding;

  printf("%-10s = %p\n", "foo.stack", pstack);
  foo2(pstack);
}

#define sizeof_int(o) (int)sizeof(o)

int main()
{
  foo(0);

  printf("\n");
  printf("%-17s = %d\n", "sizeof(int)", sizeof_int(int));
  printf("%-17s = %d\n", "sizeof(long)", sizeof_int(long));
  printf("%-17s = %d\n", "sizeof(long long)", sizeof_int(long long));
  printf("%-17s = %d\n", "sizeof(int64_t)", sizeof_int(int64_t));
  printf("%-17s = %d\n", "sizeof(void *)", sizeof_int(void*));
  printf("is %dbit\n",BITS);
  return 0;
}
