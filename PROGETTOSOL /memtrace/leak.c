/*
 * This program does not make a lot of sense. It exists to demonstrate
 * the features of MemTrace
 */

/*
 * Use replacement functions
 */

#define malloc MemTrace_Malloc
#define free MemTrace_Free

#include <stdlib.h>
#include "memtrace.h"

void f (void)
{
  /*
   * Error: this memory is leaked
   */
  char * x = malloc (42);
}

int
main (int argc, char *argv[])
{
  char * x;

  MemTrace_Init (argv[0], MEMTRACE_REPORT_ON_EXIT);
  x = malloc (12);

  /*
   * Error: Attempt to access data beyond allocated area
   */

  if (!MemTrace_CheckPtr (x, x+12, 0)) { /* x+12 is invalid */
    printf ("pointer is not valid\n");
  }

  f ();

  free (x);

  /*
   * Ignore the following allocation
   */

  MemTrace_Options (MEMTRACE_IGNORE_ALLOCATIONS, 0);
  malloc (42);
  MemTrace_Options (0, MEMTRACE_IGNORE_ALLOCATIONS);

  /*
   * Produce a report. It should complain about the area allocated in f()
   */

  MemTrace_Report (stderr);

  /*
   * Free area a second time. This is a fatal error and causes the
   * program to terminate.
   */

  free (x);

  /*
   * Never reached
   */

  return 0;
}
