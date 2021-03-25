#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#include <luaext.h>
#include <gc.h>
#include <unistd.h>
#include <string.h>
#include <execinfo.h>
#include <sys/types.h>
#include <libcork/core.h>


typedef struct {
  lua_State *L;

} gc_heap_t;

#define K_ALIVEROOT_MAP "__alive_root_map" // {root->{}, root.hex->root}
#define MT_ROOT "__mt_root"
#define MT_MANAGED_OBJ "__mt_managed_obj"

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

typedef struct calling_stack
{
  struct calling_stack *next;
} calling_stack_t;

#define CALLING_STACK_T_POOL_CAPACITY 100
struct
{
  bool inited;
  calling_stack_t calling_stack_root;
  struct  cork_mempool *calling_stack_t_pool;
  int calling_stack_t_pool_capacity;
}L = {
    .inited = false,
    .calling_stack_root.next=NULL,
    .calling_stack_t_pool_capacity = CALLING_STACK_T_POOL_CAPACITY,
};

static void initL()
{
  if (L.inited == true)
    return;
  L.inited = true;
  L.calling_stack_root.next = NULL;
  L.calling_stack_t_pool = cork_mempool_new_size_ex( sizeof(calling_stack_t), L.calling_stack_t_pool_capacity);
}

/*--------- static objects -------------*/
 int __gc_log_level = GC_LOGLEVEL_DEBUG;
static gc_heap_t __gc_heap = { NULL };
static gc_root_t __gc_roots = { NULL };

/*--------- static functions -------------*/
static void push_calling_stack()
{
  initL();
  calling_stack_t *p = (calling_stack_t *)cork_mempool_new_object(L.calling_stack_t_pool);
  xgc_debug("calling_stack_t *p= %p\n", p);
  xgc_assert(p);
  p->next = L.calling_stack_root.next;
  L.calling_stack_root.next = p;
  xgc_debug ("L.calling_stack_root.next = %p\n", L.calling_stack_root.next);
//  xgc_print_stacktrace(__FILE__, __LINE__);
}

static void pop_calling_stack()
{
  calling_stack_t *p = L.calling_stack_root.next;
  xgc_assert(p);
  L.calling_stack_root.next = p->next;
  cork_mempool_free_object(L.calling_stack_t_pool, p);
  //xgc_info("pop_calling_stack L.calling_stack_root.next = %p\n", L.calling_stack_root.next);
}

static calling_stack_t* peek_calling_stack()
{
  return L.calling_stack_root.next;
}

calling_stack_t *find_calling_stack_parent(void *p)
{
  calling_stack_t *ps = (calling_stack_t *)p;
  xgc_assert(ps);
  return ps->next;
}

static gc_heap_t *gc_heap_t_init();
static gc_root_t *gc_root_new_with_param(bool is_primitive, char *func_name, void *func, void *parent_func);
static int __root_gc(lua_State* L);
static int __obj_gc(lua_State* L);
static gc_root_t *__get_my_func_by_backtrace(int iLevel, gc_root_t *self_proot);

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
      printf("\t%s%s\n", stacktrace[i], __C_RESET__);
    }
  free(stacktrace);
  //xgc_println();

}
extern void *gc_obj_holder_new(gc_root_t *proot, void *p, delegated_free _free)
{
  gc_obj_holder_t  *holder = gc_malloc(proot, sizeof(gc_obj_holder_t));
  holder->p = p;
  holder->_free = _free;
  return holder;
}

