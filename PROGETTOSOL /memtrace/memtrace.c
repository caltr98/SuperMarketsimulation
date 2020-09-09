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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <bfd.h>
#if defined(__linux__) || defined(__sun__)
#include <link.h>
#endif
#include "memtrace.h"

/*
 * Just in case anybody redefines these
 */

#ifdef free
#undef free
#endif

#ifdef malloc
#undef malloc
#endif

/*
 * Options
 */

static unsigned int Options;

#if defined(__linux__) && defined(__i386__)

/*
 * ----------------------------------------------------------------------
 * Backtrace. Stolen from glibc. Linux/i386 only.
 * ----------------------------------------------------------------------
 */

struct layout
{
  struct layout *next;
  void *return_address;
};

static int
backtrace (void **array, int size)
{
  /* We assume that all the code is generated with frame pointers set.  */
  register void *ebp __asm__ ("ebp");
  register void *esp __asm__ ("esp");
  struct layout *current;
  int cnt = 0;

  /* We skip the call to this function, it makes no sense to record it.  */
  current = (struct layout *) ebp;
  while (cnt < size)
    {
#if 0
      /*
       * My libc doesn't have __libc_stack_end
       */
      if ((void *) current < esp || (void *) current > __libc_stack_end)
        /* This means the address is out of range.  Note that for the
           toplevel we see a frame pointer with value NULL which clearly is
           out of range.  */
        break;
#else
      if ((void *) current < esp || !current || !current->return_address) {
	break;
      }
#endif

      array[cnt++] = current->return_address;
      current = current->next;
    }

  return cnt;
}

#else

/*
 * ----------------------------------------------------------------------
 * Backtrace. Contributed by Steve Coleman. May work elsewhere -- tested
 * on SunOS/Sparc only.
 * ----------------------------------------------------------------------
 */

static int
backtrace (void **array, int size)
{
  void *retAddr = NULL;
  unsigned int n = 0;

  while (1) {
    switch (n) {
    case  0: retAddr = __builtin_return_address(0);  break;
    case  1: retAddr = __builtin_return_address(1);  break;
    case  2: retAddr = __builtin_return_address(2);  break;
    case  3: retAddr = __builtin_return_address(3);  break;
    case  4: retAddr = __builtin_return_address(4);  break;
    case  5: retAddr = __builtin_return_address(5);  break;
    case  6: retAddr = __builtin_return_address(6);  break;
    case  7: retAddr = __builtin_return_address(7);  break;
    case  8: retAddr = __builtin_return_address(8);  break;
    case  9: retAddr = __builtin_return_address(9);  break;
    case 10: retAddr = __builtin_return_address(10); break;
    case 11: retAddr = __builtin_return_address(11); break;
    case 12: retAddr = __builtin_return_address(12); break;
    case 13: retAddr = __builtin_return_address(13); break;
    case 14: retAddr = __builtin_return_address(14); break;
    case 15: retAddr = __builtin_return_address(15); break;
    case 16: retAddr = __builtin_return_address(16); break;
    case 17: retAddr = __builtin_return_address(17); break;
    case 18: retAddr = __builtin_return_address(18); break;
    case 19: retAddr = __builtin_return_address(19); break;
    default: retAddr = NULL; break;
    }

    if (retAddr && n<size) {
      array[n++] = retAddr;
    }
    else {
      break;
    }
  }

  return n;
}

#endif

/*
 * ----------------------------------------------------------------------
 * Symbol Table Access. Stolen from binutils.
 * ----------------------------------------------------------------------
 */

static asymbol **thesyms;		/* Symbol table.  */
static bfd *thebfd = (bfd *) NULL;

static asymbol **
slurp_symtab (bfd * abfd)
{
  long storage;
  long symcount;
  asymbol **syms;

  if ((bfd_get_file_flags (abfd) & HAS_SYMS) == 0)
    return NULL;

  storage = bfd_get_symtab_upper_bound (abfd);
  if (storage < 0) {
    fprintf (stderr, "bfd_get_symtab_upper_bound failed: %s: %s\n",
	     bfd_get_filename(abfd), bfd_errmsg (bfd_get_error()));
    exit (1);
  }

  syms = (asymbol **) malloc (storage);

  symcount = bfd_canonicalize_symtab (abfd, syms);
  if (symcount < 0) {
    fprintf (stderr, "bfd_canonicalize_symtab: %s: %s\n",
	     bfd_get_filename(abfd), bfd_errmsg (bfd_get_error ()));
    exit (1);
  }

  return syms;
}

