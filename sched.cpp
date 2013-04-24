// Dummy scheduler implementation - serializes everything.

#include "sched.hpp"

namespace pml::sched {

Context *init() { return NULL; }
void finish(Context *cx) { (void) cx; }

int fork(Context *cx, TaskFn fn1, TaskFn fn2) {
    if (fn1.func(cx, false, fn1.data)) return 0;
    if (fn2.func(cx, false, fn2.data)) return 1;
    return 2;
}

int forkN(Context *cx, size_t n, TaskFn *fns) {
    for (size_t i = 0; i < n; ++i) {
        if (fns[i].func(cx, false, fns[i].data))
            return i;
    }
    return n;
}

}
