#ifndef __GC_H_
#define __GC_H_

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

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

  typedef struct _gc_root_t {
    struct _gc_root_t *next;
    bool is_primitive;

  } gc_root_t;

  typedef void (*fobj_gc)(void *p);

  extern gc_root_t *gc_root_new();
  extern gc_root_t *gc_root_new_with_complex_return();
  extern void *gc_malloc(gc_root_t *proot, size_t sz);
  extern void *gc_malloc_with_gc(gc_root_t *proot, size_t sz, fobj_gc _gc);
  extern void gc_mark_ref(void *source_ptr, void *ddest_ptr);
  extern void gc_mark_unref(void *source_ptr, void *ddest_ptr);
  extern void gc_collect();


#ifndef cast
#define cast(type, expr) ((type)(expr))
#endif

#define GC_LOGLEVEL_DEBUG       7
#define GC_LOGLEVEL_INFO        6
#define GC_LOGLEVEL_WARNING     5
#define GC_LOGLEVEL_ERROR       4

#define GC_LOG_BRIEF true

#define GC_LOG(LEVEL, logstr, __fmt, ...)       if (LEVEL <= __gc_log_level)  {   \
  if (GC_LOG_BRIEF) \
  fprintf(stderr, "%s:%d\t%s\t", strrchr(__FILE__, '/')+1,  __LINE__,logstr); \
  else \
  fprintf(stderr, "%s:%s:%d\t%s\t", strrchr(__FILE__, '/')+1,   __FUNCTION__,__LINE__, logstr); \
fprintf(stderr, __fmt, ##__VA_ARGS__); \
}

#define xgc_debug(__fmt, ...)     GC_LOG(GC_LOGLEVEL_DEBUG, "DEBUG", __fmt, ##__VA_ARGS__)
#define xgc_error(__fmt, ...)     GC_LOG(GC_LOGLEVEL_ERROR, "ERROR"__fmt, ##__VA_ARGS__)
#define xgc_warn(__fmt, ...)     GC_LOG(GC_LOGLEVEL_WARNING, "WARN", __fmt, ##__VA_ARGS__)
#define xgc_info(__fmt, ...)     GC_LOG(GC_LOGLEVEL_INFO, "INFO", __fmt, ##__VA_ARGS__)
#define xgc_assert(expr) assert(expr)

extern int __gc_log_level;

#ifdef __cplusplus
}

#endif

#endif
