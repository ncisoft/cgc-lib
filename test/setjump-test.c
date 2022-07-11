#include <stdio.h>
#include <setjmp.h>
#include <coroutine.h>
//#include <call_in_stack.h>
#include <gc.h>
#undef GC_LOG_BRIEF
#define GC_LOG_BRIEF false

static jmp_buf buf;

void second(void) {
  xgc_debug("second\n");
  longjmp(buf,1);             // 跳回setjmp的调用处 - 使得setjmp返回值为1
}

void first(void) {
    second();
  xgc_debug("first\n");
}

extern void check_jumpbuf_nextpc();
void check_jmpbuf()
{
#if defined (__linux__)
  jmp_buf xbuf;
  int x=222;

   if ( ! setjmp(xbuf))
     {
       for (int i=0; i < sizeof(xbuf[0].__jmpbuf)/sizeof(__jmp_buf_reg_t); i++)
	 xgc_debug("[%d] %p\n", i, cast(void *, cast(void*, xbuf[0].__jmpbuf[i])));
       printf("\n");
       xgc_debug("sp = %p, %p\n", &xbuf, &x);
       xgc_debug("pc = %p, next pc=%p\n", &check_jmpbuf, &check_jumpbuf_nextpc);
       xgc_debug("pc = %p, next pc=%p\n", &check_jmpbuf, &&__next_pc);
       printf("\n");
     }
   else
    {
      xgc_debug("main, x = %d\n",x);
    }
__next_pc:
   xgc_info("reg_t sizeof=%u, sizeof_uint(int)=%u, sizeof_uint(long)=%u\n", sizeof_uint(xbuf[0].__jmpbuf[0]), sizeof_uint(int), sizeof_uint(long));
   xgc_info("__WORDSIZE = %d\n", __WORDSIZE );
   xgc_info("__sizeof(long long) = %u\n", sizeof_uint(long long) );
#endif
}

void check_jumpbuf_nextpc()
{

}
int main()
{
  volatile int x=33;
  void *p = cast(void*, &main);
  char *cp = cast(char*, &main);
  xgc_info("%p, %p\n", p, cp+10);
  xgc_info("%d\n", cast(int, sizeof(__jmp_buf_reg_t)));
  check_jmpbuf();
  if ( ! setjmp(buf))
    {
      first();
    }
  else
    {
      xgc_debug("main, x = %d\n",x);
    }
  return 0;

}
