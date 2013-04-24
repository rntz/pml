#include "rt.hpp"

namespace runtime {

Context *Context::init() {
    Context *cx = new Context();
    cx->parent_ = NULL;
    cx->childno_ = 0;
    cx->gc_context_ = gc::init();
    cx->sched_context_ = sched::init();
    return cx;
}

void Context::finish(Context *cx) {
    assert (cx->parent_ == NULL && cx->childno_ == 0);
    assert (cx->roots_ == NULL);

    sched::finish(cx->sched_context_);
    gc::finish(cx->gc_context_);
    delete cx;
}

ptr_t Context::alloc(size_t size) {
    return gc::alloc(gc_context_, size, NULL);
}

ptr_t Context::alloc(Root *dest, size_t size) {
    ptr_t p = this->alloc(size);
    dest->set(p);
    return p;
}

struct TaskDesc {
    Context *context;
    Root *dest;
    TaskFn taskfn;

    TaskDesc() {}
    TaskDesc(Context *c, Root *d, TaskFn f) : context(c), dest(d), taskfn(f) {}
};

bool Context::run_task(sched::Context *schedcx, bool was_stolen, void *data) {
    TaskDesc *desc = (TaskDesc*)data;
    Root *dest = desc->dest;
    Context *cx = desc->context;
    TaskFn fn = desc->taskfn;

    if (!was_stolen) {
        assert (schedcx == cx->sched_context_);
        dest->set(fn.func(cx, fn.data));
        return cx->failed_;
    }
    else {
        // Create child context.
        assert (false);         // TODO FIXME
        (void) schedcx;
    }
}

int Context::fork(Root *ret1, Root *ret2, TaskFn fn1, TaskFn fn2) {
    TaskDesc desc1(this, ret1, fn1), desc2(this, ret2, fn2);
    return sched::fork(sched_context_,
                       sched::TaskFn(run_task, (void*) &desc1),
                       sched::TaskFn(run_task, (void*) &desc2));
}

int Context::forkN(size_t n, Root *rets, TaskFn *fns) {
    sched::TaskFn schedfns[n];
    TaskDesc descs[n];
    for (size_t i = 0; i < n; ++i) {
        descs[i].context = this;
        descs[i].dest = &rets[i];
        descs[i].taskfn = fns[i];
        schedfns[i] = sched::TaskFn(run_task, (void*) &descs[i]);
    }
    return sched::forkN(sched_context_, n, schedfns);
}

void Context::fail() { failed_ = true; }

} // namespace runtime
