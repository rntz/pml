#ifndef RT_HPP_
#define RT_HPP_

#include <cstddef>
#include <cassert>

#include "util.hpp"

namespace pml::runtime {

struct Root;
struct Context;

// A runtime-managed pointer. typedef for documentation purposes.
typedef void *ptr_t;

struct TaskFn {
    ptr_t (*func)(Context *cx, void *data);
    // this is NOT traced by the GC, so anything it refers to must be kept alive
    // by other roots.
    void *data;
};


// A context, used to interact with the runtime. POD.
struct Context {
    Root *roots;
    pml::gc::Context gc_context;
    pml::sched::Context sched_context;
    Context *parent;            // our parent task
    size_t childno;             // which child we are of our parent

#ifndef NDEBUG
    Root *debug_roots_;
#endif

    static Context *init(void);

    // Should be called only on initial task, once completely finished.
    void finish(void);

    // fork2 and forkN returns the index of the first failing subtask, or the
    // total number of subtasks forked if all succeeded.
    //
    // If subtask `i` failed, all return values after index `i` may be
    // uninitialized and should be ignored.
    int fork(Root *ret1, Root *ret2, TaskFn fn1, TaskFn fn2);
    int forkN(size_t n, Root *rets, TaskFn *fns);

    // Signals that this task has failed, and its right-siblings can be
    // cancelled. Does *not* cause exceptional control flow; the calling task
    // function should return ASAP, and must not spawn any further tasks.
    void fail(Context *cx);

    void push(Root *root) {
        assert (!root->added_);
        DEBUG_EXPR(root->added_ = true);
        root->next_ = roots;
        roots = root;
    }

    void pop(Root *root) {
        // FIXME
    }
};


struct Root {
    friend class Context;

  private:
    ptr_t value_;
    Root *next_;
#ifndef NDEBUG
    // True iff we're recorded as a root in the context so the GC can see us.
    bool added_;
    Root *debug_next_;
#endif

  public:
    explicit Root(Context *cx, ptr_t init = NULL)
        : value_(init)
    {
#ifndef NDEBUG
        added_ = false;
        debug_next_ = cx->debug_roots_;
        cx->debug_roots_ this;
#endif
        (void) cx;              // make sure cx is used
    }

    ~Root() {
        assert(!added_);
    }

    ptr_t get() { return value_; }
    void set(ptr_t v) { value_ = v; }

  private:
    Root();
    NO_COPY(Root);
};


namespace client {

// TODO

} // namespace client

} // namespace pml::runtime

#endif // RT_H_
