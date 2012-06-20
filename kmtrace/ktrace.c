/* More debugging hooks for `malloc'.
   Copyright (C) 1991,92,93,94,96,97,98,99,2000 Free Software Foundation, Inc.
		 Written April 2, 1991 by John Gilmore of Cygnus Support.
		 Based on mcheck.c by Mike Haertel.
		 Hacked by AK
		 Cleanup and performance improvements by
		 Chris Schlaeger <cs@kde.org>

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
   or (US mail) as Mike Haertel c/o Free Software Foundation.
*/

#define MALLOC_HOOKS
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <pthread.h>
#include <malloc.h>

#include <dlfcn.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <execinfo.h>
#include <unistd.h>

#ifdef USE_IN_LIBIO
# include <libio/iolibio.h>
# define setvbuf(s, b, f, l) _IO_setvbuf (s, b, f, l)
#endif

/* This is the most important parameter. It should be set to two times
 * the maximum number of mallocs the application uses at a time. Prime
 * numbers are very good candidates for this value.  I added a list of
 * some prime numbers for conveniance.
 *
 * 10007, 20011, 30013, 40031, 50033, 60037, 70039, 80051, 90053,
 * 100057, 110059, 120067, 130069, 140071, 150077, 160079, 170081,
 * 180097, 190121, 200131, 210139, 220141, 230143, 240151, 250153,
 * 260171, 270191, 280199, 290201, 300221, 310223, 320237, 330241,
 * 340261, 350281, 360287, 370373, 380377, 390389 */
#define TR_CACHE_SIZE 100057

/* The DELTA value is also a value for the maximum
 * number of iterations during a positive free/realloc
 * search. It must NOT divide TR_CACHE_SIZE without
 * remainder! */
#define DELTA 23

/* The high and low mark control the flushing algorithm.  Whenever the
 * hashtable reaches the high mark every DELTAth entry is written to
 * disk until the low filling mark is reached. A hash table becomes
 * very inefficient when it becomes filled to 50% or more. */
#define TR_HIGH_MARK ((int) (TR_CACHE_SIZE * 0.5))
#define TR_LOW_MARK ((int) (TR_HIGH_MARK - (TR_CACHE_SIZE / DELTA)))

/* Maximum call stack depth. No checking for overflows
 * is done. Adjust this value with care! */
#define TR_BT_SIZE 100

#define PROFILE 1

/* The hash function. Since the smallest allocated block is probably
 * not smaller than 8 bytes we ignore the last 3 LSBs. */
#define HASHFUNC(a) (((((unsigned long) a) << 1) ^ \
                     (((unsigned long) a) >> 3)) % \
                     TR_CACHE_SIZE)

#define TR_HASHTABLE_SIZE	9973

#define TR_NONE		0
#define TR_MALLOC	1
#define TR_REALLOC	2
#define TR_FREE		3

#define TRACE_BUFFER_SIZE 512

typedef struct
{
	void* ptr;
	size_t size;
	int bt_size;
	void** bt;
} tr_entry;

typedef struct CallerNode
{
	void* funcAdr;
	unsigned long mallocs;
	unsigned long mallocsSum;
	unsigned int noCallees;
	unsigned int maxCallees;
	struct CallerNode** callees;
} CallerNode;

void ktrace(void);
void kuntrace(void);

static void addAllocationToTree(void);

static void tr_freehook __P ((void*, const void*));
static void* tr_reallochook __P ((void*, size_t,
									const void*));
static void* tr_mallochook __P ((size_t, const void*));
/* Old hook values.  */
static void (*tr_old_free_hook) __P ((void* ptr, const void*));
static void* (*tr_old_malloc_hook) __P ((size_t size,
										   const void*));
static void* (*tr_old_realloc_hook) __P ((void* ptr,
											size_t size,
											const void*));

static FILE* mallstream;
static char malloc_trace_buffer[TRACE_BUFFER_SIZE];


/* Address to breakpoint on accesses to... */
void* mallwatch;

static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;


