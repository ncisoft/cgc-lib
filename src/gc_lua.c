#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#include <luaext.h>
#include <gc.h>
#include <unistd.h>


typedef struct {
  lua_State *L;

} gc_heap_t;

#define K_ALIVEOBJ_MAP "__alive_objects_map"   // {obj->{objs}}
#define K_ALIVEROOT_MAP "__alive_root_map" // {root->{}, root.hex->root}
#define MT_ROOT "__mt_root"
#define MT_MANAGED_OBJ "__mt_managed_ob"


typedef struct
{
  void *p;
  fobj_gc _gc;
} xgc_obj_t;

/*--------- static objects -------------*/
 int __gc_log_level = GC_LOGLEVEL_DEBUG;
static gc_heap_t __gc_heap = { NULL };
static gc_root_t __gc_roots = { NULL };

/*--------- static functions -------------*/
static gc_heap_t *gc_heap_t_init();
static gc_root_t *gc_root_new_with_param(bool  do_defer); 
static int __root_gc(lua_State* L);
static int __obj_gc(lua_State* L);

gc_heap_t  *gc_heap_t_init()
{
  gc_heap_t *pgch = &__gc_heap;

  if (!pgch->L)
    {
      pgch->L = luaL_newstate();
      int _top = lua_gettop(pgch->L);

      luaL_openlibs(pgch->L);
      lua_newtable(pgch->L);
      lua_setglobal(pgch->L, K_ALIVEOBJ_MAP);
      xgc_assert( _top+0 == lua_gettop(pgch->L) );
      lua_newtable(pgch->L);
      lua_setglobal(pgch->L, K_ALIVEROOT_MAP);
      xgc_assert( _top+0 == lua_gettop(pgch->L) );


      // http://t.cn/R7dAiQd
      luaL_newmetatable(pgch->L, MT_ROOT);
      lua_pushstring(pgch->L, "__gc");
      lua_pushcfunction(pgch->L, __root_gc);
      lua_rawset(pgch->L, -3);
      lua_pop(pgch->L, 1); // pop the metatable
      xgc_assert( _top+0 == lua_gettop(pgch->L) );

      xgc_assert( luaL_newmetatable(pgch->L, MT_MANAGED_OBJ) == 1);
      lua_pushstring(pgch->L, "__gc");
      lua_pushcfunction(pgch->L, __obj_gc);
      lua_rawset(pgch->L, -3);
      lua_pop(pgch->L, 1); // pop the metatable

      xgc_debug("init gc_heap auccessful\n");
      xgc_assert( _top+0 == lua_gettop(pgch->L) );
    }
  return pgch;
}

int __root_gc(lua_State* L) 
{
  gc_root_t *proot = cast(gc_root_t *, lua_touserdata(L, -1));
  xgc_info("gc_root was released %p\n", proot);
  return 0;
}

int __obj_gc(lua_State* L) 
{
  xgc_obj_t *xobj = cast(gc_root_t *, lua_touserdata(L, -1));
  xgc_info("release xobj=%p\n", xobj);
  return 0;
}

gc_root_t *gc_root_new_with_param(bool  is_primitive) 
{
  gc_heap_t *pgch = gc_heap_t_init();
  lua_State *L = pgch->L;
  int _top = lua_gettop(L);
  gc_root_t *proot;

  // step(1): get _G.K_ALIVEROOT_MAP
    {
      lua_getglobal(pgch->L, K_ALIVEROOT_MAP);
      assert( lua_istable(pgch->L, -1) );
      xgc_assert( _top+1 == lua_gettop(L) );
      // stack layout: _G.K_ALIVEROOT_MAP
    }


  // step(2): create proot and set metatable
    {
      // stack layout: _G.K_ALIVEROOT_MAP
      xgc_assert( _top+1 == lua_gettop(L) );

      proot = cast(gc_root_t*,  lua_newuserdata(pgch->L, sizeof(gc_root_t)));
      proot->is_primitive = is_primitive;
      proot->next = __gc_roots.next;
      __gc_roots.next = proot;
      xgc_assert ( lua_isuserdata(pgch->L, -1) ); 
      // stack layout: _G.K_ALIVEROOT_MAP proot
      xgc_assert( _top+2 == lua_gettop(L) );
      luaL_getmetatable(L, MT_ROOT);
      xgc_assert ( lua_istable(pgch->L, -1) ); 
      xgc_debug("\t top = %d\n", lua_gettop(L));
      lua_setmetatable(L, -2);
      xgc_debug("\t top = %d\n", lua_gettop(L));
      // stack layout: _G.K_ALIVEROOT_MAP proot
      xgc_assert( _top+2 == lua_gettop(L) );
    }

  // step(3): set _G.K_ALIVEROOT_MAP[proot] = {}
    {
      // stack layout: _G.K_ALIVEROOT_MAP proot
      xgc_assert( _top+2 == lua_gettop(L) );

      lua_newtable(pgch->L);
      // stack layout: _G.K_ALIVEROOT_MAP proot {}
      xgc_assert( _top+3 == lua_gettop(L) );
      xgc_debug("\t top = %d\n", lua_gettop(L));
      xgc_assert ( lua_istable(pgch->L, -1) ); 
      xgc_assert ( lua_isuserdata(pgch->L, -2) ); 
      xgc_assert ( lua_istable(pgch->L, -3) ); 
      lua_rawset(pgch->L, -3);       
      // stack layout: _G.K_ALIVEROOT_MAP
      xgc_assert( _top+1 == lua_gettop(L) );
      lua_pop(L, 1);
    }


  xgc_debug("actual proot = %p\n", proot);
  return proot;

}
gc_root_t *gc_root_new(int is_primitive) 
{
  return gc_root_new_with_param(true);
}

