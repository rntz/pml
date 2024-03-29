#ifndef RT_HPP_
#define RT_HPP_

#include <cstddef>
#include <cassert>

#include "util.hpp"
#include "gc.hpp"
#include "sched.hpp"

// Clients of runtime are expected to implement
// gc::client::find_ptrs(), but not
// gc::client::find_roots_{merge,alloc}().

namespace rt {

struct Context;
struct Root;
struct Scope;

// A runtime-managed pointer. typedef for documentation purposes.
typedef void *ptr_t;

struct TaskFn {
    ptr_t (*func)(Context *cx, void *data);
    // this is NOT traced by the GC, so anything it refers to must be kept alive
    // by other roots.
    void *data;

    TaskFn() {}
    TaskFn(ptr_t (*f)(Context*, void*), void *d) : func(f), data(d) {}
};


// A context, used to interact with the runtime.
struct Context {
    friend class Scope;
    friend class Root;

  private:
    Context *parent_;           // our parent task
    size_t childno_;            // which child we are of our parent
    gc::Context *gc_context_;
    sched::Context *sched_context_;
    Root *roots_;
    bool failed_;

  public:
    static Context *init();
    // Should be called only on initial task, once completely finished.
    static void finish(Context *cx);

    ptr_t alloc(size_t size);
    ptr_t alloc(Root *dest, size_t size);

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
    void fail();

  private:
    static bool run_task(sched::Context *schedcx, bool was_stolen, void *data);

  private:
    Context() {}
    ~Context() {}
    NO_COPY(Context);
};


struct Scope {
    friend class Context;
    friend class Root;

  private:
    Context *context_;
    Root *saved_;

  public:
    explicit Scope(Context *cx)
        : context_(cx), saved_(cx->roots_)
    {}

    ~Scope() {
        context_->roots_ = saved_;
    }

  private:
    Scope();
    NO_COPY(Scope);
};


struct Root {
    friend class Context;
    friend class Scope;

  private:
    ptr_t value_;
    Root *next_;

  public:
    explicit Root(const Scope &scope, ptr_t init = NULL)
        : value_(init)
    {
        next_ = scope.context_->roots_;
        scope.context_->roots_ = this;
    }

    ptr_t get() { return value_; }
    void set(ptr_t v) { value_ = v; }

  private:
    Root();
    NO_COPY(Root);
};


} // namespace rt

#endif // RT_HPP_
