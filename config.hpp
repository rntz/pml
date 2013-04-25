#ifndef CONFIG_HPP_
#define CONFIG_HPP_

#define GC_NEWSPACE_RATIO 2.0

// The minimum size of a "chunk" of memory used by the GC to allocate from.
#define MIN_CHUNK_SIZE 4096

#define MACHINE_ALIGNMENT (sizeof(void*))

#endif // CONFIG_HPP_