gc_root_t *gc_root_new_with_complex_return() 
{
  return gc_root_new_with_param(false);

}

void *gc_malloc(gc_root_t *proot, size_t sz) 
{
  return gc_malloc_with_gc(proot, sz, NULL);
}


#define K_HEXOBJ_MAP ""
extern void *gc_malloc_with_gc(gc_root_t *proot, size_t sz, fobj_gc _gc)
{
  xgc_assert(proot != NULL);
  lua_State* L = gc_heap_t_init()->L;
  int _top = lua_gettop(L);
  xgc_obj_t *obj;

  // step(1): get _G.K_ALIVEROOT_MAP proot
    {
      lua_getglobal(L, K_ALIVEROOT_MAP);
      xgc_pushuserdata(L, proot);
      xgc_assert( lua_isuserdata(L, -1) );
      lua_rawget(L, -2);
      xgc_debug( "type=%d\n", lua_type(L, (-1))) ;
      xgc_assert( lua_istable(L, -1) );
      // stack _G.K_ALIVEROOT_MAP proot
    }
  // step(2): create object & set metatable
    {
      // stack layout[1]: _G.K_ALIVEROOT_MAP _G.K_ALIVEROOT_MAP[proot]
      xgc_assert(_top+2 == lua_gettop(L));

      void *p = lua_newuserdata(L, sz + sizeof(xgc_obj_t));
      obj = cast(xgc_obj_t*, p);
      xgc_assert(obj != NULL);
      obj->_gc = _gc;
      obj->p = cast(void *, obj+1);
      // stack layout[1]: _G.K_ALIVEROOT_MAP _G.K_ALIVEROOT_MAP[proot] xobj
      xgc_assert(_top+3 == lua_gettop(L));

      luaL_getmetatable(L, MT_MANAGED_OBJ);
      xgc_assert(_top+4 == lua_gettop(L));
      lua_setmetatable(L, -2);
      xgc_assert(_top+3 == lua_gettop(L));
    }
  // step(4): 
    {
      // stack layout[1]: __alive_root_map __alive_root_map[proot] xobj
      xgc_assert(_top+3 == lua_gettop(L));

      lua_pushinteger(L, 1);
      lua_rawset(L, -3);
      xgc_debug("\t top = %d\n", lua_gettop(L));
      xgc_assert(_top+2 == lua_gettop(L));
      lua_pop(L, 2);

    }

  xgc_assert(_top+0 == lua_gettop(L));
  xgc_debug("maloc xobj=%p, p=%p \n", obj, obj->p);
  return obj->p;
}

extern void gc_mark_ref(void *source_ptr, void *ddest_ptr)
{

}
extern void gc_mark_unref(void *source_ptr, void *ddest_ptr)
{

}

void gc_collect()
{
  gc_heap_t *pgch =  gc_heap_t_init();
  lua_State* L = pgch->L;
  int _top = lua_gettop(L);
  //  lua_gc(pgch->L, LUA_GCSTEP, 1024);

  lua_getglobal(L, K_ALIVEROOT_MAP);
  for (gc_root_t *prev=&__gc_roots, *proot = __gc_roots.next; proot != NULL; proot = prev->next)
    {
      if (proot->is_primitive)
	{
	  prev->next = proot->next;
	  xgc_info("will collect proot = %p\n", proot);

	  xgc_pushuserdata(L, proot);
	  lua_pushnil(L);
	  lua_rawset(L, -3);
	  xgc_assert(_top+1 == lua_gettop(L));
	}
      else
	{
	  prev = proot;

	}
    }
  lua_pop(L, 1);
  xgc_debug("start gc process ...\n");
  lua_gc(pgch->L, LUA_GCCOLLECT, 0);


}
