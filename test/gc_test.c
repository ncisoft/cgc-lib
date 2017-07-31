#include <gc.h>
#include <stdio.h>

//#define cast(type, p) ((type)(p))
void test()
{
  gc_root_t *proot= gc_root_new();
  xgc_debug("proot=%p\n", proot);
  xgc_debug("sizeof(int)=%d, sizeof(void *)=%d\n", sizeof(int), sizeof(void *));
    {

  void *p = gc_malloc(proot, 48);
  printf("proot=%p, p=%p\n", proot, p);
    }

}
int main()
{
  test();
  gc_collect();
  return 0;
}

