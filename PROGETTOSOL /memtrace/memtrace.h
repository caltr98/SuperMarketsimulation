/*
 * ----------------------------------------------------------------------
 *
 * Memory Tracer 
 *
 * Print a backtrace for each allocated chunk of memory.
 *
 * (w) 1999 Frank Pilhofer
 *
 * ----------------------------------------------------------------------
 */

#ifndef __MEMTRACE_H__
#define __MEMTRACE_H__

#include <stdio.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MEMTRACE_REPORT_ON_EXIT        (1<<0)
#define MEMTRACE_FULL_FILENAMES        (1<<1)
#define MEMTRACE_NO_INVALIDATE_MEMORY  (1<<2)
#define MEMTRACE_IGNORE_ALLOCATIONS    (1<<3)

/*
 * Admin interface
 */

int    MemTrace_Init      (const char *, int);
int    MemTrace_Options   (int, int);
void   MemTrace_Report    (FILE *);
void   MemTrace_Flush     (void);

/*
 * Obtaining and printing backtraces yourself
 */

int    MemTrace_Backtrace (void **, int);
void   MemTrace_PrintTrace(void **, int, FILE *);
void   MemTrace_SelfTrace (FILE *);

/*
 * malloc() and free() replacements
 */

void * MemTrace_Malloc    (size_t);
void * MemTrace_Calloc    (size_t, size_t);
void * MemTrace_Realloc   (void *, size_t);
void   MemTrace_Free      (void *);

/*
 * Verify that a pointer points into an allocated region
 */

int    MemTrace_CheckPtr  (void *, void *, int);

/*
 * Shortcut
 */

#define MemTrace_AssertPtr(p) MemTrace_CheckPtr(NULL, p, 1)

#ifdef __cplusplus
}
#endif

#endif