static int
init_symtab (const char *argv0)
{
  char **matching;

  if (thebfd) {
    return 0;
  }

  bfd_init ();
  /* set_default_bfd_target (); */

  thebfd = bfd_openr (argv0, NULL);
  if (thebfd == NULL) {
    fprintf (stderr, "bfd_openr failed for `%s': %s\n", argv0,
	     bfd_errmsg(bfd_get_error()));
    return -1;
  }

  if (bfd_check_format (thebfd, bfd_archive)) {
    fprintf (stderr, "can not get addresses from archive `%s': %s\n",
	     argv0, bfd_errmsg(bfd_get_error()));
    bfd_close (thebfd);
    thebfd = NULL;
    return -1;
  }

  if (!bfd_check_format_matches (thebfd, bfd_object, &matching)) {
    fprintf (stderr, "%s: %s\n", bfd_get_filename (thebfd),
	     bfd_errmsg (bfd_get_error ()));
    if (bfd_get_error () == bfd_error_file_ambiguously_recognized) {
      char **p = matching;
      fprintf (stderr, "%s: Matching formats:", bfd_get_filename (thebfd));
      while (*p) {
	fprintf (stderr, " %s", *p++);
      }
      fprintf (stderr, "\n");
      free (matching);
    }
    bfd_close (thebfd);
    thebfd = NULL;
    return -1;
  }

#if 0
  fprintf (stderr, "loading symbols from `%s' ... ", argv0);
  fflush  (stderr);
#endif
  thesyms = slurp_symtab (thebfd);
#if 0
  if (!thesyms) {
    fprintf (stderr, "(no symbols) ... ");
  }
  fprintf (stderr, "done.\n");
#endif

  return 0;
}

/*
 * ----------------------------------------------------------------------
 * Shared library stuff. Stolen from gdb. ELF only.
 * ----------------------------------------------------------------------
 */

#if defined(__linux__) || defined(__sun__)

struct section_table {
  PTR addr;               /* Lowest address in section */
  PTR endaddr;            /* 1+highest address in section */
  sec_ptr the_bfd_section;
  bfd *abfd;              /* BFD file pointer */
};

struct so_list {
  struct so_list * next;
  struct link_map lm;			/* copy of link map from inferior */
  struct link_map *lmaddr;		/* addr in inferior lm was read from */
  PTR lmend;
  char so_name[1024];
  bfd * abfd;
  struct section_table *sections;
  struct section_table *sections_end;
  struct section_table *textsection;
  asymbol ** syms;
};

#define LM_ADDR(so) ((so) -> lm.l_addr)
#define LM_NEXT(so) ((so) -> lm.l_next)
#define LM_NAME(so) ((so) -> lm.l_name)
/* Test for first link map entry; first entry is the exec-file. */
#define IGNORE_FIRST_LINK_MAP_ENTRY(x) ((x).l_prev == NULL)

static struct so_list *so_list_head = NULL;
static PTR debug_base;		/* Base of dynamic linker structures */

typedef struct {
  unsigned char d_tag[4];               /* entry tag value */
  union {
    unsigned char       d_val[4];
    unsigned char       d_ptr[4];
  } d_un;
} Elf32_External_Dyn;

static PTR
elf_locate_base (bfd * abfd)
{
  sec_ptr dyninfo_sect;
  int dyninfo_sect_size;
  PTR dyninfo_addr;
  char *buf;
  char *bufend;

  /* Find the start address of the .dynamic section.  */
  dyninfo_sect = bfd_get_section_by_name (abfd, ".dynamic");
  if (dyninfo_sect == NULL)
    return 0;
  dyninfo_addr = (void *) bfd_section_vma (abfd, dyninfo_sect);

#ifdef __sun__
  /*
   * XXX Sanity check. Is this relocation at work?
   */
  if ((unsigned long) dyninfo_addr == 1) {
    return 0;
  }
#endif

  /* Read in .dynamic section, silently ignore errors.  */
  dyninfo_sect_size = bfd_section_size (abfd, dyninfo_sect);
  buf = (char *) malloc (dyninfo_sect_size);
  memcpy (buf, dyninfo_addr, dyninfo_sect_size);

  /* Find the DT_DEBUG entry in the the .dynamic section.
     For mips elf we look for DT_MIPS_RLD_MAP, mips elf apparently has
     no DT_DEBUG entries.  */
  for (bufend = buf + dyninfo_sect_size;
       buf < bufend;
       buf += sizeof (Elf32_External_Dyn)) {
    Elf32_External_Dyn *x_dynp = (Elf32_External_Dyn *)buf;
    long dyn_tag;
    PTR dyn_ptr;
    
    dyn_tag = bfd_h_get_32 (abfd, (bfd_byte *) x_dynp->d_tag);
    if (dyn_tag == DT_NULL)
      break;
    else if (dyn_tag == DT_DEBUG)
      {
	dyn_ptr = (void *) bfd_h_get_32 (abfd, (bfd_byte *) x_dynp->d_un.d_ptr);
	return dyn_ptr;
      }
  }
  
  /* DT_DEBUG entry not found.  */
  return 0;
}

