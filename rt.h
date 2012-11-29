#ifndef RT_H_
#define RT_H_

#include <stddef.h>


/* ---------- Initialization etc. ---------- */
void rt_init();


/* ---------- Allocation & garbage collection ---------- */

/* A GC-managed pointer. Convenience typedef for documentation purposes. */
typedef void *gcptr_t;

/* Note that calling gc_alloc() may cause a GC cycle. Therefore any memory
 * allocated by a previous call to gc_alloc must be initialized before calling
 * gc_alloc() again.
 *
 * `find_roots_data' is passed to client_find_roots() if we do a GC cycle.
 */
gcptr_t gc_alloc(size_t size, void *find_roots_data);

/* Forces a GC cycle. */
void gc_run_cycle(void *find_roots_data);

/* Opaque data structure used to communicate with client program during a GC
 * cycle. */
typedef struct gc_ctx gc_ctx_t;

/* Registers roots during a GC cycle. Called by client_find_roots(). */
void gc_register_roots(gc_ctx_t *ctx, size_t nroots, gcptr_t *roots);

/* Traces pointers during a GC cycle. Called by client_trace_object(). */
void gc_trace_ptrs(gc_ctx_t *ctx, size_t nptrs, gcptr_t *ptrs);


/* ---------- Tasks ---------- */
typedef struct task task_t;     /* opaque */
typedef gcptr_t (task_fn_t)(gcptr_t data);

task_t *task_spawn(task_fn_t *func, gcptr_t data);
void *task_wait(task_t *task);
void task_cancel(task_t *task); /* asynchronous */

/* This isn't actually useful to client programs, but it's important to note
 * that the runtime will need to keep track internally of which task it's
 * currently executing.
 */
task_t *current_task();


/* ---------- Client program responsibilities ----------
 * These functions must be provided by the client program.
 */

/* Finds the roots for a particular task during a GC cycle. `ctx' is a GC
 * context specific to this GC cycle, used for communicating with the GC system.
 * `data' is the value `find_roots_data' passed to the function that caused GC.
 *
 * Must call gc_register_roots() to register roots. May call gc_register_roots()
 * more than once.
 *
 * Must not call gc_alloc().
 */
void client_find_roots(gc_ctx_t *ctx, void *data);

/* Traces a particular object, finding its outgoing pointers, during a GC cycle.
 * Must call gc_trace_ptrs() to trace pointers. May call gc_trace_ptrs() more
 * than once.
 *
 * Must not call gc_alloc().
 */
void client_trace_object(gc_ctx_t *ctx, gcptr_t ptr);

#endif // RT_H_
