/* More debugging hooks for `malloc'.
   Copyright (C) 1991,92,93,94,96,97,98,99,2000 Free Software Foundation, Inc.
		 Written April 2, 1991 by John Gilmore of Cygnus Support.
		 Based on mcheck.c by Mike Haertel.
		 Hacked by AK

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with this library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.

   The author may be reached (Email) at the address mike@ai.mit.edu,
   or (US mail) as Mike Haertel c/o Free Software Foundation.  */

#define _LIBC
#define MALLOC_HOOKS

#ifndef	_MALLOC_INTERNAL
#define	_MALLOC_INTERNAL
#include <malloc.h>
#include <mcheck.h>
#include <bits/libc-lock.h>
#endif

#include <dlfcn.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <stdio-common/_itoa.h>
#include <elf/ldsodefs.h>

#ifdef USE_IN_LIBIO
# include <libio/iolibio.h>
# define setvbuf(s, b, f, l) _IO_setvbuf (s, b, f, l)
#endif

#define TRACE_BUFFER_SIZE 512

static FILE *mallstream;
static const char mallenv[]= "MALLOC_TRACE";
static char malloc_trace_buffer[TRACE_BUFFER_SIZE];

__libc_lock_define_initialized (static, lock)

/* Address to breakpoint on accesses to... */
__ptr_t mallwatch;

/* File name and line number information, for callers that had
   the foresight to call through a macro.  */
char *_mtrace_file;
int _mtrace_line;

/* Old hook values.  */
static void (*tr_old_free_hook) __P ((__ptr_t ptr, const __ptr_t));
static __ptr_t (*tr_old_malloc_hook) __P ((__malloc_size_t size,
					   const __ptr_t));
static __ptr_t (*tr_old_realloc_hook) __P ((__ptr_t ptr,
					    __malloc_size_t size,
					    const __ptr_t));

#define TR_PIPELINE_SIZE	16
#define TR_BT_SIZE		80
#define TR_HASHTABLE_SIZE	9973

#define TR_NONE		0
#define TR_MALLOC	1
#define TR_REALLOC	2
#define TR_FREE		3

typedef struct {
   int op;
   __ptr_t ptr;
   __ptr_t old;
   __malloc_size_t size;
   int bt_size;
   void *bt[TR_BT_SIZE+1];
} tr_entry;

static tr_entry tr_pipeline[TR_PIPELINE_SIZE];
static int tr_pipeline_pos;
static void *tr_hashtable[TR_HASHTABLE_SIZE];


/* This function is called when the block being alloc'd, realloc'd, or
   freed has an address matching the variable "mallwatch".  In a debugger,
   set "mallwatch" to the address of interest, then put a breakpoint on
   tr_break.  */

void tr_break __P ((void));
void
tr_break ()
{
}

static void
tr_backtrace(void **bt, int size)
{
  char buf[20];
  int i;
  Dl_info info;
  for(i = 0; i < size; i++)
  {
     long hash = (((unsigned long)bt[i]) / 4) % TR_HASHTABLE_SIZE;
     if ((tr_hashtable[hash]!= bt[i]) && _dl_addr(bt[i], &info) && info.dli_fname  && *info.dli_fname)
     {
        if (bt[i] >= (void *) info.dli_saddr)
           sprintf(buf, "+%#lx", (unsigned long)(bt[i] - info.dli_saddr));
        else
           sprintf(buf, "-%#lx", (unsigned long)(info.dli_saddr - bt[i]));
        fprintf(mallstream, "%s%s%s%s%s[%p]\n",
		   info.dli_fname ?: "",
		   info.dli_sname ? "(" : "",
		   info.dli_sname ?: "",
		   info.dli_sname ? buf : "",
		   info.dli_sname ? ")" : "",
		   bt[i]);
	tr_hashtable[hash] = bt[i];
     }
     else {
        fprintf(mallstream, "[%p]\n", bt[i]);
     }
  } 
}