static struct r_debug debug_copy;

static struct link_map *
first_link_map_member ()
{
  struct link_map *lm = NULL;

  memcpy ((char *) &debug_copy, debug_base, sizeof (struct r_debug));
  /* FIXME:  Perhaps we should validate the info somehow, perhaps by
     checking r_version for a known version number, or r_state for
     RT_CONSISTENT. */
  lm = debug_copy.r_map;
  return lm;
}

static void
add_to_section_table (bfd * abfd, sec_ptr asect, PTR table_pp_char)
{
  struct section_table **table_pp = (struct section_table **)table_pp_char;
  flagword aflag;

  aflag = bfd_get_section_flags (abfd, asect);
  if (!(aflag & SEC_ALLOC))
    return;
  if (0 == bfd_section_size (abfd, asect))
    return;
  (*table_pp)->abfd = abfd;
  (*table_pp)->the_bfd_section = asect;
  (*table_pp)->addr = (PTR) bfd_section_vma (abfd, asect);
  (*table_pp)->endaddr = (PTR) ((char *) (*table_pp)->addr + bfd_section_size (abfd, asect));
  (*table_pp)++;
}

/* Builds a section table, given args BFD, SECTABLE_PTR, SECEND_PTR.
   Returns 0 if OK, 1 on error.  */

int
build_section_table (bfd * some_bfd,
		     struct section_table ** start,
		     struct section_table ** end)
{
  unsigned count;

  count = bfd_count_sections (some_bfd);
  if (*start)
    free ((PTR)*start);
  *start = (struct section_table *) malloc (count * sizeof (**start));
  *end = *start;
  bfd_map_over_sections (some_bfd, add_to_section_table, (char *)end);
  if (*end > *start + count) {
    fprintf (stderr, "build_section_table() oops\n");
    exit (1);
  }
  /* We could realloc the table, but it probably loses for most files.  */
  return 0;
}

static void
solib_map_sections (struct so_list *so)
{
  struct section_table *p;
  char *filename;
  char *scratch_pathname;
  bfd *abfd;
  
  filename = so -> so_name;
  scratch_pathname = strdup (filename);

  /* Leave scratch_pathname allocated.  abfd->name will point to it.  */

  abfd = bfd_openr (scratch_pathname, NULL);
  if (!abfd)
    {
      fprintf (stderr, "Could not open `%s' as an executable file: %s",
	       scratch_pathname, bfd_errmsg (bfd_get_error ()));
      return;
    }
  /* Leave bfd open, core_xfer_memory and "info files" need it.  */
  so -> abfd = abfd;

  strncpy (so->so_name, scratch_pathname, 1024);
  so->so_name[1023] = '\0';

  if (!bfd_check_format (abfd, bfd_object))
    {
      fprintf (stderr, "\"%s\": not in executable format: %s.",
	       scratch_pathname, bfd_errmsg (bfd_get_error ()));
      return;
    }

  if (build_section_table (abfd, &so -> sections, &so -> sections_end))
    {
      fprintf (stderr, "Can't find the file sections in `%s': %s", 
	       bfd_get_filename (abfd), bfd_errmsg (bfd_get_error ()));
      return;
    }

  for (p = so -> sections; p < so -> sections_end; p++)
    {
      /* Relocate the section binding addresses as recorded in the shared
	 object's file by the base address to which the object was actually
	 mapped. */
      p -> addr = (PTR) ((char *) p->addr + (size_t) LM_ADDR (so));
      p -> endaddr = (PTR) ((char *) p->endaddr + (size_t) LM_ADDR (so));
      if (p->endaddr > so->lmend) {
	so->lmend = p->endaddr;
      }
      if (strcmp (p -> the_bfd_section -> name, ".text") == 0)
	{
	  so -> textsection = p;
	}
    }

#if 0
  fprintf (stderr, "loading symbols from `%s' ... ", scratch_pathname);
  fflush  (stderr);
#endif
  so->syms = slurp_symtab (so->abfd);
#if 0
  if (!so->syms) {
    fprintf (stderr, "(no symbols) ... ");
  }
  fprintf (stderr, "done.\n");
#endif
}