static int bt_size;
static void *bt[TR_BT_SIZE + 1];
static char tr_offsetbuf[20];
static tr_entry tr_cache[TR_CACHE_SIZE];
static int tr_cache_level;
static int tr_cache_pos;
static int tr_max_offset = 0;
static void *tr_hashtable[TR_HASHTABLE_SIZE];
#ifdef PROFILE
static unsigned long tr_mallocs = 0;
static unsigned long tr_logged_mallocs = 0;
static unsigned long tr_frees = 0;
static unsigned long tr_logged_frees = 0;
static unsigned long tr_current_mallocs = 0;
static unsigned long tr_max_mallocs = 0;
static unsigned long tr_flashes = 0;
static unsigned long tr_failed_free_lookups = 0;
static unsigned long tr_malloc_collisions = 0;
#endif

static CallerNode* CallTree = NULL;
static char* mallTreeFile = NULL;
static FILE* mallTreeStream = NULL;
static long mallThreshold = 2000;

extern __pid_t __fork (void);

/* This function is called when the block being alloc'd, realloc'd, or
 * freed has an address matching the variable "mallwatch".  In a
 * debugger, set "mallwatch" to the address of interest, then put a
 * breakpoint on tr_break.  */
void tr_break __P ((void));
void
tr_break()
{
}

__inline__ static void 
tr_backtrace(void **_bt, int size)
{
	int i;
	Dl_info info;
	for (i = 0; i < size; i++)
	{
		long hash = (((unsigned long)_bt[i]) / 4) % TR_HASHTABLE_SIZE;
		if ((tr_hashtable[hash]!= _bt[i]) && dladdr(_bt[i], &info) &&
			info.dli_fname  && *info.dli_fname)
		{
			if (_bt[i] >= (void *) info.dli_saddr)
				sprintf(tr_offsetbuf, "+%#lx", (unsigned long)
						(_bt[i] - info.dli_saddr));
			else
				sprintf(tr_offsetbuf, "-%#lx", (unsigned long)
						(info.dli_saddr - _bt[i]));
			fprintf(mallstream, "%s%s%s%s%s[%p]\n",
					info.dli_fname ?: "",
					info.dli_sname ? "(" : "",
					info.dli_sname ?: "",
					info.dli_sname ? tr_offsetbuf : "",
					info.dli_sname ? ")" : "",
					_bt[i]);
			tr_hashtable[hash] = _bt[i];
		}
		else
		{
			fprintf(mallstream, "[%p]\n", _bt[i]);
		}
	} 
}

