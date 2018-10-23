#ifndef _COROUTINE_H_
#define _COROUTINE_H_

#include <stdint.h>
#include <setjmp.h>

#ifdef __cplusplus
extern "C"
{
#endif
// header here...
#ifndef cast
#define cast(type, expr) ((type)(expr))
#endif

  typedef void (*coroutine_fn)();
# if __WORDSIZE == 64
  typedef uint64_t  __jmp_buf_reg_t;
//#warning "__WORDSIZE == 64, using uint64_t"

# elif __WORDSIZE == 32
  typedef uint32_t __jmp_buf_reg_t;
#warning "__WORDSIZE == 32, using uint32_t"

 
# elif defined  __x86_64__ 
  typedef uint64_t__jmp_buf_reg_t;
#warning "64-2"

#else
  typedef uint32_t __jmp_buf_reg_t;
#error "111"

#endif

  typedef struct coroutine_t_
    {
      char *pstack;
      jmp_buf env;

    } coroutine_t;

extern void call_in_stack(char *pstack, coroutine_fn fn);

#ifdef __cplusplus
}

#endif
#endif