static struct so_list *
find_solib (struct so_list *so_list_ptr)
{
  struct so_list *so_list_next = NULL;
  struct link_map *lm = NULL;
  struct so_list *newso;

  if (thebfd == NULL) {
    return NULL;
  }
  
  if (so_list_ptr == NULL)
    {
      /* We are setting up for a new scan through the loaded images. */
      if ((so_list_next = so_list_head) == NULL)
	{
	  /* We have not already read in the dynamic linking structures
	     from the inferior, lookup the address of the base structure. */
	  debug_base = elf_locate_base (thebfd);
	  if (debug_base != 0)
	    {
	      /* Read the base structure in and find the address of the first
		 link map list member. */
	      lm = first_link_map_member ();
	    }
	}
    }
  else
    {
      /* We have been called before, and are in the process of walking
	 the shared library list.  Advance to the next shared object. */
      if ((lm = LM_NEXT (so_list_ptr)) == NULL)
	{
	  /* We have hit the end of the list, so check to see if any were
	     added, but be quiet if we can't read from the target any more. */
	  memcpy ((char *) &(so_list_ptr -> lm),
		  so_list_ptr -> lmaddr,
		  sizeof (struct link_map));
	  lm = LM_NEXT (so_list_ptr);
	}
      so_list_next = so_list_ptr -> next;
    }
  if ((so_list_next == NULL) && (lm != NULL))
    {
      /* Get next link map structure from inferior image and build a local
	 abbreviated load_map structure */
      newso = (struct so_list *) malloc (sizeof (struct so_list));
      memset ((char *) newso, 0, sizeof (struct so_list));
      newso -> lmaddr = lm;
      /* Add the new node as the next node in the list, or as the root
	 node if this is the first one. */
      if (so_list_ptr != NULL)
	{
	  so_list_ptr -> next = newso;
	}
      else
	{
	  so_list_head = newso;
	}      
      so_list_next = newso;
      memcpy ((char *) &(newso -> lm), lm, sizeof (struct link_map));
      /* For SVR4 versions, the first entry in the link map is for the
	 inferior executable, so we must ignore it.  For some versions of
	 SVR4, it has no name.  For others (Solaris 2.3 for example), it
	 does have a name, so we can no longer use a missing name to
	 decide when to ignore it. */
      if (!IGNORE_FIRST_LINK_MAP_ENTRY (newso -> lm))
	{
	  strncpy (newso -> so_name, LM_NAME(newso), 1024 - 1);
	  newso -> so_name[1023] = '\0';
	  solib_map_sections (newso);
	}
    }
  return (so_list_next);
}

static struct so_list *
solib_address (PTR address)
{
  struct so_list *so = 0;   	/* link map state variable */
  
  while ((so = find_solib (so)) != NULL)
    {
      if (so -> so_name[0])
	{
	  if ((address >= (PTR) LM_ADDR (so)) &&
	      (address < (PTR) so -> lmend)) {
	    return so;
	  }
	}
    }
  return NULL;
}

#endif

/*
 * ----------------------------------------------------------------------
 * Search symbol. Stolen from binutils.
 * ----------------------------------------------------------------------
 */

static bfd_vma pc;
static const char *filename;
static const char *functionname;
static unsigned int line;
static int found;
static asymbol **cursyms;

/* Look for an address in a section.  This is called via
   bfd_map_over_sections.  */

static void
find_address_in_section (bfd * abfd, asection * section, PTR data)
{
  bfd_vma vma;

  if (found)
    return;

  if ((bfd_get_section_flags (abfd, section) & SEC_ALLOC) == 0)
    return;

  vma = bfd_get_section_vma (abfd, section);
  if (pc < vma)
    return;

  found = bfd_find_nearest_line (abfd, section, cursyms, pc - vma,
				 &filename, &functionname, &line);
}

