// Dummy GC implementation. Does no GC, just calls out to malloc.

#include "gc.hpp"
#include "config.hpp"
#include "util.hpp"

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdlib>

extern "C" {
#include <pthread.h>
}

namespace gc {

using namespace util;

/* ---------- Chunks and blocks ---------- */
extern "C" {
    typedef struct chunk chunk_t;
    typedef struct block_header block_header_t;
    typedef struct block block_t;

    // Header for a contiguous chunk of allocatable space
    struct chunk {
        size_t size;
        chunk_t *next;
    };

    // Header for an allocated block
    struct block_header {
        // `size' includes space used by header
        size_t size;            // low bits used for metadata
    };

    struct block {
        block_header_t header;
        // Following fields used only if block is free.
        block_t *next;         // next free block
    };
}

/* ---------- Manipulating block headers ---------- */
#define BLOCK_MIN_ALIGN 4
#define BLOCK_INFO_MASK 3
#define BLOCK_FREE      1
#define BLOCK_MARKED    2

#define BLOCK_ALIGN MAX(MACHINE_ALIGNMENT, BLOCK_MIN_ALIGN)
#define BLOCK_HEADER_SIZE ALIGN_UP(MACHINE_ALIGNMENT, sizeof(block_header_t))

static inline block_t *block_from_ptr(void *ptr) {
    assert (ALIGNED(MACHINE_ALIGNMENT, (uintptr_t) ptr));
    return (block_t*)(((char*)ptr) - BLOCK_HEADER_SIZE);
}

static inline void *block_to_ptr(block_t *blk) {
    return (void*)(((char*)blk) + BLOCK_HEADER_SIZE);
}

static inline size_t block_size(block_t *blk) {
    return blk->header.size & ~BLOCK_INFO_MASK;
}

static inline bool block_is_marked(block_t *blk) {
    return (bool)(blk->header.size & BLOCK_MARKED);
}

static inline bool block_is_free(block_t *blk) {
    return (bool)(blk->header.size & BLOCK_FREE);
}

static inline void block_set_free(block_t *blk)
{ blk->header.size |= BLOCK_FREE; }
static inline void block_set_used(block_t *blk)
{ blk->header.size &= ~BLOCK_FREE; }

static inline void mark_block(block_t *blk)
{ blk->header.size |= BLOCK_MARKED; }
static inline void unmark_block(block_t *blk)
{ blk->header.size &= ~BLOCK_MARKED; }


/* ---------- Context operations ---------- */
// Carefully calculated so that we GC when we use up our initial chunk.
#define INITIAL_OLD_SPACE                                               \
    ((size_t)((MIN_CHUNK_SIZE - ALIGN_UP(BLOCK_ALIGN, sizeof(chunk_t))) \
              / GC_NEWSPACE_RATIO))

struct Heap {
    chunk_t *chunks;
    block_t *free_head, *free_tail;
    size_t used_space;
    size_t old_space;

    Heap() : chunks(NULL), free_head(NULL), free_tail(NULL), used_space(0),
             old_space(INITIAL_OLD_SPACE)
    {}
};

struct Context {
    Context *parent;
    Context *children;
    Context *next_child;
    Heap heap;
    pthread_mutex_t lock;

    Context() : parent(NULL), children(NULL), next_child(NULL), heap()
    {
        if (pthread_mutex_init(&lock, NULL))
            die("could not initialize mutex");
    }

    ~Context() {
        if (pthread_mutex_destroy(&lock))
            die("could not destroy mutex");
    }
};

ptr_t alloc(Context *cx, size_t size, void *find_roots_data) {
    size_t real_size = BLOCK_HEADER_SIZE + size;

    // Check whether allocating would exceed our limits. If so, run a GC cycle.
    die("unimplemented");   // FIXME
    (void) find_roots_data;

    // Search for a free block to use.
    block_t *prev = NULL, *blk = cx->heap.free_head;
    while (blk) {
        assert (block_is_free(blk));
        assert (!block_is_marked(blk)); // free blocks always unmarked
        if (real_size <= block_size(blk))
            break;              // found a block
        prev = blk;
        blk = blk->next;
    }

    if (!blk) {
        // Didn't find an block to allocate!
        die("unimplemented");   // FIXME
    }

    // Allocate this block.
    block_set_used(blk);

    // Remove block from list and make the free list start where we left off.
    block_t *head = cx->heap.free_head,
            *tail = cx->heap.free_tail;

    if (prev && blk->next) {
        // blk isn't the head or the tail
        assert (blk != head && blk != tail);

        prev->next = NULL;
        tail->next = head;
        cx->heap.free_head = blk->next;
        cx->heap.free_tail = prev;
    }
    else if (!prev) {
        // we just used the head
        assert (blk == head && blk != tail);
        cx->heap.free_head = blk->next;
    }
    else if (!blk->next) {
        // we just used the tail
        assert (blk != head && blk == tail);
        prev->next = NULL;
        cx->heap.free_tail = prev;
    }
    else {
        // we just used the only block, which is both head & tail
        assert (blk == head && blk == tail);
        cx->heap.free_head = cx->heap.free_tail = NULL;
    }

    assert (cx->heap.free_tail
            ? (cx->heap.free_head && cx->heap.free_tail->next == NULL)
            : !cx->heap.free_head);
    assert (!block_is_free(blk) && !block_is_marked(blk));
    return block_to_ptr(blk);
}

Context *init() {
    return new Context();
}

void finish(Context *cx) {
    die("unimplemented");
    (void) cx;
}

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
    abort();
    (void) cx; (void) nroots; (void) roots;
}

void found_ptrs(CycleContext *cx, size_t nptrs, ptr_t *ptrs) {
    abort();
    (void) cx; (void) nptrs; (void) ptrs;
}

} // namespace gc
