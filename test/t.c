#include <snippets/tt.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>

int main()
{
  while (1)
    {
      int fd=open("", 0);
      assert(fd >= 0);
    }
  printf("%s", tt);
}