static int
find_address (bfd * abfd, void * addr)
{
  /*
   * Search shared libs. Take relocation into account.
   */

#if defined(__linux__) || defined(__sun__)
  struct so_list * soptr = solib_address ((PTR) addr);
#endif

  found = 0;

#if defined(__linux__) || defined(__sun__)
  if (soptr && soptr->syms) {
    pc = (bfd_vma) ((char *) addr - (size_t) LM_ADDR(soptr));
    cursyms = soptr->syms;
    bfd_map_over_sections (soptr->abfd, find_address_in_section, (PTR) NULL);
  }
#endif

  /*
   * Try executable
   */

  if (!found) {
    pc = (bfd_vma) addr;
    cursyms = thesyms;
    bfd_map_over_sections (abfd, find_address_in_section, (PTR) NULL);
  }

  return found;
}

static void
print_address (bfd * abfd, void * addr, int fullname, FILE * out)
{
  if (!find_address (abfd, addr)) {
    fprintf (out, "[0x%lx]", (unsigned long) addr);
  }
  else {
    if (functionname == NULL || *functionname == '\0') {
      fprintf (out, "[0x%lx]", (unsigned long) addr);
    }
    else {
      fprintf (out, "%s", functionname);

      if (!fullname && filename) {
	char *h;

	h = strrchr (filename, '/');
	if (h != NULL)
	  filename = h + 1;
      }

      if (filename) {
	fprintf (out, "  (%s:%u)", filename, line);
      }
    }
  }
}

/*
 * ----------------------------------------------------------------------
 * MemTrace Stuff. Not stolen.
 * ----------------------------------------------------------------------
 */

/*
 * Since malloc is expensive, we don't want to permanently allocate
 * and free the structures we maintain. Instead, we keep some buckets
 * that hold a number of information blocks. 
 */

#define STACK_DEPTH	20	/* should be configurable */
#define BUCKET_SIZE	50
#define ALIGNMENT	8
#define MEMTRACE_MAGIC  100800884L

struct AllocInfo;

struct AllocTrace {
  struct AllocInfo * region;
  int depth;
  void * stack[STACK_DEPTH];
};

struct TraceBucket {
  int entries;
  unsigned char full[BUCKET_SIZE];
  struct AllocTrace entry[BUCKET_SIZE];
  struct TraceBucket * next;
};
  
struct AllocInfo {
  long magic;
  struct TraceBucket * bptr;
  int entry;
  size_t size;
};

static struct TraceBucket * head = (struct TraceBucket *) NULL;

/*
 * Internal Management
 */

static void
RecordAllocTrace (struct AllocInfo * info)
{
  struct TraceBucket * iter=head, * last=NULL;
  int i;

  while (iter) {
    if (iter->entries < BUCKET_SIZE) {
      break;
    }
    last = iter;
    iter = iter->next;
  }

  if (!iter) {
    iter = (struct TraceBucket *) malloc (sizeof (struct TraceBucket));
    if (!iter) {
      fprintf (stderr, "oops ");
      MemTrace_SelfTrace (stderr);
      _exit (1);
    }
    iter->entries = 0;
    iter->next = NULL;
    memset (iter->full, 0, BUCKET_SIZE * sizeof (unsigned char));
    
    if (last) {
      last->next = iter;
    }
    else {
      head = iter;
    }
  }

  for (i=0; i<BUCKET_SIZE; i++) {
    if (!iter->full[i]) {
      break;
    }
  }

  info->bptr = iter;
  info->entry = i;
  iter->entries++;
  iter->full[i] = 1;
  iter->entry[i].depth = backtrace (iter->entry[i].stack, STACK_DEPTH);
  iter->entry[i].region = info;
}

static void
ReleaseAllocTrace (struct AllocInfo * info)
{
  if (info->bptr) {
    info->bptr->entries--;
    info->bptr->full[info->entry] = 0;
  }
}

/*
 * Top two functions on the stack are MemTrace_Malloc and
 * RecordAllocTrace, so we start printing the third.
 */

