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

/* Age counters are used to distinguish objects in child heaps from those in
 * ancestor heaps. They are monotonically increasing within the partial ordering
 * on heaps formed by memory visibility. That is to say, if heap A can see
 * objects in heap B, then heap A has a greater age than heap B.
 */
typedef uint32_t age_t;

/* ---------- Chunks and blocks ---------- */
extern "C" {
    typedef struct chunk chunk_t;
    typedef struct free_block free_block_t;
    typedef struct used_block used_block_t;

    // Header for a contiguous chunk of allocatable space
    struct chunk {
        size_t size;
        chunk_t *next;
    };

    struct used_block {
        // `size' includes space used by header
        size_t size;            // low bits used for metadata
        age_t age;
    };

    struct free_block {
        size_t size;            // ditto used_block
        free_block_t *next;     // next free block
    };
}

/* ---------- Manipulating block headers ---------- */
#define BLOCK_SIZE_ALIGNMENT 4
#define BLOCK_INFO_MASK      3
#define BLOCK_USED_FLAG      1
#define BLOCK_MARKED_FLAG    2

#define MACHINE_ALIGN(x) ALIGN_UP(MACHINE_ALIGNMENT, x)
#define BLOCK_SIZE_ALIGN(x) ALIGN_UP(BLOCK_SIZE_ALIGNMENT, x)
#define USED_BLOCK_HEADER_SIZE MACHINE_ALIGN(sizeof(used_block_t))
#define CHUNK_HEADER_SIZE MACHINE_ALIGN(sizeof(chunk_t))

#define MIN_BLOCK_SIZE BLOCK_SIZE_ALIGN(                                \
        MAX(sizeof(free_block_t), USED_BLOCK_HEADER_SIZE + 2*sizeof(void*)))

static inline used_block_t *block_from_ptr(void *ptr) {
    assert (ALIGNED(MACHINE_ALIGNMENT, (uintptr_t) ptr));
    return (used_block_t*)(((char*)ptr) - USED_BLOCK_HEADER_SIZE);
}

static inline void *block_to_ptr(used_block_t *blk) {
    return (void*)(((char*)blk) + USED_BLOCK_HEADER_SIZE);
}

#define BLOCK_SIZE(blk) ((bool)((blk)->size & ~BLOCK_INFO_MASK))
#define BLOCK_USED(blk) ((bool)((blk)->size & BLOCK_USED_FLAG))
#define BLOCK_FREE(blk) (!BLOCK_USED(blk))
#define BLOCK_MARKED(blk) ((bool)((blk)->size & BLOCK_MARKED_FLAG))

static inline free_block_t *block_make_free(used_block_t *blk) {
    assert (offsetof(used_block_t, size) == offsetof(free_block_t, size));
    assert (!BLOCK_MARKED(blk) && BLOCK_USED(blk));
    blk->size &= ~BLOCK_USED_FLAG;
    return (free_block_t*) blk;
}

static inline used_block_t *block_make_used(free_block_t *blk) {
    assert (offsetof(used_block_t, size) == offsetof(free_block_t, size));
    assert (!BLOCK_MARKED(blk) && BLOCK_FREE(blk));
    blk->size |= BLOCK_USED_FLAG;
    return (used_block_t*) blk;
}

static inline void mark_block(used_block_t *blk) {
    assert(!BLOCK_MARKED(blk));
    blk->size |= BLOCK_MARKED_FLAG;
}

static inline void unmark_block(used_block_t *blk) {
    assert (BLOCK_MARKED(blk));
    blk->size &= ~BLOCK_MARKED_FLAG;
}


/* ---------- Contexts and Heaps ---------- */
// Carefully calculated so that we GC when we use up our initial chunk.
#define INITIAL_OLD_SPACE                                               \
    ((size_t)((MIN_CHUNK_SIZE - CHUNK_HEADER_SIZE) / GC_NEWSPACE_RATIO))

