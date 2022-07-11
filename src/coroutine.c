#include <coroutine.h>

# ifdef __i386__

# define JB_BX  0
# define JB_SI  1
# define JB_DI  2
# define JB_BP  3

# define JB_SP  3
# define JB_PC  2

# elif defined  __x86_64__

# define JB_SP  6
# define JB_PC  2 //7

# endif

static coroutine_t gctx;
static void __jb_start()
{

}
void call_in_stack(char *pstack, coroutine_fn fn)
{
# ifdef __i386__
  gctx.env[0].__jmpbuf[JB_SP] = cast(__jmp_buf_reg_t, pstack);
  gctx.env[0].__jmpbuf[JB_PC] = cast(__jmp_buf_reg_t, __jb_start);
# elif defined( __x86_64__) && defined ( __linux )
  // save stack pointer
  gctx.env[0].__jmpbuf[6] = cast(__jmp_buf_reg_t, pstack);
  // save function pointer, a.k.a ip
  gctx.env[0].__jmpbuf[7] = cast(__jmp_buf_reg_t, __jb_start);
#endif

}