static void
PrintTrace (struct AllocTrace * t, FILE * out)
{
  fprintf (out, "%lu bytes allocated by ",
	   (unsigned long) t->region->size);
  if (t->depth < 2) {
    fprintf (out, "??\n");
  }
  else {
    int i;
    if (!thebfd) {
      fprintf (out, "[0x%lx]", (unsigned long) t->stack[2]);
    }
    else {
      print_address (thebfd, t->stack[2],
		     Options & MEMTRACE_FULL_FILENAMES, out);
    }
    for (i=3; i<t->depth; i++) {
      fprintf (out, "\n        called by ");
      if (!thebfd) {
	fprintf (out, "[0x%lx]", (unsigned long) t->stack[i]);
      }
      else {
	print_address (thebfd, t->stack[i],
		       Options & MEMTRACE_FULL_FILENAMES, out);
      }
    }
    fprintf (out, "\n");
  }
}

/*
 * Admin Interface
 */

static void
ReportOnExit (void)
{
  if (Options & MEMTRACE_REPORT_ON_EXIT) {
    fprintf (stderr, "#### Memory Tracer exiting\n");
    MemTrace_Report (stderr);
  }
}

int
MemTrace_Init (const char * argv0, int theopts)
{
  char execfile[1024], *path, *ptr;
  struct stat sb;
  int count, res=-1;

  /*
   * Search executable from argv0 and PATH
   */

  if (!stat (argv0, &sb)) {
    res = init_symtab (argv0);
  }
  else {
    path = getenv ("PATH");

    while (path && *path) {
      for (count=0, ptr=execfile; count<1024 && *path != ':'; count++) {
	*ptr++ = *path++;
      }
      if (count<1024) {
	*ptr++ = '/';
	count++;
      }
      strncpy (ptr, argv0, 1024-count);
      execfile[1023] = '\0';
      
      if (!stat (execfile, &sb)) {
	res = init_symtab (execfile);
	break;
      }

      path++;
    }
  }

  Options = theopts;

  if (res == 0) {
    atexit (ReportOnExit);
  }

  return res;
}

int
MemTrace_Options (int options_to_set, int options_to_unset)
{
  int oldopts = Options;
  Options |= options_to_set;
  Options &= ~options_to_unset;
  return oldopts;
}

void
MemTrace_Report (FILE * out)
{
  size_t total=0, count=0;
  struct TraceBucket * iter=head;
  int i;

  /*
   * Load Shared Libraries
   */

#if defined(__linux__) || defined(__sun__)
  struct so_list * so = NULL;
  while ((so = find_solib (so)) != NULL);
#endif

  /*
   * Produce Report
   */

  fprintf (out, "#### Memory Usage Report\n");

  while (iter) {
    for (i=0; i<BUCKET_SIZE; i++) {
      if (iter->full[i]) {
	if (iter->entry[i].region->magic != MEMTRACE_MAGIC) {
	  fprintf (stderr, "MemTrace_Report: memory corruption detected.\n");
	  MemTrace_SelfTrace (stderr);
	  _exit (1);
	}
	total += iter->entry[i].region->size;
	count++;
	PrintTrace (&iter->entry[i], out);
      }
    }
    iter = iter->next;
  }

  fprintf (out, "#### a total of %lu bytes allocated in %lu chunks\n",
	   (unsigned long) total, (unsigned long) count);
}

void
MemTrace_Flush ()
{
  struct TraceBucket * iter=head;
  int i;

  while (iter) {
    for (i=0; i<BUCKET_SIZE; i++) {
      if (iter->full[i]) {
	iter->entry[i].region->bptr = NULL;
	iter->full[i] = 0;
	iter->entries--;
      }
    }
    iter = iter->next;
  }
}

/*
 * Custom backtraces
 */

int
MemTrace_Backtrace (void ** array, int size)
{
  return backtrace (array, size);
}

void
MemTrace_PrintTrace (void ** stack, int count, FILE * out)
{
  /*
   * Load Shared Libraries
   */

#if defined(__linux__) || defined(__sun__)
  struct so_list * so = NULL;
  while ((so = find_solib (so)) != NULL);
#endif

  fprintf (out, "at ");

  if (count < 1) {
    fprintf (out, "??\n");
  }
  else {
    int i;
    if (!thebfd) {
      fprintf (out, "[0x%lx]", (unsigned long) stack[0]);
    }
    else {
      print_address (thebfd, stack[0],
		     Options & MEMTRACE_FULL_FILENAMES, out);
    }
    for (i=1; i<count; i++) {
      fprintf (out, "\n        called by ");
      if (!thebfd) {
	fprintf (out, "[0x%lx]", (unsigned long) stack[i]);
      }
      else {
	print_address (thebfd, stack[i],
		       Options & MEMTRACE_FULL_FILENAMES, out);
      }
    }
    fprintf (out, "\n");
  }
}