struct Heap {
    chunk_t *chunks;
    free_block_t *free_head, *free_tail;
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
    age_t age;
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


/* ---------- ALLOCATION ---------- */

// helper function forward decls
static void check_for_alloc_gc(Heap *heap, size_t extra, void *find_roots_data);
static free_block_t *split_block(Heap *heap, free_block_t *blk, size_t size);
static void remove_from_free_list(
    Heap *heap, free_block_t *prev, free_block_t *blk);
static free_block_t *get_block_from_new_chunk(Heap *heap, size_t size);

ptr_t alloc(Context *cx, size_t size, void *find_roots_data) {
    size_t real_size = BLOCK_SIZE_ALIGN(USED_BLOCK_HEADER_SIZE + size);
    Heap *heap = &cx->heap;

    // Check whether allocating would exceed our limits. If so, run a GC cycle.
    check_for_alloc_gc(heap, real_size, find_roots_data);

    // Search for a free block to use.
    free_block_t *prev = NULL, *blk = heap->free_head;
    while (blk) {
        assert (BLOCK_FREE(blk) && !BLOCK_MARKED(blk));
        if (real_size <= BLOCK_SIZE(blk))
            break;              // found a block
        prev = blk;
        blk = blk->next;
    }

    if (!blk) {
        // Didn't find an block to allocate!
        blk = get_block_from_new_chunk(heap, real_size);
        prev = NULL;            // new block is on front of free list
    }

    // If the block is large enough, we can just split it.
    if (BLOCK_SIZE(blk) - real_size >= MIN_BLOCK_SIZE) {
        // Split the block.
        blk = split_block(heap, blk, real_size);
    } else {
        // Allocate the entire block.
        remove_from_free_list(heap, prev, blk);
    }

    used_block_t *block = block_make_used(blk);
    block->age = cx->age;
    assert (BLOCK_USED(blk) && !BLOCK_MARKED(blk));
    return block_to_ptr(block);
}

static void check_for_alloc_gc(Heap *heap, size_t extra, void *find_roots_data)
{
    // die("unimplemented");   // FIXME
    (void) heap;
    (void) extra;
    (void) find_roots_data;
}

static free_block_t *get_block_from_new_chunk(Heap *heap, size_t reqsz) {
    size_t size = MIN(MIN_CHUNK_SIZE, CHUNK_HEADER_SIZE + reqsz);
    chunk_t *chunk = (chunk_t*) smalloc(size);
    chunk->size = size;
    chunk->next = heap->chunks;
    heap->chunks = chunk;

    free_block_t *block = (free_block_t*)(((char*)chunk) + CHUNK_HEADER_SIZE);
    assert (ALIGNED(MACHINE_ALIGNMENT, (uintptr_t) block));

    // Set block metadata.
    block->size = size - CHUNK_HEADER_SIZE;
    assert (BLOCK_FREE(block) && !BLOCK_MARKED(block));

    // Push block on front of free list.
    block->next = heap->free_head;
    heap->free_head = block;
    if (!block->next) {
        heap->free_tail = block;
    }

    return block;
}

static free_block_t *split_block(Heap *heap, free_block_t *blk, size_t size) {
    size_t blk_size = BLOCK_SIZE(blk);
    size_t new_size = blk_size - size;
    assert (BLOCK_FREE(blk)
            && !BLOCK_MARKED(blk)
            && blk_size > size
            && new_size >= MIN_BLOCK_SIZE
            && size >= MIN_BLOCK_SIZE
            && ALIGNED(BLOCK_SIZE_ALIGNMENT, new_size));

    free_block_t *split = (free_block_t*)(((char*)blk) + new_size);
    blk->size = new_size;
    split->size = size;
    assert (BLOCK_FREE(blk) && !BLOCK_MARKED(blk)
            && BLOCK_FREE(split) && !BLOCK_MARKED(split));

    return split;
    (void) heap;
}

static void remove_from_free_list(
    Heap *heap, free_block_t *prev, free_block_t *blk)
{
    assert (!prev || prev->next == blk);
    free_block_t *head = heap->free_head,
                 *tail = heap->free_tail;

    if (prev && blk->next) {
        // blk isn't the head or the tail
        assert (blk != head && blk != tail);

        prev->next = NULL;
        tail->next = head;
        heap->free_head = blk->next;
        heap->free_tail = prev;
    }
    else if (!prev) {
        // we just used the head
        assert (blk == head && blk != tail);
        heap->free_head = blk->next;
    }
    else if (!blk->next) {
        // we just used the tail
        assert (blk != head && blk == tail);
        prev->next = NULL;
        heap->free_tail = prev;
    }
    else {
        // we just used the only block, which is both head & tail
        assert (blk == head && blk == tail);
        heap->free_head = heap->free_tail = NULL;
    }

    assert (heap->free_tail
            ? (heap->free_head && heap->free_tail->next == NULL)
            : !heap->free_head);
}


/* ---------- Other context manipulation ---------- */
Context *init() {
    return new Context();
}

void finish(Context *cx) {
    assert (!cx->children && !cx->parent && !cx->next_child);

    // Free all chunks.
    chunk_t *chunk = cx->heap.chunks;
    while (chunk) {
        chunk_t *p = chunk;
        chunk = p->next;
        sfree(p->size, p);
    }

    delete cx;
}

Context *create(Context *parent) {
    assert (parent != NULL);
    die("unimplemented");   // FIXME
    (void) parent;
    return NULL;
}

Context *merge(
    Context *parent, void *parent_find_roots_data,
    Context *child, void *child_find_roots_data)
{
    die("unimplemented");   // FIXME
    (void) parent; (void) parent_find_roots_data;
    (void) child; (void) child_find_roots_data;
    return NULL;
}

void suspend(Context *heap) {
    die("unimplemented");   // FIXME
    (void) heap;
}

// should never get called
void found_roots(CycleContext *cx, size_t nroots, ptr_t *roots) {
    die("unimplemented");   // FIXME
    (void) cx; (void) nroots; (void) roots;
}

void found_ptrs(CycleContext *cx, size_t nptrs, ptr_t *ptrs) {
    die("unimplemented");   // FIXME
    (void) cx; (void) nptrs; (void) ptrs;
}

} // namespace gc