__inline__ static void 
tr_log(const void* caller, void* ptr, void* old,
	   size_t size, int op)
{
	int i, offset;

	switch (op)
	{
	case TR_FREE:
		i = HASHFUNC(ptr);
		if ((offset = (i + tr_max_offset + 1)) >= TR_CACHE_SIZE)
			offset -= TR_CACHE_SIZE;
		do
		{
			if (tr_cache[i].ptr == ptr)
			{
				tr_cache[i].ptr = NULL;
				free(tr_cache[i].bt);
				tr_cache_level--;
				return;
			}
			if (++i >= TR_CACHE_SIZE)
				i = 0;
#ifdef PROFILE
			tr_failed_free_lookups++;
#endif
		} while (i != offset);

		/* We don't know this allocation, so it has been flushed to disk
		 * already. So flush free as well. */
		fprintf(mallstream, "@\n");
		bt_size = backtrace(bt, TR_BT_SIZE);
		tr_backtrace(&(bt[1]), bt_size - 2);
		fprintf(mallstream, "- %p\n", ptr);
#ifdef PROFILE
		tr_logged_frees++;
#endif
		return;

	case TR_REALLOC:
		/* If old is 0 it's actually a malloc. */
		if (old)
		{
			i = HASHFUNC(old);
 			if ((offset = (i + tr_max_offset + 1)) >= TR_CACHE_SIZE)
				offset -= TR_CACHE_SIZE;
			do
			{
				if (tr_cache[i].ptr == old)
				{
					int j = HASHFUNC(ptr);
					/* We move the entry otherwise the free will be
					 * fairly expensive due to the wrong place in the
					 * hash table. */
					tr_cache[i].ptr = NULL;
					for ( ; ; )
					{
						if (tr_cache[j].ptr == NULL)
							break;

						if (++j >= TR_CACHE_SIZE)
							i = 0;
					}
					tr_cache[j].ptr = ptr;
					if (ptr)
					{
						tr_cache[j].size = tr_cache[i].size;
						tr_cache[j].bt_size = tr_cache[i].bt_size;
						tr_cache[j].bt = tr_cache[i].bt;
					}
					else
						tr_cache_level--;
					tr_cache[i].size = size;
					return;
				}
				if (++i >= TR_CACHE_SIZE)
					i = 0;
			} while (i != offset);

			fprintf(mallstream, "@\n");
			bt_size = backtrace(bt, TR_BT_SIZE);
			tr_backtrace(&(bt[1]), bt_size - 2);
			fprintf(mallstream, "< %p\n", old);
			fprintf(mallstream, "> %p %#lx\n", ptr,
					(unsigned long) size);
			return;
		}

	case TR_MALLOC:
		if (tr_cache_level >= TR_HIGH_MARK)
		{
			/* The hash table becomes ineffective when the high mark has
			 * been reached. We still need some more experience with
			 * the low mark. It's unclear what reasonable values are. */
#ifdef PROFILE
			tr_flashes++;
#endif
			i = HASHFUNC(ptr);
			do
			{
				if (tr_cache[i].ptr)
				{
#ifdef PROFILE
					tr_logged_mallocs++;
#endif
					fprintf(mallstream, "@\n");
					tr_backtrace(&(tr_cache[i].bt[1]),
								 tr_cache[i].bt_size - 2);
					fprintf(mallstream, "+ %p %#lx\n",
							tr_cache[i].ptr, 
							(unsigned long int)
							tr_cache[i].size);
					tr_cache[i].ptr = NULL;
					tr_cache_level--;
				}
				if ((i += DELTA) >= TR_CACHE_SIZE)
					i -= TR_CACHE_SIZE;
			} while (tr_cache_level > TR_LOW_MARK);
		}

		i = HASHFUNC(ptr);
		for ( ; ; )
		{
			if (tr_cache[i].ptr == NULL)
				break;

			if (++i >= TR_CACHE_SIZE)
				i = 0;
#ifdef PROFILE
			tr_malloc_collisions++;
#endif
		}
		if ((offset = (i  - HASHFUNC(ptr))) < 0)
			offset += TR_CACHE_SIZE;
		if (offset > tr_max_offset)
			tr_max_offset = offset;

		tr_cache[i].ptr = ptr; 
		tr_cache[i].size = size;
		tr_cache[i].bt = (void**) malloc(TR_BT_SIZE * sizeof(void*));
		tr_cache[i].bt_size = backtrace(
			tr_cache[i].bt, TR_BT_SIZE);
		tr_cache[i].bt = realloc(tr_cache[i].bt, tr_cache[i].bt_size * sizeof(void*));
		tr_cache_level++;

		return;

	case TR_NONE:
		if (tr_cache[tr_cache_pos].ptr)
		{
#ifdef PROFILE
			tr_logged_mallocs++;
#endif
			fprintf(mallstream, "@\n");
			tr_backtrace(&(tr_cache[tr_cache_pos].bt[1]),
						 tr_cache[tr_cache_pos].bt_size - 2);
			fprintf(mallstream, "+ %p %#lx\n", 
					tr_cache[tr_cache_pos].ptr, 
					(unsigned long int)
					tr_cache[tr_cache_pos].size);
			tr_cache[tr_cache_pos].ptr = NULL;
			free(tr_cache[tr_cache_pos].bt);
			tr_cache_level--;
		}

		if (++tr_cache_pos >= TR_CACHE_SIZE)
			tr_cache_pos = 0;
		break;
	}
}

static void
tr_freehook (ptr, caller)
     void* ptr;
     const void* caller;
{
	if (ptr == NULL)
		return;
	if (ptr == mallwatch)
		tr_break ();

        pthread_mutex_lock(&lock);
#ifdef PROFILE
	tr_frees++;
	tr_current_mallocs--;
#endif

	__free_hook = tr_old_free_hook;

	if (tr_old_free_hook != NULL)
		(*tr_old_free_hook) (ptr, caller);
	else
		free(ptr);
	tr_log(caller, ptr, 0, 0, TR_FREE);

	__free_hook = tr_freehook;
        pthread_mutex_unlock(&lock);
}