void
MemTrace_SelfTrace (FILE * out)
{
  void * stack[STACK_DEPTH];
  int count;

  count = backtrace (stack, STACK_DEPTH);
  MemTrace_PrintTrace (stack, count, out);
}

/*
 * malloc() and free() replacements
 */

void *
MemTrace_Malloc (size_t size)
{
  unsigned char * r;
  struct AllocInfo * p = (struct AllocInfo *)
    malloc (size+sizeof(struct AllocInfo)+3*ALIGNMENT);
  if (!p) {
    fprintf (stderr, "panic in MemTrace_Malloc: malloc() returned NULL\n");
    MemTrace_SelfTrace (stderr);
    _exit (1);
  }
  p->magic = MEMTRACE_MAGIC;
  p->size  = size;
  if (!(Options & MEMTRACE_IGNORE_ALLOCATIONS)) {
    RecordAllocTrace (p);
  }
  else {
    p->bptr = NULL;
  }
  r  = (char *) p + sizeof (struct AllocInfo);
  r += ALIGNMENT - (sizeof(struct AllocInfo)%ALIGNMENT);
  if (!(Options & MEMTRACE_NO_INVALIDATE_MEMORY)) {
    memset (r, 0xaa, size+2*ALIGNMENT);
  }
  else {
    memset (r, 0xaa, ALIGNMENT);
    memset (r+size+ALIGNMENT, 0xaa, ALIGNMENT);
  }
  return (r+ALIGNMENT);
}

void
MemTrace_Free (void * ptr)
{
  struct AllocInfo * p;
  unsigned char *r, *i, *op;

  if (!ptr) {
    return;
  }

  op = (unsigned char *) ptr;
  r  = op - sizeof (struct AllocInfo) - ALIGNMENT;
  r -= ALIGNMENT - (sizeof(struct AllocInfo)%ALIGNMENT);
  p  = (struct AllocInfo *) r;

  if (p->magic != MEMTRACE_MAGIC) {
    fprintf (stderr, "MemTrace_Free: illegal argument or memory corrupted\n");
    MemTrace_SelfTrace (stderr);
    _exit (1);
  }

  for (i=op-ALIGNMENT; i<op; i++) {
    if (*i != 0xaa) {
      fprintf (stderr, "MemTrace_Free: lower fencepost touched\n");
      MemTrace_SelfTrace (stderr);
      fprintf (stderr, "This block of memory was allocated\n");
      PrintTrace (&p->bptr->entry[p->entry], stderr);
      _exit (1);
    }
  }

  for (i=op+p->size; i<op+p->size+ALIGNMENT; i++) {
    if (*i != 0xaa) {
      fprintf (stderr, "MemTrace_Free: upper fencepost touched\n");
      MemTrace_SelfTrace (stderr);
      fprintf (stderr, "This block of memory was allocated\n");
      PrintTrace (&p->bptr->entry[p->entry], stderr);
      _exit (1);
    }
  }

  ReleaseAllocTrace (p);
  if (!(Options & MEMTRACE_NO_INVALIDATE_MEMORY)) {
    memset (r, 0x55, p->size+sizeof(struct AllocInfo)+3*ALIGNMENT);
  }
  else {
    memset (op+p->size, 0x55, ALIGNMENT);
    memset (r, 0x55, sizeof(struct AllocInfo) + 2*ALIGNMENT);
  }
  free (p);
}

void *
MemTrace_Calloc (size_t nmemb, size_t size)
{
  void *ptr;
  int oldopts;
  oldopts  = Options;
  Options &= ~MEMTRACE_NO_INVALIDATE_MEMORY;
  ptr = MemTrace_Malloc (nmemb * size);
  memset (ptr, 0, nmemb * size);
  Options = oldopts;
  return ptr;
}

