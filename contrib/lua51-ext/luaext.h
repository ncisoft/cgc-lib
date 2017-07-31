#ifndef __LUAEXT_H_
#define __LUAEXT_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#include <unistd.h>

extern void xgc_pushuserdata(lua_State *L, void *_pud);

#ifdef __cplusplus
}
#endif

#endif
