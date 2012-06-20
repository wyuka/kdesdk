#ifndef _KTRACE_H
#define _KTRACE_H

extern "C" {
/* Activate a standard collection of tracing hooks.  */
extern void ktrace (void);
extern void kuntrace (void);
}

#endif /* ktrace.h */
