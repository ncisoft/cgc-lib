#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#include <luaext.h>
#include <gc.h>
#include <unistd.h>
#include <string.h>
#include <execinfo.h>


typedef struct {
  lua_State *L;

} gc_heap_t;

#define K_ALIVEROOT_MAP "__alive_root_map" // {root->{}, root.hex->root}
#define MT_ROOT "__mt_root"
#define MT_MANAGED_OBJ "__mt_managed_ob"

#define luat_true 1
#define luat_false 0

enum {
  MASK_NONE = 0,
  MASK_INITIAL_CLOSED = 1,
  MASK_SUBSEQUENT_CLOSED=2
};

typedef struct
{
  void *p;
  gc_root_t *proot;
  fobj_gc _gc;
} xgc_obj_t;

/*--------- static objects -------------*/
 int __gc_log_level = GC_LOGLEVEL_DEBUG;
static gc_heap_t __gc_heap = { NULL };
static gc_root_t __gc_roots = { NULL };

/*--------- static functions -------------*/
static gc_heap_t *gc_heap_t_init();
static gc_root_t *gc_root_new_with_param(bool  is_primitive, char *func_name, char *parent_func_name); 
static int __root_gc(lua_State* L);
static int __obj_gc(lua_State* L);
static gc_root_t *__get_my_function_by_backtrace(int iLevel, gc_root_t *self_proot);

void xgc_print_stacktrace(char *fname, int lineno)
{
    int size = 16;
    void * array[16];
    int stack_num = backtrace(array, size);

    xgc_debug("backstrace: %s:%d ...\n", fname, lineno);

    char ** stacktrace = backtrace_symbols(array, stack_num);
    for (int i = 0; i < stack_num; ++i)
    {
        printf(__C_MAGENTA__);
        printf("\t%s\n", stacktrace[i]);
        printf(__C_RESET__);
    }
    free(stacktrace);
    //xgc_println();
}


gc_heap_t  *gc_heap_t_init()
{
  gc_heap_t *pgch = &__gc_heap;

  if (!pgch->L)
    {
      pgch->L = luaL_newstate();
      int _top = lua_gettop(pgch->L);

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

      xgc_info("init gc_heap auccessful\n");
      xgc_assert( _top+0 == lua_gettop(pgch->L) );
    }
  return pgch;
}

int __root_gc(lua_State* L) 
{
  gc_root_t *proot = cast(gc_root_t *, lua_touserdata(L, -1));
  xgc_debug("release gc_root=%p func=%s\n", proot, proot->my_function_name);
  return 0;
}

int __obj_gc(lua_State* L) 
{
  xgc_obj_t *xobj = cast(xgc_obj_t *, lua_touserdata(L, -1));
  xgc_debug("  release _p=%p, xobj=%p\n", xobj+1,xobj);
  if (xobj->_gc)
    xobj->_gc(xobj->p);
  return 0;
}