static void*
tr_mallochook (size, caller)
     size_t size;
     const void* caller;
{
	void* hdr;

        pthread_mutex_lock(&lock);

	__malloc_hook = tr_old_malloc_hook;
	__realloc_hook = tr_old_realloc_hook;
	__free_hook = tr_old_free_hook;

	if (tr_old_malloc_hook != NULL)
		hdr = (void*) (*tr_old_malloc_hook) (size, caller);
	else
		hdr = (void*) malloc(size);
	tr_log(caller, hdr, 0, size, TR_MALLOC);
	/* We only build the allocation tree if mallTreeFile has been set. */
	if (mallTreeFile)
		addAllocationToTree();

	__malloc_hook = tr_mallochook;
	__realloc_hook = tr_reallochook;
	__free_hook = tr_freehook;

#ifdef PROFILE
	tr_mallocs++;
	tr_current_mallocs++;
	if (tr_current_mallocs > tr_max_mallocs)
		tr_max_mallocs = tr_current_mallocs;
#endif
        pthread_mutex_unlock(&lock);

	if (hdr == mallwatch)
		tr_break ();

	return hdr;
}

static void*
tr_reallochook (ptr, size, caller)
     void* ptr;
     size_t size;
     const void* caller;
{
	void* hdr;

	if (ptr == mallwatch)
		tr_break ();

        pthread_mutex_lock(&lock);

	__free_hook = tr_old_free_hook;
	__malloc_hook = tr_old_malloc_hook;
	__realloc_hook = tr_old_realloc_hook;

	if (tr_old_realloc_hook != NULL)
		hdr = (void*) (*tr_old_realloc_hook) (ptr, size, caller);
	else
		hdr = (void*) realloc (ptr, size);

	tr_log(caller, hdr, ptr, size, TR_REALLOC);

	__free_hook = tr_freehook;
	__malloc_hook = tr_mallochook;
	__realloc_hook = tr_reallochook;

#ifdef PROFILE
	/* If ptr is 0 there was no previos malloc of this location */
	if (ptr == NULL)
	{
		tr_mallocs++;
		tr_current_mallocs++;
		if (tr_current_mallocs > tr_max_mallocs)
			tr_max_mallocs = tr_current_mallocs;
	}
#endif

        pthread_mutex_unlock(&lock);

	if (hdr == mallwatch)
		tr_break ();

	return hdr;
}

void
addAllocationToTree(void)
{
	int bt_size;
	int i, j;
	void *bt[TR_BT_SIZE + 1];
	CallerNode* cn = CallTree;
	CallerNode** parent = &CallTree;
	
	bt_size = backtrace(bt, TR_BT_SIZE);
	for (i = bt_size - 1; i >= 4; i--)
	{
		if (cn == NULL)
		{
			*parent = cn = (CallerNode*) malloc(sizeof(CallerNode));
			cn->funcAdr = bt[i];
			cn->mallocs = 0;
			cn->noCallees = 0;
			cn->maxCallees = 0;
			cn->callees = NULL;
		}
		if (i == 4)
			cn->mallocs++;
		else
		{
			int knownCallee = 0;
			for (j = 0; j < cn->noCallees; j++)
				if (bt[i - 1] == cn->callees[j]->funcAdr)
				{
					parent = &cn->callees[j];
					cn = cn->callees[j];
					knownCallee = 1;
					break;
				}
			if (!knownCallee)
			{
				if (cn->noCallees == cn->maxCallees)
				{
					/* Copy callees into new, larger array. */
					CallerNode** tmp;
					int newSize = 2 * cn->maxCallees;
					if (newSize == 0)
						newSize = 4;
					tmp = (CallerNode**) malloc(newSize * sizeof(CallerNode*));
					memcpy(tmp, cn->callees,
						   cn->maxCallees * sizeof(CallerNode*));
					if (cn->callees)
						free(cn->callees);
					cn->callees = tmp;
					memset(&cn->callees[cn->maxCallees], 0,
						   (newSize - cn->maxCallees) * sizeof(CallerNode*));
					cn->maxCallees = newSize;
				}
				parent = &cn->callees[cn->noCallees++];
				cn = 0;
			}
		}
	}
}

static int
removeBranchesBelowThreshold(CallerNode* root)
{
	int i;
	int max;

	if (!root)
		return (0);
	for (i = 0; i < root->noCallees; i++)
	{
		if (removeBranchesBelowThreshold(root->callees[i]))
		{
			free(root->callees[i]);
			if (root->noCallees > 1)
			{
				root->callees[i] = root->callees[root->noCallees - 1];
				root->callees[root->noCallees - 1] = 0;
			}
			else if (root->noCallees == 1)
				root->callees[i] = 0;
			
			root->noCallees--;
			i--;
		}
	}
	if (root->noCallees == 0 && root->mallocs < mallThreshold )
		return (1);

	return (0);
}

