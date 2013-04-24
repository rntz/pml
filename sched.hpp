#ifndef SCHED_HPP_
#define SCHED_HPP_

#include <cstddef>
#include <cstdbool>

namespace pml::sched {

struct Context;

struct TaskFn {
    // returns true if task succeeded, false if it failed
    bool (*func)(Context *cx, bool was_stolen, void *data);
    void *data;
};

Context *init(void);
void finish(Context *cx);       // call only when finishing initial context

// returns index of failing task or 2 if no failure
int fork(TaskFn fn1, TaskFn fn2);
int forkN(size_t n, TaskFn *fns);

} // namespace pml::sched

#endif // SCHED_HPP_