gc_root_t *gc_root_new_with_param(bool  is_primitive, 
				  char *func_name, char *parent_func_name) 
{
  gc_heap_t *pgch = gc_heap_t_init();
  lua_State *L = pgch->L;
  int _top = lua_gettop(L);
  gc_root_t *proot;

  xgc_debug("-- func_name = %s\n", func_name);
  xgc_debug("-- parc_name = %s\n", parent_func_name);
  xgc_assert(func_name[0] != '\0');
  if (__gc_roots.next != NULL)
    xgc_assert( parent_func_name[0] != '\0' );
  xgc_assert(func_name[0] != '\0');
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
      proot->m_closed = MASK_NONE;
      strncpy( proot->my_function_name, func_name, sizeof(proot->my_function_name));
      strncpy( proot->my_parent_function_name, parent_func_name, sizeof(proot->my_parent_function_name));
      proot->is_primitive = is_primitive;
      proot->next = __gc_roots.next;
      __gc_roots.next = proot;
      
      xgc_assert ( lua_isuserdata(pgch->L, -1) ); 
      // stack layout: _G.K_ALIVEROOT_MAP proot
      xgc_assert( _top+2 == lua_gettop(L) );
      luaL_getmetatable(L, MT_ROOT);
      xgc_assert ( lua_istable(pgch->L, -1) ); 
      lua_setmetatable(L, -2);
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
      xgc_assert ( lua_istable(pgch->L, -1) ); 
      xgc_assert ( lua_isuserdata(pgch->L, -2) ); 
      xgc_assert ( lua_istable(pgch->L, -3) ); 
      lua_rawset(pgch->L, -3);       
      // stack layout: _G.K_ALIVEROOT_MAP
      xgc_assert( _top+1 == lua_gettop(L) );
      lua_pop(L, 1);
    }


  xgc_debug("**new proot = %p, my_func=%s\n\n", proot, proot->my_function_name);
  return proot;

}
gc_root_t *gc_root_new() 
{
  gc_root_t tmp_root = {NULL};
  gc_root_t *pfakeroot= __get_my_function_by_backtrace(2, &tmp_root);
  xgc_debug("** my_stack=%s\n", pfakeroot->my_function_name);
  xgc_debug("gc_root_new()\n");
  gc_collect();
  return gc_root_new_with_param(true, pfakeroot->my_function_name, pfakeroot->my_parent_function_name);
}

gc_root_t *gc_root_new_with_complex_return() 
{
  gc_root_t tmp_root = {NULL};
  gc_root_t *pfakeroot= __get_my_function_by_backtrace(2, &tmp_root);
  xgc_debug("** my_stack=%s\n", pfakeroot->my_function_name);
  xgc_debug("gc_root_new()\n");
  gc_collect();
  return gc_root_new_with_param(false, pfakeroot->my_function_name, pfakeroot->my_parent_function_name);

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
      obj->proot = proot;
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
  xgc_debug("**maloc xobj=%p, p=%p \n", obj, obj->p);
  return obj->p;
}

static void xgc_mark_ref_with_params(void *source_ptr, void *dest_ptr, bool is_one2many)
{
  gc_heap_t *pgch =  gc_heap_t_init();
  lua_State* L = pgch->L;
  int _top = lua_gettop(L);
  xgc_obj_t *sp = cast(xgc_obj_t*, source_ptr)-1;
  xgc_obj_t *dp = cast(xgc_obj_t*, source_ptr)-1;

step01: // get _G.K_ALIVEROOT_MAP[sp.proot] as {} 
    {
      lua_getglobal(L, K_ALIVEROOT_MAP);
      xgc_pushuserdata(L, sp->proot);
      xgc_assert( lua_isuserdata(L, -1) );
      lua_rawget(L, -2);
      xgc_debug("\t top = %d, proot=%p\n", lua_gettop(L), sp->proot);
      xgc_assert( lua_istable(L, -1) );
      // stack layout:  _G.K_ALIVEROOT_MAP _G.K_ALIVEROOT_MAP[sp->proot] as {}
      lua_remove(L, -2);
      // stack layout:  _G.K_ALIVEROOT_MAP[sp->proot] as {}
      xgc_assert(_top+1 == lua_gettop(L));
    }

step02: // set _G.K_ALIVEROOT_MAP[sp.proot].sp={}
    {
      // stack layout:  _G.K_ALIVEROOT_MAP[sp.proot] as {}
      xgc_assert(_top+1 == lua_gettop(L));

      xgc_pushuserdata(L, sp);
      lua_rawget(L, -2);
      xgc_assert (!lua_isnil(L, -1));
      // stack layout:  _G.K_ALIVEROOT_MAP[sp->proot] as {}, _G.K_ALIVEROOT_MAP[sp->proot].sp as {}?
      if (!lua_istable(L, -1) || !is_one2many)
	{
	  lua_pop(L, 1);
	  // stack layout:  _G.K_ALIVEROOT_MAP[sp->proot] as {}
	  xgc_assert(_top+1 == lua_gettop(L));
	  xgc_pushuserdata(L, sp);
	  lua_newtable(L);
	  lua_rawset(L, -3);
	  // stack layout:  _G.K_ALIVEROOT_MAP[sp->proot] as {}
	  xgc_assert(_top+1 == lua_gettop(L));
	  xgc_pushuserdata(L, sp);
	  lua_rawget(L, -2);
	  // stack layout: _G.K_ALIVEROOT_MAP[sp->proot] as {},  _G.K_ALIVEROOT_MAP[sp->proot][sp] as {}, 
	  xgc_assert(_top+2 == lua_gettop(L));
	  lua_remove(L, -2);
	  // stack layout: _G.K_ALIVEROOT_MAP[sp->proot][sp->proot][sp] as {}, 
	  xgc_assert(_top+1 == lua_gettop(L));
	}
    }

step03: // set ref: _G.K_ALIVEROOT_MAP[sp->proot].sp[dp] = true

    {
      // stack layout:  _G.K_ALIVEROOT_MAP[sp->proot].sp as {}
      xgc_assert(_top+1 == lua_gettop(L));
      xgc_pushuserdata(L, dp);
      lua_pushboolean(L, luat_true);
      lua_rawset(L, -3);
      lua_pop(L, 1);
      xgc_assert(_top+0 == lua_gettop(L));
    }
}