static void
dumpCallTree(CallerNode* root, char* indentStr, int rawMode)
{
	int i;
	Dl_info info;
	char* newIndentStr = 0;
	size_t indDepth = 0;

	if (!root || !mallTreeStream)
		return;

	if (rawMode)
	{
		fprintf(mallTreeStream, "-");
	}
	else
	{
		newIndentStr = (char*) malloc(strlen(indentStr) + 2);
		strcpy(newIndentStr, indentStr);
		if (root->noCallees > 0)
			strcat(newIndentStr, "+");
		indDepth = strlen(newIndentStr);
		fprintf(mallTreeStream, "%s- ", newIndentStr);
	}

	if (dladdr(root->funcAdr, &info) && info.dli_fname  && *info.dli_fname)
	{
		if (root->funcAdr >= (void *) info.dli_saddr)
			sprintf(tr_offsetbuf, "+%#lx", (unsigned long)
					(root->funcAdr - info.dli_saddr));
		else
			sprintf(tr_offsetbuf, "-%#lx", (unsigned long)
					(info.dli_saddr - root->funcAdr));
		fprintf(mallTreeStream, "%s%s%s%s%s[%p]",
				info.dli_fname ?: "",
				info.dli_sname ? "(" : "",
				info.dli_sname ?: "",
				info.dli_sname ? tr_offsetbuf : "",
				info.dli_sname ? ")" : "",
				root->funcAdr);
	}
	else
	{
		fprintf(mallTreeStream, "[%p]", root->funcAdr);
	}
	fprintf(mallTreeStream, ": %lu\n", root->mallocs);
	if (indDepth > 1 && !rawMode)
	{
		if (newIndentStr[indDepth - 2] == '+')
			newIndentStr[indDepth - 2] = '|';
		else if (newIndentStr[indDepth - 2] == '\\')
			newIndentStr[indDepth - 2] = ' ';
	}

	for (i = 0; i < root->noCallees; i++)
	{
		if (rawMode)
			dumpCallTree(root->callees[i], "", 1);
		else
		{
			if (i == root->noCallees - 1)
				newIndentStr[indDepth - 1] = '\\';
			dumpCallTree(root->callees[i], newIndentStr, rawMode);
		}
	}
	if (rawMode)
		fprintf(mallTreeStream, ".\n");
	else
		free(newIndentStr);
	
}

#ifdef _LIBC
extern void __libc_freeres (void);

/* This function gets called to make sure all memory the library
 * allocates get freed and so does not irritate the user when studying
 * the mtrace output.  */
static void
release_libc_mem (void)
{
	/* Only call the free function if we still are running in mtrace
	 * mode. */
	/*if (mallstream != NULL)
		__libc_freeres ();*/

	kuntrace();
        write(2, "kuntrace()\n", 11); 
}
#endif

/* We enable tracing if either the environment variable MALLOC_TRACE
 * or the variable MALLOC_TREE are set, or if the variable mallwatch
 * has been patched to an address that the debugging user wants us to
 * stop on.  When patching mallwatch, don't forget to set a breakpoint
 * on tr_break! */
