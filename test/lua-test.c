#include <luaext.h>
#include <gc.h>
#include <unistd.h>
#include <execinfo.h>

lua_State *init()
{
  lua_State *L;

  L = luaL_newstate();
  luaL_openlibs(L);
  return L;
}

#define K_TABLE "__tbl"

// http://www.wuzesheng.com/?p=1804
void print_stacktrace()
{
    int size = 16;
    void * array[16];
    int stack_num = backtrace(array, size);
    char ** stacktrace = backtrace_symbols(array, stack_num);
    for (int i = 0; i < stack_num; ++i)
    {
        printf("%s\n", stacktrace[i]);
    }
    free(stacktrace);
}

void test(lua_State *L)
{
  void *p;
  lua_newtable(L);
  lua_setglobal(L, K_TABLE);

  lua_getglobal(L, K_TABLE);
  p = lua_newuserdata(L, 48);
  xgc_debug("pud = %p\n", p);
  lua_pushboolean(L, true);
  lua_rawset(L, -3);

  xgc_pushuserdata(L, p);
  lua_rawget(L, -2);
  xgc_debug( "type=%d\n", lua_type(L, (-1))) ;
  xgc_assert( lua_isboolean(L, -1) );
  print_stacktrace();


}

int main()
{
  lua_State *L = init();
  test(L);
  return 0;
}