void gc_mark_ref(void *source_ptr, void *dest_ptr)
{
  xgc_mark_ref_with_params(source_ptr, dest_ptr, false);
}

void gc_mark_ref_with_one2many(void *source_ptr, void *dest_ptr)
{
  xgc_mark_ref_with_params(source_ptr, dest_ptr, true);
}

extern void gc_mark_unref(void *source_ptr, void *dest_ptr)
{
  gc_heap_t *pgch =  gc_heap_t_init();
  lua_State* L = pgch->L;
  int _top = lua_gettop(L);
  xgc_obj_t *sp = cast(xgc_obj_t*, source_ptr)-1;
  xgc_obj_t *dp = cast(xgc_obj_t*, source_ptr)-1;

step01: // get _G.K_ALIVEROOT_MAP[sp.proot] as {} 
    {
      lua_getglobal(L, K_ALIVEROOT_MAP);
      xgc_pushuserdata(L, sp->proot);
      xgc_assert( lua_isuserdata(L, -1) );
      lua_rawget(L, -2);
      xgc_debug("\t top = %d, proot=%p\n", lua_gettop(L), sp->proot);
      xgc_assert( lua_istable(L, -1) );
      // stack layout:  _G.K_ALIVEROOT_MAP _G.K_ALIVEROOT_MAP[sp->proot] as {}
      lua_remove(L, -2);
      // stack layout:  _G.K_ALIVEROOT_MAP[sp->proot] as {}
      xgc_assert(_top+1 == lua_gettop(L));
    }

step02: // set _G.K_ALIVEROOT_MAP[sp.proot].sp={}
    {
      // stack layout:  _G.K_ALIVEROOT_MAP[sp.proot] as {}
      xgc_assert(_top+1 == lua_gettop(L));

      xgc_pushuserdata(L, sp);
      lua_rawget(L, -2);
      xgc_assert (!lua_isnil(L, -1));
      // stack layout:  _G.K_ALIVEROOT_MAP[sp->proot] as {}, _G.K_ALIVEROOT_MAP[sp->proot].sp as {}?
      if (lua_istable(L, -1))
	{
	  lua_remove(L, -2);
	  // stack layout:  _G.K_ALIVEROOT_MAP[sp->proot].sp as {}
	  xgc_assert(_top+1 == lua_gettop(L));
	  xgc_pushuserdata(L, dp);
	  lua_pushnil(L);
	  lua_rawset(L, -3);
	  lua_pop(L, 1);
	  xgc_assert(_top+0 == lua_gettop(L));
	}
      else
	{
	  // stack layout:  _G.K_ALIVEROOT_MAP[sp.proot] as {}, _G.K_ALIVEROOT_MAP[sp.proot].[sp]
	  lua_pop(L, 2);
	  xgc_assert(_top+0 == lua_gettop(L));
	}
    }
}

