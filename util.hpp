#ifndef UTIL_HPP_
#define UTIL_HPP_

#include <cassert>
#include <cstddef>
#include <cstdarg>

#ifdef NDEBUG
#define DEBUG(...)
#define DEBUG_EXPR(...) ((void)0)
#else
#define DEBUG(...) __VA_ARGS__
#define DEBUG_EXPR(...) ((void)(__VA_ARGS__))
#endif

#define DEBUG_BLOCK(...) do { DEBUG(__VA_ARGS__) } while (0)

// Avoids copy ctors. Put in a private section.
#define NO_COPY(classname) classname(const classname &that)

#define UNUSED __attribute__((__unused__))

#define MAX(x,y) ((x)>=(y) ? (x) : (y))
#define MIN(x,y) ((x)<=(y) ? (x) : (y))

#define ALIGNED(align, v) (!((v) % (align)))
#define ALIGN_DOWN(align, v) ((v) - ((v) % (align)))
#define ALIGN_UP(align, v)                                      \
    ((v) % (align) ? (v) + (align) - ((v) % (align)) : (v))

/* ---------- Utility functions ----------
 *
 * Most of this is just isolating platform-specific functionality behind the
 * smallest, cleanest interface we need to use it.
 */
namespace util {

void die();
void die(const char *format, ...);
void vdie(const char *format, va_list ap);

size_t page_size();

// Will die() rather than return NULL.
void *smalloc(size_t size);
void *smemalign(size_t alignment, size_t size);
void sfree(size_t size, void *ptr);

} // namespace util

#endif // UTIL_HPP_