void
ktrace()
{
#ifdef _LIBC
	static int added_atexit_handler;
#endif
	char* mallfile;

	/* Don't panic if we're called more than once.  */
	if (mallstream != NULL)
		return;

#ifdef _LIBC
	/* When compiling the GNU libc we use the secure getenv function
	 * which prevents the misuse in case of SUID or SGID enabled
     * programs.  */
	mallfile = __secure_getenv("MALLOC_TRACE");
	mallTreeFile = __secure_getenv("MALLOC_TREE");
	if( __secure_getenv("MALLOC_THRESHOLD") != NULL )
	    mallThreshold = atol(__secure_getenv("MALLOC_THRESHOLD"));
#else
	mallfile = getenv("MALLOC_TRACE");
	mallTreeFile = getenv("MALLOC_TREE");
	if( getenv("MALLOC_THRESHOLD") != NULL )
	    mallThreshold = atol(getenv("MALLOC_THRESHOLD"));
#endif
	if (mallfile != NULL || mallTreeFile != NULL || mallwatch != NULL)
    {
		mallstream = fopen (mallfile != NULL ? mallfile : "/dev/null", "w");
		if (mallstream != NULL)
		{
			char buf[512];
            
			/* Be sure it doesn't malloc its buffer!  */
			setvbuf (mallstream, malloc_trace_buffer, _IOFBF,
					 TRACE_BUFFER_SIZE);
			fprintf (mallstream, "= Start\n");
			memset(buf, 0, sizeof(buf));
			readlink("/proc/self/exe", buf, sizeof(buf));
			if(*buf)
				fprintf (mallstream, "#%s\n", buf);

			/* Save old hooks and hook in our own functions for all
			 * malloc, realloc and free calls */
			tr_old_free_hook = __free_hook;
			__free_hook = tr_freehook;
			tr_old_malloc_hook = __malloc_hook;
			__malloc_hook = tr_mallochook;
			tr_old_realloc_hook = __realloc_hook;
			__realloc_hook = tr_reallochook;

			tr_cache_pos = TR_CACHE_SIZE;
			do
			{
				tr_cache[--tr_cache_pos].ptr = NULL;
			} while (tr_cache_pos);
			tr_cache_level = 0;

			memset(tr_hashtable, 0, sizeof(void*) * TR_HASHTABLE_SIZE);
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
kuntrace()
{
	if (mallstream == NULL)
		return;

	/* restore hooks to original values */
	__free_hook = tr_old_free_hook;
	__malloc_hook = tr_old_malloc_hook;
	__realloc_hook = tr_old_realloc_hook;

	if (removeBranchesBelowThreshold(CallTree))
		CallTree = 0;
	if (mallTreeFile)
	{
		if (mallTreeStream = fopen(mallTreeFile, "w"))
		{
			dumpCallTree(CallTree, "", 0);
			fclose(mallTreeStream);
		}
	}

	/* Flush cache. */
	while (tr_cache_level)
		tr_log(NULL, 0, 0, 0, TR_NONE);

	fprintf (mallstream, "= End\n");
#ifdef PROFILE
	fprintf(mallstream, "\nMax Mallocs:    %8ld   Cache Size:   %8ld"
			"   Flashes:      %8ld\n"
			"Mallocs:        %8ld   Frees:        %8ld   Leaks:        %8ld\n"
			"Logged Mallocs: %8ld   Logged Frees: %8ld   Logged Leaks: %8ld\n"
			"Avg. Free lookups: %ld  Malloc collisions: %ld  Max offset: %ld\n",
			tr_max_mallocs, TR_CACHE_SIZE, tr_flashes,
			tr_mallocs, tr_frees, tr_current_mallocs,
			tr_logged_mallocs, tr_logged_frees,
			tr_logged_mallocs - tr_logged_frees,
			tr_frees > 0 ? ( tr_failed_free_lookups / tr_frees ) : 0,
			tr_malloc_collisions, tr_max_offset);
#endif
	fclose (mallstream);
	mallstream = NULL;
	write(2, "kuntrace()\n", 11);
}

int fork()
{
  int result;
  if (mallstream)
     fflush(mallstream);
  result = __fork();
  if (result == 0)
  {
    if (mallstream)
    {
      close(fileno(mallstream));
      mallstream = NULL;
      __free_hook = tr_old_free_hook;
      __malloc_hook = tr_old_malloc_hook;
      __realloc_hook = tr_old_realloc_hook;
    }
  }
  return result;
}


static int my_mcount_lock = 0;
void mcount()
{
   Dl_info info;
   int i = 1;
   if (my_mcount_lock) return;
   my_mcount_lock = 1;
   bt_size = backtrace(bt, TR_BT_SIZE);
#if 0
for(i = 1; (i < 5) && (i < bt_size); i++)
{
#endif
   if (dladdr(bt[i], &info) &&	info.dli_fname  && *info.dli_fname)
   {
	fprintf(stdout, "%s\n", info.dli_sname ? info.dli_sname : "");
   }
   else
   {
	fprintf(stdout, "[%p]\n", bt[i]);
   }
#if 0
}
   fprintf(stdout, "\n");
#endif   
   my_mcount_lock = 0;
}

