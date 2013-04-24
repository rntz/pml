#include "rt.hpp"

#include <cstdio>
#include <cstdlib>

void *taskfunc(rt::Context *cx, void *data) {
    printf("hello from child %d\n", *((int*)&data));
    return NULL;
}

int main(int argc, char **argv) {
    rt::Context *cx = rt::Context::init();
    {
        rt::Scope scope(cx);
        rt::Root a(scope), b(scope);
        cx->fork(&a, &b,
                 rt::TaskFn(taskfunc, (void*)1),
                 rt::TaskFn(taskfunc, (void*)2));
        assert (a.get() == NULL && b.get() == NULL);
    }

    rt::Context::finish(cx);

    // done.
    (void) argc; (void) argv;
    return 0;
}

namespace gc {
namespace client {

void find_ptrs(CycleContext *cx, ptr_t object) {
    abort();
    (void) cx;
    (void) object;
}

} // namespace client
} // namespace gc
