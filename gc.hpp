#ifndef GC_HPP_
#define GC_HPP_

#include <cstddef>

namespace pml::gc {

typedef void *ptr_t;

struct Context;
struct CycleContext;

/* ---------- Allocation ---------- */

/* Note that calling gc_alloc() may cause a GC cycle. Therefore any memory
 * allocated by a previous call to gc_alloc must be initialized before calling
 * gc_alloc() again.
 *
 * `find_roots_data' is passed to client::find_roots_alloc() if we do a GC
 * cycle.
 */
ptr_t alloc(Context *cx, size_t size, void *find_roots_data);


/* ---------- Heap management ---------- */
Context *heap_create(Context *parent); // pass NULL to create initial heap
Context *heap_merge(Context *parent, void *parent_find_roots_data,
                    Context *child, void *child_find_roots_data);
void heap_suspend(Context *heap);


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

} // namespace pml::gc

#endif // GC_HPP_