void *
MemTrace_Realloc (void * ptr, size_t nsize)
{
  struct AllocInfo * p;
  unsigned char *r, *i, *op;
  void * nptr;

  if (!ptr) {
    return MemTrace_Malloc (nsize);
  }

  if (!nsize) {
    MemTrace_Free (ptr);
    return NULL;
  }

  op = (unsigned char *) ptr;
  r  = op - sizeof (struct AllocInfo) - ALIGNMENT;
  r -= ALIGNMENT - (sizeof(struct AllocInfo)%ALIGNMENT);
  p  = (struct AllocInfo *) r;

  if (p->magic != MEMTRACE_MAGIC) {
    fprintf (stderr, "MemTrace_Realloc: illegal argument or memory corrupted\n");
    MemTrace_SelfTrace (stderr);
    _exit (1);
  }

  for (i=op-ALIGNMENT; i<op; i++) {
    if (*i != 0xaa) {
      fprintf (stderr, "MemTrace_Realloc: lower fencepost touched\n");
      MemTrace_SelfTrace (stderr);
      fprintf (stderr, "This block of memory was allocated\n");
      PrintTrace (&p->bptr->entry[p->entry], stderr);
      _exit (1);
    }
  }

  for (i=op+p->size; i<op+p->size+ALIGNMENT; i++) {
    if (*i != 0xaa) {
      fprintf (stderr, "MemTrace_Realloc: upper fencepost touched\n");
      MemTrace_SelfTrace (stderr);
      fprintf (stderr, "This block of memory was allocated\n");
      PrintTrace (&p->bptr->entry[p->entry], stderr);
      _exit (1);
    }
  }

  nptr = MemTrace_Malloc (nsize);
  memcpy (nptr, ptr, (nsize>p->size) ? p->size : nsize);
  MemTrace_Free (ptr);
  return nptr;
}

/*
 * Pointer verification
 */

int
MemTrace_CheckPtr (void * base, void * ptr, int failiffalse)
{
  struct AllocInfo * p;
  struct TraceBucket * iter;
  unsigned char * r;
  int i;

  if (!ptr) {
    if (failiffalse) {
      fprintf (stderr, "MemTrace_CheckPtr: called with null ptr\n");
      MemTrace_SelfTrace (stderr);
      _exit (1);
    }
    return 0;
  }

  if (base) {
    r  = (unsigned char *) base - sizeof (struct AllocInfo);
    r -= ALIGNMENT - (sizeof(struct AllocInfo)%ALIGNMENT);
    p  = (struct AllocInfo *) r;

    if (p->magic != MEMTRACE_MAGIC) {
      if (failiffalse) {
	fprintf (stderr, "MemTrace_CheckPtr: invalid base ptr or memory corrupted\n");
	MemTrace_SelfTrace (stderr);
	_exit (1);
      }
      return 0;
    }
    if ((char *) ptr < (char *) base ||
	(size_t) ((char *) ptr - (char *) base) >= p->size) {
      if (failiffalse) {
	fprintf (stderr, "MemTrace_CheckPtr: range check failed:\n");
	fprintf (stderr, "   pointer points %ld bytes into allocated area of %lu bytes\n",
		 (long) ((char *) ptr - (char *) base),
		 p->size);
	MemTrace_SelfTrace (stderr);
	fprintf (stderr, "This block of memory was allocated\n");
	PrintTrace (&p->bptr->entry[p->entry], stderr);
	_exit (1);
      }
      return 0;
    }
    return 1;
  }

  /*
   * Try to find some allocated region that contains ptr
   */

  for (iter=head; iter; iter=iter->next) {
    for (i=0; i<BUCKET_SIZE; i++) {
      if (iter->full[i]) {
	if (iter->entry[i].region->magic != MEMTRACE_MAGIC) {
	  fprintf (stderr, "MemTrace_CheckPtr: memory corruption detected\n");
	  MemTrace_SelfTrace (stderr);
	  _exit (1);
	}
	p  = iter->entry[i].region;
	r  = (unsigned char *) p + sizeof (struct AllocInfo);
	r += ALIGNMENT - (sizeof(struct AllocInfo)%ALIGNMENT);
	if ((unsigned char *) ptr > r &&
	    (size_t) ((unsigned char *) ptr - r) < p->size) {
	  return 1;
	}
      }
    }
  }

  if (failiffalse) {
    fprintf (stderr, "MemTrace_CheckPtr: pointer does not point to allocated memory\n");
    MemTrace_SelfTrace (stderr);
    _exit (1);
  }

  return 0;
}
