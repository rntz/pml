#ifndef CONFIG_HPP_
#define CONFIG_HPP_

#define GC_NEWSPACE_RATIO 2.0

// The minimum size of a "chunk" of memory used by the GC to allocate from.
#define MIN_CHUNK_SIZE 4096

// I think this can actually be 4 on amd64 even though sizeof(void*) is 8? But
// I should check that.
#define MACHINE_ALIGNMENT (sizeof(void*))

#endif // CONFIG_HPP_
