#include "ktrace.h"
#include <stdlib.h>

class KTraceActivate 
{
public:
   KTraceActivate() { setenv("LD_PRELOAD","",1); ktrace(); }
   ~KTraceActivate() { kuntrace(); }
} kTraceActivateInstance;


