#include <stdio.h>
#include <execinfo.h>

void trace()
{
  int size = 16;
  void * array[16];
  int stack_num = backtrace(array, size);
  char ** stacktrace = backtrace_symbols(array, stack_num);
  for (int i = 0; i < stack_num; ++i)
    {
      printf("  %s\n", stacktrace[i]);
    }
  printf("%p\n", &size);
}

int main()
{
 trace();
}
