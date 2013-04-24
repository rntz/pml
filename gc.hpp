#ifndef GC_HPP_
#define GC_HPP_

#include <cstddef>

namespace gc {

typedef void *ptr_t;

struct Context;
struct CycleContext;


/* ---------- Context management ---------- */
Context *init();
void finish(Context *cx);

Context *create(Context *parent);
Context *merge(Context *parent, void *parent_find_roots_data,
               Context *child, void *child_find_roots_data);
void suspend(Context *heap);


/* ---------- Allocation ---------- */

/* Note that calling alloc() may cause a GC cycle. Therefore any memory
 * allocated by a previous call to alloc() must be initialized before calling
 * alloc() again.
 *
 * `find_roots_data' is passed to client::find_roots_alloc() if we do a GC
 * cycle.
 */
ptr_t alloc(Context *cx, size_t size, void *find_roots_data);


/* ---------- GC interface and client responsibilities ---------- */

// Called by client::find_roots().
void found_roots(CycleContext *cx, size_t nroots, ptr_t *roots);

// Called by client::scan_object().
void found_ptrs(CycleContext *cx, size_t nptrs, ptr_t *ptrs);

namespace client {

void find_roots_merge(CycleContext *cx, void *find_roots_data);
void find_roots_alloc(CycleContext *cx, void *find_roots_data);
void find_ptrs(CycleContext *cx, ptr_t object);

} // namespace client

} // namespace gc

#endif // GC_HPP_
