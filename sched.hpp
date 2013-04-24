#ifndef SCHED_HPP_
#define SCHED_HPP_

#include <cstddef>

namespace sched {

struct Context;

struct TaskFn {
    // returns true if task succeeded, false if it failed
    bool (*func)(Context *cx, bool was_stolen, void *data);
    void *data;

    TaskFn() {}
    TaskFn(bool (*f)(Context*, bool, void*), void *d) : func(f), data(d) {}
};

Context *init(void);
void finish(Context *cx);       // call only when finishing initial context

// returns index of failing task or 2 if no failure
int fork(Context *cx, TaskFn fn1, TaskFn fn2);
int forkN(Context *cx, size_t n, TaskFn *fns);

} // namespace sched

#endif // SCHED_HPP_
