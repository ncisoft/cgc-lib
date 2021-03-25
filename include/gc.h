#ifndef __GC_H_
#define __GC_H_
#define UUID_2FF3A001_1E1B_4344_888A_70F3AAE7B9BB


#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include <colors.h>

#ifdef __cplusplus
extern "C"
{
#endif


  /**
   * garbdge collection path:
   *   root -> {malloced_obj}
   *   malloced_object -> {ref_object}
   * when root was expired
   *   set root -> nil
   *   then all objs malloced by the root will be GCed
   **/

  typedef struct gc_root {
    struct gc_root *next;
    bool is_primitive;
    char* my_func_name;
    void* my_func;
    void* my_parent_func;
    int m_closed;
  } gc_root_t;

  typedef gc_root_t* gc_root_ptr_t;

  typedef void (*fobj_gc)(void *p);
  typedef void (*delegated_free)(void *p);

  typedef struct gc_obj_holder
    {
      void *p;
      delegated_free _free;
    } gc_obj_holder_t;

  extern void *gc_obj_holder_new(gc_root_t *proot, void *p, delegated_free _free);

  extern gc_root_t *gc_root_new(const char *my_func_name);
  extern gc_root_t *gc_root_new_with_complex_return(const char *my_func_name);
  extern void gc_root_close(gc_root_ptr_t *pproot);
  extern void gc_root_cleanup(void *p);

  extern void *gc_malloc(gc_root_t *proot, size_t sz);
  extern void *gc_malloc_with_gc(gc_root_t *proot, size_t sz, fobj_gc _gc);

  extern void *gc_checkin(gc_root_t *proot, void *dest, void *src);
  extern void *gc_checkout(gc_root_t *proot, void *dest, void *src);

  extern void gc_mark_ref(void *source_ptr, void *dest_ptr);
  extern void gc_mark_ref_with_one2many(void *source_ptr, void *dest_ptr);
  extern void gc_mark_unref(void *source_ptr, void *dest_ptr);
  extern void gc_collect();

  extern void xgc_print_stacktrace(char *fname, int lineno);

#ifndef __GNUC__
#error   RAII For c  support gcc only
#endif

#define using_raii_proot()                                                        \
  for ( gc_root_ptr_t                                                             \
       proot __attribute__((cleanup (gc_root_close))) = gc_root_new(__FUNCTION__) \
       , proot_next = proot;                                                      \
       proot_next == proot; proot_next++)

#define using_raii_proot_complex_return()              \
  for ( gc_root_ptr_t                                  \
       proot __attribute__((cleanup (gc_root_close)))  \
       = gc_root_new_with_complex_return(__FUNCTION__) \
       , proot_next=proot;                             \
       proot_next == proot; proot_next++ )

#define dismiss_unused(o) \
  do {                    \
  } while ((o) != (o))

#ifndef cast
#define cast(type, expr) ((type)(expr))
#endif

#ifndef sizeof_uint
# define sizeof_uint(type) cast(unsigned int, sizeof(type))
#endif

#define GC_LOGLEVEL_DEBUG       7
#define GC_LOGLEVEL_INFO        6
#define GC_LOGLEVEL_WARNING     5
#define GC_LOGLEVEL_ERROR       4
#define GC_LOGLEVEL_FATAL       4

#define GC_LOG_BRIEF false

#define GC_LOG(LEVEL, logstr, color, __fmt, ...)       if (LEVEL <= __gc_log_level)  {   \
  if (GC_LOG_BRIEF) \
  fprintf(stdout, color "%s:%d\t%s\t", strrchr(__FILE__, '/')+1,  __LINE__,logstr); \
  else \
  fprintf(stdout, color "%s:%s:%d\t%s\t", strrchr(__FILE__, '/')+1,   __FUNCTION__,__LINE__, logstr); \
  fprintf(stdout, __fmt, ##__VA_ARGS__); \
  fprintf(stdout, __C_RESET__ ); \
}

#define xgc_debug(__fmt, ...)     GC_LOG(GC_LOGLEVEL_DEBUG, "DEBUG", __C_RESET__,  __fmt, ##__VA_ARGS__)
#define xgc_info(__fmt, ...)     GC_LOG(GC_LOGLEVEL_INFO, "INFO", __C_YELLOW__,  __fmt, ##__VA_ARGS__)
#define xgc_warn(__fmt, ...)     GC_LOG(GC_LOGLEVEL_WARNING, "WARN", __C_CYAN__,  __fmt, ##__VA_ARGS__)
#define xgc_error(__fmt, ...)     GC_LOG(GC_LOGLEVEL_ERROR, "ERROR", __C_RED__,  __fmt, ##__VA_ARGS__)
#define xgc_fatal(__fmt, ...)     GC_LOG(GC_LOGLEVEL_ERROR, "FATAL", __C_YELLOW__,  __fmt, ##__VA_ARGS__)

#define xgc_assert(expr)  { if (!(expr)) { xgc_print_stacktrace(__FILE__,  __LINE__);  assert(expr); } }


#define xgc_println() printf("%s\n", __C_RESET__ );
#define xgc_reset() pintf("%s", __C_RESET__ );

extern int __gc_log_level;

#ifdef __cplusplus
}

#endif

#endif