extern void gc_root_cleanup(void *p)
{
  gc_root_t *proot=p;
  //gc_root_close(&proot);
  proot->m_closed = MASK_INITIAL_CLOSED;
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

void gc_root_close(gc_root_ptr_t *pproot)
{
  gc_root_ptr_t proot = *pproot;
  pop_calling_stack();
  dismiss_unused(pproot);
  if (proot->is_primitive)
    {
      proot->m_closed = MASK_INITIAL_CLOSED;
      gc_collect();
    }
  else
    xgc_info("postpone collect <%s> parent_func=%p because its return type is nor primitive",
             proot->my_func_name, proot->my_parent_func
             );
}

int __root_gc(lua_State* L)
{
  gc_root_t *proot = cast(gc_root_t *, lua_touserdata(L, -1));
  xgc_debug("release <%s> gc_root=%p func=%p\n", proot->my_func_name, proot, proot->my_func);
  return 0;
}

int __obj_gc(lua_State* L)
{
  xgc_obj_t *xobj = cast(xgc_obj_t *, lua_touserdata(L, -1));
  gc_root_t *proot = xobj->proot;
  xgc_debug("  release <%s> _p=%p, xobj=%p\n", proot->my_func_name,
            xobj+1,xobj);
  if (xobj->_gc)
    xobj->_gc(xobj->p);
  return 0;
}

gc_root_t *gc_root_new_with_param(bool  is_primitive, char *func_name,
				  void *func, void *parent_func)
{
  gc_heap_t *pgch = gc_heap_t_init();
  lua_State *L = pgch->L;
  int _top = lua_gettop(L);
  gc_root_t *proot;

  xgc_debug("-- func = %p\n", func);
  xgc_debug("-- parent = %p\n", parent_func);
  xgc_debug("-- __gc_roots.next = %p\n", __gc_roots.next);
  xgc_assert(func != NULL);
  if (__gc_roots.next != NULL)
    xgc_assert( parent_func != NULL);
  xgc_assert(func != NULL);
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
      proot->my_func_name = func_name;
      proot->my_func= func;
      proot->my_parent_func= parent_func;
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


  xgc_info("**new proot = <%s> %p, my_func=%p parent_func=%p\n\n",
           proot->my_func_name,
           proot,
           proot->my_func,
           proot->my_parent_func);
  return proot;

}
gc_root_t *gc_root_new(const char *function_name)
{
  push_calling_stack();
  xgc_debug("gc_root_new(), __gc_roots.next=%p\n", __gc_roots.next);
  gc_root_t tmp_root = {.next=NULL,
      .my_func_name = cast(char *, function_name),
      .my_func=NULL, .my_parent_func=NULL};
  gc_root_t *pfakeroot= __get_my_func_by_backtrace(2, &tmp_root);
  xgc_debug("** my_stack=%p--%p--\n", pfakeroot->my_func, pfakeroot->my_parent_func);
  xgc_debug("__gc_roots.next = %p\n", __gc_roots.next);
  //gc_collect();
  xgc_debug("-- __gc_roots.next = <%p>\n", __gc_roots.next);
  return gc_root_new_with_param(true, pfakeroot->my_func_name,
                                pfakeroot->my_func, pfakeroot->my_parent_func);
}

gc_root_t *gc_root_new_with_complex_return(const char *function_name)
{
  push_calling_stack();
  gc_root_t tmp_root = {.next=NULL,
      .my_func_name = cast(char *, function_name),
      .my_func=NULL, .my_parent_func=NULL};
  gc_root_t *pfakeroot= __get_my_func_by_backtrace(2, &tmp_root);
  xgc_debug("** my_stack=%p\n", pfakeroot->my_func);
  xgc_debug("gc_root_new()\n");
  //gc_collect();
  return gc_root_new_with_param(false,pfakeroot->my_func_name,
                                pfakeroot->my_func, pfakeroot->my_parent_func);

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
      xgc_info("<%s>\n", proot->my_func_name);
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
  xgc_obj_t *dp = cast(xgc_obj_t*, dest_ptr)-1;

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
      xgc_info("gc_mark_ref succ proot=<%s>, source=%p dest=%p\n",
               sp->proot->my_func_name,
               source_ptr, dest_ptr);
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

static void gc_mark()
{
  bool do_scan=true;
  int niterator = 0;

  while (do_scan == true)
    {
      bool new_mark=false;
      for (gc_root_t *proot = __gc_roots.next; proot != NULL; proot = proot->next)
        {
          if (proot->m_closed == MASK_NONE )
            for (gc_root_t *pmask = __gc_roots.next; pmask != NULL; pmask = pmask->next)
              if (proot->my_parent_func == pmask->my_func && pmask->m_closed > MASK_NONE)
                {
                  proot->m_closed = MASK_SUBSEQUENT_CLOSED;
                  xgc_info("[%d] mark <%s> as MASK_SUBSEQUENT_CLOSED\n", niterator, proot->my_func_name);
                  new_mark = true;
                  break;
                }
          if (new_mark == true)
            {
              niterator++;
              break;
            }
        }
      do_scan = new_mark;
    }
}

static int gc_count_root()
{
 int n=0;
  for (gc_root_t *proot = __gc_roots.next; proot != NULL; proot = proot->next)
      n++;
  return n;
}

static void gc_dump_roots()
{
 int n=0;
 xgc_debug("dump __gc_roots\n");
  for (gc_root_t *proot = __gc_roots.next; proot != NULL; proot = proot->next)
    {
      printf("  [%d] root=<%12s> my_func=<%p> my_parent_func=<%p>\n", n++,
             proot->my_func_name,
             proot->my_func, proot->my_parent_func);
    }
}

void gc_collect()
{
  gc_heap_t *pgch =  gc_heap_t_init();
  lua_State* L = pgch->L;
  int _top = lua_gettop(L);
  int nCollected = 0;
  //  lua_gc(pgch->L, LUA_GCSTEP, 1024);

  gc_dump_roots();
  gc_mark();
  lua_getglobal(L, K_ALIVEROOT_MAP);
  for (gc_root_t *prev=&__gc_roots, *proot = __gc_roots.next; proot != NULL; proot = prev->next)
    {
      if (proot->m_closed > MASK_NONE)
	{
	  prev->next = proot->next;
	  xgc_info("will collect proot = [%d] <%s> is_primitive=%d %p, func=<%p>\n",
                   gc_count_root(),
                   proot->my_func_name, proot->is_primitive,
                   proot, proot->my_func);

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
      lua_gc(pgch->L, LUA_GCCOLLECT, 0);
    }
  else
      xgc_debug("start gc process with collected_roots = %d ...\n", nCollected);
}

gc_root_t *__get_my_func_by_backtrace(int iLevel, gc_root_t *self_proot)
{
  gc_root_t *proot = __gc_roots.next;
  gc_root_t *proot_parent = NULL;

  xgc_debug("found my_func start ...\n");

step01: // get function by backtrace
    {
      calling_stack_t *ps = peek_calling_stack();
      xgc_debug("peek_calling_stack()=%p\n", ps);
      xgc_assert(ps);
      self_proot->my_func = ps;
      self_proot->my_parent_func = ps->next;

      xgc_debug("found my_func=%p, name=%s--\n", self_proot->my_func, self_proot->my_func_name);
      //xgc_debug("-- __gc_roots.next = <%p>\n", __gc_roots.next);
    }

step02: // find out parent's proot
    if (proot != NULL)
      {
        for (proot_parent = NULL, proot = __gc_roots.next; proot != NULL; proot = proot->next)
          {
            if (self_proot->my_parent_func == proot->my_func)
              {
                // found parent root
                proot_parent = proot;
                break;
              }
          }
      }

step03: // found parent proot, set neighbour proot which is primitive
    if (proot_parent == NULL)
      {
        xgc_info("<%s> found proot_parent fail\n", self_proot->my_func_name);
      }
    else
      {
        proot = __gc_roots.next;
        for (; proot != proot_parent && proot != NULL; proot = proot->next)
          if (proot->is_primitive && proot->m_closed == MASK_NONE)
            {
              gc_root_t *xproot = __gc_roots.next;
              proot->m_closed = MASK_INITIAL_CLOSED;
              //set subsequent child root to be closed;
              for (; xproot != NULL && xproot != proot; xproot = xproot->next)
                {
                  if (xproot->m_closed != MASK_INITIAL_CLOSED &&
                      xproot->my_parent_func != proot->my_func)
                    {
                      xproot->m_closed = MASK_SUBSEQUENT_CLOSED;
                    }
                }
            }
      }
step200_exit:
  return self_proot;
}