static void
inline
tr_log(const __ptr_t caller, __ptr_t ptr, __ptr_t old,  __malloc_size_t size, int op)
{
  switch(op)
  {
    case TR_REALLOC:
	break;
    case TR_MALLOC:
	break;
    case TR_FREE:
    {
      int i = tr_pipeline_pos;
      do {
	if (i) i--; else i = TR_PIPELINE_SIZE-1;
        if (tr_pipeline[i].ptr == ptr)
        {
           if (tr_pipeline[i].op == TR_MALLOC)
           {
              tr_pipeline[i].op = TR_NONE;
              tr_pipeline[i].ptr = NULL;
              return;
           }
           break;
        }
      } while (i != tr_pipeline_pos);
    }  
  }

  if (tr_pipeline[tr_pipeline_pos].op)
  {
      putc('@', mallstream);
      putc('\n', mallstream);
      /* Generate backtrace... 
       * We throw out the first frame (tr_mallochook)
       * end the last one (_start)
       */
      tr_backtrace(&(tr_pipeline[tr_pipeline_pos].bt[1]), 
		     tr_pipeline[tr_pipeline_pos].bt_size-2);
	
      switch(tr_pipeline[tr_pipeline_pos].op)
      {
	case TR_MALLOC:
	        fprintf (mallstream, "+ %p %#lx\n", 
 			tr_pipeline[tr_pipeline_pos].ptr, 
			(unsigned long int) tr_pipeline[tr_pipeline_pos].size);
		break;
	case TR_FREE:
	        fprintf (mallstream, "- %p\n", 
 			tr_pipeline[tr_pipeline_pos].ptr);
		break;
	case TR_REALLOC:
	        fprintf (mallstream, "< %p\n", 
 			tr_pipeline[tr_pipeline_pos].old);
	        fprintf (mallstream, "> %p %#lx\n", 
 			tr_pipeline[tr_pipeline_pos].ptr, 
			(unsigned long int) tr_pipeline[tr_pipeline_pos].size);
		break;
      }
  }

  tr_pipeline[tr_pipeline_pos].op = op; 
  tr_pipeline[tr_pipeline_pos].ptr = ptr; 
  tr_pipeline[tr_pipeline_pos].old = old; 
  tr_pipeline[tr_pipeline_pos].size = size; 
  tr_pipeline[tr_pipeline_pos].bt_size = backtrace(
	tr_pipeline[tr_pipeline_pos].bt, TR_BT_SIZE);
  tr_pipeline_pos++;
  if (tr_pipeline_pos == TR_PIPELINE_SIZE)
     tr_pipeline_pos = 0;
}

static void tr_freehook __P ((__ptr_t, const __ptr_t));
static void
tr_freehook (ptr, caller)
     __ptr_t ptr;
     const __ptr_t caller;
{
  if (ptr == NULL)
    return;
  __libc_lock_lock (lock);
  tr_log(caller, ptr, 0, 0, TR_FREE );
  __libc_lock_unlock (lock);
  if (ptr == mallwatch)
    tr_break ();
  __libc_lock_lock (lock);
  __free_hook = tr_old_free_hook;
  if (tr_old_free_hook != NULL)
    (*tr_old_free_hook) (ptr, caller);
  else
    free (ptr);
  __free_hook = tr_freehook;
  __libc_lock_unlock (lock);
}

static __ptr_t tr_mallochook __P ((__malloc_size_t, const __ptr_t));
static __ptr_t
tr_mallochook (size, caller)
     __malloc_size_t size;
     const __ptr_t caller;
{
  __ptr_t hdr;

  __libc_lock_lock (lock);

  __malloc_hook = tr_old_malloc_hook;
  if (tr_old_malloc_hook != NULL)
    hdr = (__ptr_t) (*tr_old_malloc_hook) (size, caller);
  else
    hdr = (__ptr_t) malloc (size);
  __malloc_hook = tr_mallochook;

  tr_log(caller, hdr, 0, size, TR_MALLOC);

  __libc_lock_unlock (lock);

  if (hdr == mallwatch)
    tr_break ();

  return hdr;
}

