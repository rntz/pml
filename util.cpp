#include "util.hpp"

#include <cstdlib>
#include <cstdio>
#include <cstdint>
#include <cerrno>

namespace util {

void die() {
    abort();
}

void die(const char *format, ...) {
    va_list ap;
    va_start(ap, format);
    vdie(format, ap);
    va_end(ap);
}

void vdie(const char *format, va_list ap) {
    vfprintf(stderr, format, ap);
    die();
}

size_t page_size() {
    // TODO: this is wrong
    return 4096;
}

void *smalloc(size_t size) {
    void *p = malloc(size);
    if (p == NULL) die("out of memory");
    return p;
}

void *smemalign(size_t alignment, size_t size) {
    // round alignment up to the next power of two, as required for
    // posix_memalign.
    //
    // TODO: find a faster way to do this.
    size_t align = 1;
    while (align < alignment) {
        if (align >= (SIZE_MAX / 2)) {
            die("requested alignment is too large: %zu", size);
        }
        align <<= 1;
    }

    void *p;
    int err = posix_memalign(&p, align, size);
    if (err) {
        perror("posix_memalign");
        die();
    }
    assert (p != NULL);
    return p;
}

void sfree(void *p, size_t size) {
    free(p);
    (void) size;
}

} // namespace util
