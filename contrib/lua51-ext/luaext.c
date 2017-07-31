#include <lualib.h>
#include <lauxlib.h>
#include <lobject.h>
#include <lstate.h>
#include <gc.h>
#include <luaext.h>
#include <unistd.h>

#define api_incr_top(L)   {api_check(L, L->top < L->ci->top); L->top++;}

// http://lua-users.org/lists/lua-l/2005-05/msg00098.html
void xgc_pushuserdata(lua_State *L, void *_pud)
{
  Udata *pud = cast(Udata *, _pud);

  setuvalue(L, L->top, pud-1);
 //  L->top++;
  api_incr_top(L);

//   xgc_debug("sizeof(UData)=%d\n", sizeof(Udata));
}
