// Dummy GC implementation. Does no GC, just calls out to malloc.

#include "gc.hpp"
#include "util.hpp"

#include <cstdlib>
#include <cassert>

use util;

namespace gc {

ptr_t alloc(Context *cx, size_t size, void *find_roots_data) {
    (void) cx;
    (void) find_roots_data;
    void *p = malloc(size);
    assert (p);
    return p;
}

Context *init() { return NULL; }
void finish(Context *cx) { (void) cx; }

Context *create(Context *parent) {
    assert (parent != NULL);
    (void) parent;
    return NULL;
}

Context *merge(
    Context *parent, void *parent_find_roots_data,
    Context *child, void *child_find_roots_data)
{
    (void) parent; (void) parent_find_roots_data;
    (void) child; (void) child_find_roots_data;
    return NULL;
}

void suspend(Context *heap) {
    (void) heap;
}

// should never get called
void found_roots(CycleContext *cx, size_t nroots, ptr_t *roots) {
    die("unimplemented");
    (void) cx; (void) nroots; (void) roots;
}

void found_ptrs(CycleContext *cx, size_t nptrs, ptr_t *ptrs) {
    die("unimplemented");
    (void) cx; (void) nptrs; (void) ptrs;
}

} // namespace gc