void gc_collect()
{
  gc_heap_t *pgch =  gc_heap_t_init();
  lua_State* L = pgch->L;
  int _top = lua_gettop(L);
  int nCollected = 0;
  //  lua_gc(pgch->L, LUA_GCSTEP, 1024);

  lua_getglobal(L, K_ALIVEROOT_MAP);
  for (gc_root_t *prev=&__gc_roots, *proot = __gc_roots.next; proot != NULL; proot = prev->next)
    {
      if (proot->m_closed > MASK_NONE)
	{
	  prev->next = proot->next;
	  xgc_info("will collect proot = %p, func=%s\n", proot, proot->my_function_name);

	  xgc_pushuserdata(L, proot);
	  lua_pushnil(L);
	  lua_rawset(L, -3);
	  xgc_assert(_top+1 == lua_gettop(L));
	  nCollected++;
	}
      else
	{
	  prev = proot;
	}
    }
  lua_pop(L, 1);
  if (nCollected > 0)
    {
      xgc_println();
      xgc_debug("start gc process with collected_roots = %d ...\n", nCollected);
      xgc_print_stacktrace(__FILE__, __LINE__);
      lua_gc(pgch->L, LUA_GCCOLLECT, 0);
    }
  else
      xgc_debug("start gc process with collected_roots = %d ...\n", nCollected);
}

gc_root_t *__get_my_function_by_backtrace(int iLevel, gc_root_t *self_proot)
{
  int size = 16;
  void * array[16];
  int stack_num = backtrace(array, size);
  char ** stacktrace = backtrace_symbols(array, stack_num);
  gc_root_t *proot = __gc_roots.next;
  gc_root_t *proot_parent = NULL;
   
  xgc_debug("found my_function start ...\n");

    for (int i = 0; i < stack_num; ++i)
    {
        xgc_debug("\t%s\n", stacktrace[i]);
    }

step01: // get function by backtrace
  self_proot->my_function_name[0] = '\0';
 xgc_assert (iLevel < stack_num)
  if (iLevel < stack_num)
    {
      char *cp = self_proot->my_function_name;
      char *cpp;
      strncpy(cp, stacktrace[iLevel], sizeof(self_proot->my_function_name));
      if ((cpp = strchr(cp, ' ')) != NULL)
	*cpp ='\0';
      if ((cpp = strchr(cp, '+')) != NULL)
	  *cpp = '\0';
    }

step02: // find out parent's proot
 if (proot != NULL)
   {
     for (proot_parent = NULL, proot = __gc_roots.next; proot != NULL; proot = proot->next)
       {
	 char * fname = proot->my_function_name;
	 size_t fname_sz = strlen(fname);

	 for (int i = iLevel+1; i < stack_num; i++)
	   {
	       if ( strncmp(fname, stacktrace[i], fname_sz) )
		 continue;
	       // found parent root
	       proot_parent = proot;
	       strncpy(self_proot->my_parent_function_name, proot->my_function_name, sizeof(self_proot->my_parent_function_name));
	       break;
	     }
	   if (proot_parent != NULL)
	     {
	       break;
	     }
	 }
    }

step03: // found parent proot, set neighbour proot which is primitive
  if (proot_parent != NULL)
    {
      proot = __gc_roots.next;
      for (; proot != proot_parent && proot != NULL; proot = proot->next)
	if (proot->is_primitive && proot->m_closed == MASK_NONE)
	{
	  gc_root_t *xproot = __gc_roots.next;
	  size_t sz = strlen(proot->my_function_name);
	  proot->m_closed = MASK_INITIAL_CLOSED;
	  //set subsequent child root to be closed; 
	  for (; xproot != NULL && xproot != proot; xproot = xproot->next)
	    {
	      if (xproot->m_closed != MASK_INITIAL_CLOSED && 
		  !strncmp(xproot->my_parent_function_name, proot->my_function_name, sz))
		{
		  xproot->m_closed = MASK_SUBSEQUENT_CLOSED;
		}
 	    } 
	}
    }
step200_exit:
  free(stacktrace);
  return self_proot;
}
