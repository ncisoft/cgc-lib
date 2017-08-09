#include <luaext.h>
#include <gc.h>
#include <colors.h>
#include <unistd.h>
#include <execinfo.h>
#include <stdlib.h>

// http://www.wuzesheng.com/?p=1804
void print_stacktrace()
{
    int size = 16;
    void * array[16];
    int stack_num = backtrace(array, size);
    xgc_println();
    char ** stacktrace = backtrace_symbols(array, stack_num);
    for (int i = 0; i < stack_num; ++i)
    {
        printf("%s\n", stacktrace[i]);
    }
    free(stacktrace);
    xgc_println();
}


typedef struct foo_t foo;
typedef struct foo_mt_t foo_mt;

struct foo_t
{
  int age;
  foo_mt *mt;
};

struct foo_mt_t
{
  void (*print)(foo *self);
  void (*set)(foo *self, int age);
};


void foo_print(foo *self)
{
  xgc_debug("age=%d\n", self->age);
  print_stacktrace();
}

void foo_set(foo *self, int age)
{
  self->age = age;
}

foo_mt xmt= {
  foo_print,
  foo_set
};

void mt_check_internal(void *_mt, size_t mt_sz)
{
  void ** mt = cast(void **, _mt);
  for (int i=0, n=mt_sz/sizeof(void *); i < n; mt++, i++)
    {
      if (*mt == NULL) 
	{
	  xgc_debug("i=%d\n", i);
	  xgc_assert(mt != NULL);
	}

    }
}

#define mt_check(mt) ( mt_check_internal(&mt, sizeof(mt)), (&mt)  )

foo *foo_new() 
{
  foo *o = cast(foo *, malloc(sizeof(foo)));
  o->mt = mt_check(xmt);
  return o;
}
int main()
{
  foo *o = foo_new(); 
  o->mt->set(o, 100);
  o->mt->print(o);
  printf(__C_GREEN__ "green" __C_RESET__ " \treset\n");
  printf(__C_RED__ "red" __C_RESET__ " \treset\n");
  printf(__C_YELLOW__ "yellow" __C_RESET__ " \treset\n");
  printf(__C_BLUE__ "blue" __C_RESET__ " \treset\n");
  printf(__C_MAGENTA__ "magenta" __C_RESET__ " \treset\n");
  printf(__C_CYAN__ "cyan" __C_RESET__ " \treset\n");
  return 0;
}
