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

#include "memtrace.h"

void *operator new (size_t sz)
{
  return MemTrace_Malloc (sz);
}

void *operator new[] (size_t sz)
{
  return MemTrace_Malloc (sz);
}

void operator delete (void *_p)
{
  if (!_p) {
    return;
  }
  MemTrace_Free (_p);
}

void operator delete[] (void *_p)
{
  if (!_p) {
    return;
  }
  MemTrace_Free (_p);
}
