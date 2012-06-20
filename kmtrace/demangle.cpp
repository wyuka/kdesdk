#include <q3intdict.h>
//Added by qt3to4:
#include <stdio.h>
#include <qstringlist.h>
#include <q3strlist.h>
#include <qtextstream.h>
#include <q3sortedlist.h>
#include <qfile.h>
#include <q3tl.h>
#include <q3valuelist.h>
#include <stdlib.h>
#include <ktemporaryfile.h>
#include <kcomponentdata.h>
#include <kstandarddirs.h>
#include <kcmdlineargs.h>

extern "C" {
/* Options passed to cplus_demangle (in 2nd parameter). */

#define DMGL_NO_OPTS	0		/* For readability... */
#define DMGL_PARAMS	(1 << 0)	/* Include function args */
#define DMGL_ANSI	(1 << 1)	/* Include const, volatile, etc */
#define DMGL_JAVA	(1 << 2)	/* Demangle as Java rather than C++. */

#define DMGL_AUTO	(1 << 8)
#define DMGL_GNU	(1 << 9)
#define DMGL_LUCID	(1 << 10)
#define DMGL_ARM	(1 << 11)
#define DMGL_HP 	(1 << 12)       /* For the HP aCC compiler; same as ARM
                                           except for template arguments, etc. */
#define DMGL_EDG	(1 << 13)
#define DMGL_GNU_V3     (1 << 14)
#define DMGL_GNAT       (1 << 15)


extern char *cplus_demangle(const char *mangled, int options);
}


int main(int argc, char **argv)
{
   char buf[1024];

   while(!feof(stdin))
   {
      fgets(buf, 1024, stdin);
      QByteArray line = buf;
      line = line.trimmed();
      char *res = cplus_demangle(line.data(), DMGL_PARAMS | DMGL_AUTO | DMGL_ANSI );
      if (res)
      {
         printf("%s\n", res);
         free(res);
      }
      else
      {
         printf("%s\n", line.data());
      }
   }
}

