#include <gc.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

static int masks[100] = {0};

typedef  struct
{
  int id;
} foo_t;

void xmask_set(int index, bool isUsed)
{
  xgc_assert(index > 0 && index < sizeof(masks)/sizeof(int));
  masks[index] = (isUsed) ? 1 : 0;
}

// check obj is alive
bool xmask_check(int index)
{
  xgc_assert(index > 0 && index < sizeof(masks)/sizeof(int));
  return (masks[index] != 0) ? true :false;
}

void obj_gc(void *p)
{
  int index = atoi( cast(const char *, p) );
  xmask_set( index, false );
  xgc_info("**release obj= %s, p=%p\n", cast(char*, p), p);
}

void* obj_new(gc_root_t *proot, int id)
{
  void *p = gc_malloc_with_gc(proot, 48, obj_gc);
  char buf[64];
  sprintf(buf, "%d", id);
  strcpy(cast(char *, p), buf);
  xmask_set(id, true);
  return p;
}

foo_t* test_0F_01(void *p30)
{
  using_raii_proot_complex_return()
    {
      void *p31 = obj_new(proot, 31);
      gc_mark_ref(p30, p31);
      gc_mark_unref(p30, p31);
    }
  return  NULL;
}

foo_t* test_0F_02(void *p30)
{
  using_raii_proot_complex_return()
    {
      void *p31 = obj_new(proot, 31);
      gc_mark_ref(p30, p31);
      gc_mark_unref(p30, p31);
    }
  return  NULL;
}

uint32_t * test_0F_03(void *p30)
{
  using_raii_proot_complex_return()
    {
      void *p31 = obj_new(proot, 31);
      gc_mark_ref(p30, p31);
      gc_mark_unref(p30, p31);
    }
  return  NULL;
}

void testcase_0E()
{
  using_raii_proot()
    {
      void *p30 = obj_new(proot, 30);
      dismiss_unused(p30);

    }
}


void *test_0D_01(void *p20)
{
  using_raii_proot_complex_return()
    {
      void *p21 = obj_new(proot, 21);
      gc_mark_ref(p20, p21);
      return p21;
    }
  return NULL;
}

void testcase_0D()
{
 using_raii_proot()
   {
     void *p20 = obj_new(proot, 20);
     void *p21 = test_0D_01(p20);
     xgc_assert( xmask_check(20) == true );
     xgc_assert( xmask_check(21) == true );
     xgc_assert( p21 != NULL );
   }
}

void *test_0C_01()
{
  using_raii_proot_complex_return()
    {
      void *p10 = obj_new(proot, 10);
      return p10;
    }
  return NULL;
}

void testcase_0C()
{
  using_raii_proot()
    {
      void *p8 = gc_malloc(proot, 8);
      void *p10 = test_0C_01();
      xgc_info("===\n");
      void *p11 = obj_new(proot, 11);
      xgc_info("----\n");
      gc_mark_ref(p11, p10);
      xgc_assert( xmask_check(10) == true );
      xgc_assert( xmask_check(11) == true );
      dismiss_unused(p8);
    }
}

void test_0B_01(void *p3)
{
 using_raii_proot()
   {
     void *p4 = obj_new(proot, 4);
     gc_mark_ref(p3, p4);
     void *p5 = obj_new(proot, 5);
     xgc_assert( p5 != NULL );
   }
}


void testcase_0B()
{
  using_raii_proot()
    {
      void *p3 = obj_new(proot, 3);
      test_0B_01(p3);
      xgc_assert( xmask_check(3) == true );
      xgc_assert( xmask_check(4) == true );
      xgc_assert( xmask_check(5) == false);
    }
}

void testcase_0A()
{
  using_raii_proot()
    {

      xgc_debug("proot=%p\n", proot);
      xgc_debug("sizeof(int)=%u, sizeof(void *)=%u\n", sizeof_uint(int), sizeof_uint(void *));
        {

          void *p1 = obj_new(proot, 1);
          void *p2 = obj_new(proot, 2);

          gc_mark_ref(p1, p2);
          xgc_debug("**p=%p, **p2=%p\n",  p1, p2);
        }
   }
}

void testcase_exit()
{
 using_raii_proot()
   {
      xgc_assert(proot != NULL);
  }
}

int main()
{
 using_raii_proot()
    {
      xgc_info("[start] testcase_0A ...\n");
      testcase_0A();
step_assert_testcase_0A:
      xgc_assert( xmask_check(1) == false );
      xgc_assert( xmask_check(2) == false );
      xgc_info("[pass]  testcase_0A\n");
step_assert_testcase_0B:
      xgc_info("[start] testcase_0B ...\n");
      testcase_0B();
      xgc_assert( xmask_check(3) == false );
      xgc_assert( xmask_check(4) == false );
      xgc_assert( xmask_check(5) == false );
          xgc_info("[pass]  testcase_0B\n");
step_assert_testcase_0C:
      if (1)
        {
          xgc_info("[start] testcase_0C ...\n");
          testcase_0C();
          xgc_assert( xmask_check(10) == false );
          xgc_assert( xmask_check(11) == false );
          xgc_info("[pass]  testcase_0C\n");
        }
step_assert_testcase_0D:
      if (1)
        {
          xgc_info("[start] testcase_0D ...\n");
          testcase_0D();
          xgc_assert( xmask_check(20) == false );
          xgc_assert( xmask_check(21) == false );
          xgc_info("[pass]  testcase_0D\n");
        }
step_assert_testcase_0E:
      if (1)
        {
          xgc_info("[start] testcase_0E ...\n");
          testcase_0E();
          xgc_assert( xmask_check(30) == false );
          xgc_assert( xmask_check(31) == false );
          for (int i=1; i < sizeof(masks)/sizeof(int); i++)
            xgc_assert( xmask_check(i) == false );
          xgc_debug("all xmasks were clean\n")

          xgc_info("[pass]  testcase_0E\n");
            gc_collect();

        }
      return 0;
    }
}

