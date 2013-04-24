#ifndef UTIL_HPP_
#define UTIL_HPP_

#include <cassert>

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

#endif // UTIL_HPP_