static __ptr_t tr_reallochook __P ((__ptr_t, __malloc_size_t, const __ptr_t));
static __ptr_t
tr_reallochook (ptr, size, caller)
     __ptr_t ptr;
     __malloc_size_t size;
     const __ptr_t caller;
{
  __ptr_t hdr;

  if (ptr == mallwatch)
    tr_break ();

  __libc_lock_lock (lock);

  __free_hook = tr_old_free_hook;
  __malloc_hook = tr_old_malloc_hook;
  __realloc_hook = tr_old_realloc_hook;
  if (tr_old_realloc_hook != NULL)
    hdr = (__ptr_t) (*tr_old_realloc_hook) (ptr, size, caller);
  else
    hdr = (__ptr_t) realloc (ptr, size);
  __free_hook = tr_freehook;
  __malloc_hook = tr_mallochook;
  __realloc_hook = tr_reallochook;

  tr_log(caller, hdr, ptr, size, TR_REALLOC);

  __libc_lock_unlock (lock);

  if (hdr == mallwatch)
    tr_break ();

  return hdr;
}


#ifdef _LIBC
extern void __libc_freeres (void);

/* This function gets called to make sure all memory the library
   allocates get freed and so does not irritate the user when studying
   the mtrace output.  */
static void
release_libc_mem (void)
{
  /* Only call the free function if we still are running in mtrace mode.  */
  if (mallstream != NULL)
    __libc_freeres ();
}
#endif


/* We enable tracing if either the environment variable MALLOC_TRACE
   is set, or if the variable mallwatch has been patched to an address
   that the debugging user wants us to stop on.  When patching mallwatch,
   don't forget to set a breakpoint on tr_break!  */

void
mtrace ()
{
#ifdef _LIBC
  static int added_atexit_handler;
#endif
  char *mallfile;

  /* Don't panic if we're called more than once.  */
  if (mallstream != NULL)
    return;

#ifdef _LIBC
  /* When compiling the GNU libc we use the secure getenv function
     which prevents the misuse in case of SUID or SGID enabled
     programs.  */
  mallfile = __secure_getenv (mallenv);
#else
  mallfile = getenv (mallenv);
#endif
  if (mallfile != NULL || mallwatch != NULL)
    {
      mallstream = fopen (mallfile != NULL ? mallfile : "/dev/null", "w");
      if (mallstream != NULL)
	{
	  /* Be sure it doesn't malloc its buffer!  */
	  setvbuf (mallstream, malloc_trace_buffer, _IOFBF, TRACE_BUFFER_SIZE);
	  fprintf (mallstream, "= Start\n");
	  tr_old_free_hook = __free_hook;
	  __free_hook = tr_freehook;
	  tr_old_malloc_hook = __malloc_hook;
	  __malloc_hook = tr_mallochook;
	  tr_old_realloc_hook = __realloc_hook;
	  __realloc_hook = tr_reallochook;

          tr_pipeline_pos = TR_PIPELINE_SIZE;
          for(;tr_pipeline_pos--;) 
	  {
            tr_pipeline[tr_pipeline_pos].op = TR_NONE;
            tr_pipeline[tr_pipeline_pos].ptr = NULL;
          }
          memset(tr_hashtable, 0, sizeof(void *)*TR_HASHTABLE_SIZE);
#ifdef _LIBC
	  if (!added_atexit_handler)
	    {
	      added_atexit_handler = 1;
	      atexit (release_libc_mem);
	    }
#endif
	}
    }
}

void
muntrace ()
{
  if (mallstream == NULL)
    return;

  fprintf (mallstream, "= End\n");
  fclose (mallstream);
  mallstream = NULL;
  __free_hook = tr_old_free_hook;
  __malloc_hook = tr_old_malloc_hook;
  __realloc_hook = tr_old_realloc_hook;
}


