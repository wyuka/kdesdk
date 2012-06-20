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
#include <Q3CString>
#include <stdlib.h>
#include <ktemporaryfile.h>
#include <kcomponentdata.h>
#include <kstandarddirs.h>
#include <kcmdlineargs.h>
#include <kshell.h>

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

struct Entry {
  int base;
  int size;
  int signature;
  int count;
  int total_size;
  int backtrace[1];

  bool operator==(const Entry &e) { return total_size == e.total_size; }
  bool operator<(const Entry &e) { return total_size > e.total_size; }
};

Q3IntDict<Entry> *entryDict = 0;
Q3IntDict<char> *symbolDict = 0;
Q3IntDict<char> *formatDict = 0;
Q3SortedList<Entry> *entryList = 0;
Q3StrList *excludes = 0;

const char * const unknown = "<unknown>";
const char * const excluded = "<excluded>";
int allocCount = 0;
int leakedCount = 0;
int count = 0;
int maxCount;
int totalBytesAlloced = 0;
int totalBytesLeaked = 0;
int totalBytes = 0;
int maxBytes;

int fromHex(const char *str);
void parseLine(const QString &_line, char operation);
void dumpBlocks();

int fromHex(const char *str)
{
   if (*str == '[') str++;
   str += 2; // SKip "0x"
   return strtoll(str, NULL, 16);
}

// [address0][address1] .... [address] + base size
void parseLine(const QString &_line, char operation)
{
  char *line= (char *) _line.data();
  const char *cols[200];
  int i = 0;
  cols[i++] = line;
  while(*line)
  {
     if (*line == ' ')
     {
        *line = 0;
        line++;
        while (*line && (*line==' ')) line++;
        if (*line) cols[i++] = line;
     }
     else line++;
  }
  int cols_count = i;
  if (cols_count > 199) fprintf(stderr, "Error cols_count = %d\n", cols_count);
  if (cols_count < 4) return;
  switch (operation)
  {
   case '+':
   {
     Entry *entry = (Entry *) malloc((cols_count+3) *sizeof(int));
     entry->base = fromHex(cols[cols_count-2]);
     entry->size = fromHex(cols[cols_count-1]);
     int signature = 0;
     for(int i = cols_count-3; i--;)
     {
       signature += (entry->backtrace[i-1] = fromHex(cols[i]));
     }
     entry->signature = (signature / 4)+cols_count;
     entry->count = 1;
     entry->total_size = entry->size;
     entry->backtrace[cols_count-4] = 0;
     totalBytesAlloced += entry->size;
     totalBytes += entry->size;
     count++;
     if (totalBytes > maxBytes)
        maxBytes = totalBytes;
     if (count > maxCount)
        maxCount = count;
     if (entryDict->find(entry->base))
        fprintf(stderr, "\rAllocated twice: 0x%08x                    \n", entry->base);
     entryDict->replace(entry->base, entry);
   } break;
   case '-':
   {
     int base = fromHex(cols[cols_count-1]);
     Entry *entry = entryDict->take(base);
     if (!entry)
     {
	if (base)
           fprintf(stderr, "\rFreeing unalloacted memory: 0x%08x                   \n", base);
     }
     else
     {
        totalBytes -= entry->size;
        count--;
        free(entry);
     }
   } break;
   default:
     break;
  }
}

void sortBlocks()
{
   Q3IntDictIterator<Entry> it(*entryDict);
   for(;it.current(); ++it)
   {
      Entry *entry = it.current();
      totalBytesLeaked += entry->total_size;
      entryList->append(entry);
      for(int i = 0; entry->backtrace[i]; i++)
      {
         if (!symbolDict->find(entry->backtrace[i]))
             symbolDict->insert(entry->backtrace[i], unknown);
      }
   }
   entryList->sort();
}

void collectDupes()
{
   Q3IntDict<Entry> dupeDict;
   Q3IntDictIterator<Entry> it(*entryDict);
   for(;it.current();)
   {
      Entry *entry = it.current();
      ++it;
      Entry *entry2 = dupeDict.find(entry->signature);
      if (entry2)
      {
         entry2->count++;
         entry2->total_size += entry->size;
         entryDict->remove(entry->base);
      }
      else
      {
         dupeDict.insert(entry->signature, entry);
      }
   }
}

int lookupSymbols(FILE *stream)
{
  int i = 0;
  int symbols = 0;
  char line2[1024];
  while(!feof(stream))
  {
     fgets(line2, 1023, stream);
     if (line2[0] == '=' )
     {
         if(strcmp(line2,"= End") == 0 )
             break;
     }
     else if (line2[0] == '#')
         ;
     else if (line2[0] == '@')
         ;
     else if (line2[0] == '[')
         ;
     else if (line2[0] == '-')
         ;
     else if (line2[0] == '<')
         ;
     else if (line2[0] == '>')
         ;
     else if (line2[0] == '+')
     {
        i++;
        if (i & 1024)
        {
           fprintf(stderr, "\rLooking up symbols: %d found %d of %d symbols", i, symbols, symbolDict->count());
        }
     }
     else
     {
        char *addr = index(line2, '[');
        if (addr)
        {
           long i_addr = fromHex(addr);
           const char* str = symbolDict->find(i_addr);
           if (str == unknown)
           {
               *addr = 0;
               char* str;
               if( rindex(line2, '/') != NULL )
                   str = qstrdup(rindex(line2, '/')+1);
               else
                   str = qstrdup(line2);
               symbolDict->replace(i_addr, str);
               symbols++;
           }
        }
     }
  }
  fprintf(stderr, "\rLooking up symbols: %d found %d of %d symbols\n", i, symbols, symbolDict->count());
  return symbolDict->count()-symbols;
}

void lookupUnknownSymbols(const char *appname)
{
   KTemporaryFile inputFile;
   KTemporaryFile outputFile;
   inputFile.open();
   outputFile.open();
   QTextStream str ( &inputFile );
   Q3IntDict<char> oldDict = *symbolDict;
   Q3IntDictIterator<char> it(oldDict);
   for(;it.current(); ++it)
   {
       QString temp;
       temp.sprintf("%08lx\n", it.currentKey());
       str << temp;
   }
   str.flush();
   Q3CString command;
   command.sprintf("addr2line -e %s -f -C -s < %s > %s", appname,
	QFile::encodeName(KShell::quoteArg(inputFile.fileName())).data(),
	QFile::encodeName(KShell::quoteArg(outputFile.fileName())).data());
   system(command.data());
   FILE *fInputFile = fopen(QFile::encodeName(outputFile.fileName()), "r");
   if (!fInputFile)
   {
      fprintf(stderr, "Error opening temp file.\n");
      return;
   }
   Q3IntDictIterator<char> it2(oldDict);
   char buffer1[1024];
   char buffer2[1024];
   for(;it2.current(); ++it2)
   {
      if (feof(fInputFile))
      {
	fprintf(stderr, "Premature end of symbol output.\n");
        fclose(fInputFile);
        return;
      }
      if (!fgets(buffer1, 1023, fInputFile)) continue;
      if (!fgets(buffer2, 1023, fInputFile)) continue;
      buffer1[strlen(buffer1)-1]=0;
      buffer2[strlen(buffer2)-1]=0;
      Q3CString symbol(sizeof(buffer1) + sizeof(buffer2) + 3);
      symbol.sprintf("%s(%s)", buffer2, buffer1);
      if(*buffer1 != '?')
          symbolDict->replace(it2.currentKey(),qstrdup(symbol.data()));
   }
   fclose(fInputFile);
}

int match(const char *s1, const char *s2)
{
  register int result;
  while(true)
  {
    result = *s1 - *s2;
    if (result)
       return result;
    s1++;
    s2++;
    if (!*s2) return 0;
    if (!*s1) return -1;
  }
  return 0;
}

const char *lookupAddress(int addr)
{
   char *str = formatDict->find(addr);
   if (str) return str;
   QByteArray s = symbolDict->find(addr);
   if (s.isEmpty())
   {
fprintf(stderr, "Error!\n");
     exit(1);
   }
   else
   {
     int start = s.indexOf('(');
     int end = s.lastIndexOf('+');
     if (end < 0)
        end = s.lastIndexOf(')');
     if ((start > 0) && (end > start))
     {
       QByteArray symbol = s.mid(start+1, end-start-1);
       char *res = 0;
       if (symbol.indexOf(')') == -1)
           res = cplus_demangle(symbol.data(), DMGL_PARAMS | DMGL_AUTO | DMGL_ANSI );

       if (res)
       {
          symbol = res;
          free(res);
       }
       res = (char *) symbol.data();
       for(const char *it = excludes->first();it;it = excludes->next())
       {
          int i = match(res, it);
          if (i == 0)
          {
             formatDict->insert(addr,excluded);
             return excluded;
          }
       }
       s.replace(start+1, end-start-1, symbol);
     }
   }
   str = qstrdup(s.data());
   formatDict->insert(addr,str);
   return str;
}

void dumpBlocks()
{
   int filterBytes = 0;
   int filterCount = 0;
   for(Entry *entry = entryList->first();entry; entry = entryList->next())
   {
      for(int i = 0; entry->backtrace[i]; i++)
      {
         const char *str = lookupAddress(entry->backtrace[i]);
         if (str == excluded)
         {
            entry->total_size = 0;
            continue;
         }
      }
      if (!entry->total_size) continue;
      filterBytes += entry->total_size;
      filterCount++;
   }
   printf("Leaked memory after filtering: %d bytes in %d blocks.\n", filterBytes, filterCount);
   for(Entry *entry = entryList->first();entry; entry = entryList->next())
   {
      if (!entry->total_size) continue;
      printf("[%d bytes in %d blocks, 1st. block is %d bytes at 0x%08x] ", entry->total_size, entry->count, entry->size, entry->base);
      printf("\n");
      for(int i = 0; entry->backtrace[i]; i++)
      {
         const char *str = lookupAddress(entry->backtrace[i]);
         printf("   0x%08x %s\n", entry->backtrace[i], str);
      }
   }
}

struct TreeEntry
{
   int address;			// backtrace
   int total_size;
   int total_count;
   typedef QList < TreeEntry > TreeList;
   TreeList *subentries () const;
   mutable TreeList *_subentries;
   TreeEntry (int adr = 0, int size = 0, int count = 0, TreeList * sub = NULL );
   bool operator == (const TreeEntry &) const;
   bool operator < (const TreeEntry &) const;
};

typedef QList < TreeEntry > TreeList;

inline TreeEntry::TreeEntry (int adr, int size, int count, TreeList * sub)
 : address (adr), total_size (size), total_count (count), _subentries (sub)
{
}

inline bool TreeEntry::operator == (const TreeEntry & r) const
{				// this one is for QValueList
   return address == r.address;
}

inline
bool TreeEntry::operator < (const TreeEntry & r) const
{				// this one is for qBubbleSort() ... yes, ugly hack
   // the result is also reversed to get descending order
   return total_size > r.total_size;
}

inline TreeList * TreeEntry::subentries () const
{				// must be allocated only on-demand
   if (_subentries == NULL)
      _subentries = new TreeList;	// this leaks memory, but oh well
   return _subentries;
}

TreeList * treeList = 0;

void buildTree ()
{
   for (Entry * entry = entryList->first ();
	entry != NULL; entry = entryList->next ())
   {
      if (!entry->total_size)
	 continue;
      TreeList * list = treeList;
      int i;
      for (i = 0; entry->backtrace[i]; ++i)
	 ;			// find last (topmost) backtrace entry
      for (--i; i >= 0; --i)
      {
	 TreeList::Iterator pos = qFind(list->begin(), list->end(), entry->backtrace[i]);
	 if (pos == list->end ())
	 {
	    list->prepend (TreeEntry (entry->backtrace[i], entry->total_size,
				      entry->count));
	    pos = list->begin();
	 }
	 else
	    *pos = TreeEntry (entry->backtrace[i],
			  entry->total_size + (*pos).total_size,
			  entry->count + (*pos).total_count,
			  (*pos)._subentries);
	 list = (*pos).subentries ();
      }
   }
}

void processTree (TreeList * list, int threshold, int maxdepth, int depth)
{
   if (++depth > maxdepth && maxdepth > 0) // maxdepth <= 0 means no limit
      return;
   for (TreeList::Iterator it = list->begin (); it != list->end ();)
   {
      if ((*it).subentries ()->count () > 0)
	 processTree ((*it).subentries (), threshold, maxdepth, depth);
      if ((*it).total_size < threshold || (depth > maxdepth && maxdepth > 0))
      {
	 it = list->erase(it);
	 continue;
      }
      ++it;
   }
   qBubbleSort (*list);
}

void
dumpTree (const TreeEntry & entry, int level, char *indent, FILE * file)
{
   bool extra_ind = (entry.subentries ()->count () > 0);
   if(extra_ind)
      indent[level++] = '+';
   indent[level] = '\0';
   char savindent[2];
   const char * str = lookupAddress (entry.address);
   fprintf (file, "%s- %d/%d %s[0x%08x]\n", indent,
	    entry.total_size, entry.total_count, str, entry.address);
   if (level > 1)
   {
      savindent[0] = indent[level - 2];
      savindent[1] = indent[level - 1];
      if (indent[level - 2] == '+')
	 indent[level - 2] = '|';
      else if (indent[level - 2] == '\\')
	 indent[level - 2] = ' ';
   }
   int pos = 0;
   int last = entry.subentries ()->count() - 1;
   for (TreeList::ConstIterator it = entry.subentries ()->constBegin ();
	it != entry.subentries ()->constEnd (); ++it)
   {
      if (pos == last)
	 indent[level - 1] = '\\';
      dumpTree ((*it), level, indent, file);
      ++pos;
   }
   if (level > 1)
   {
      indent[level - 2] = savindent[0];
      indent[level - 1] = savindent[1];
   }
   if (extra_ind)
      --level;
   indent[level] = '\0';
}

void dumpTree (FILE * file)
{
   char indent[1024];
   indent[0] = '\0';
   for (TreeList::ConstIterator it = treeList->constBegin ();
	it != treeList->constEnd (); ++it)
      dumpTree (*it, 0, indent, file);
}

void createTree (const QString & treefile, int threshold, int maxdepth)
{
   FILE * file = fopen (treefile.toLocal8Bit(), "w");
   if (file == NULL)
   {
      fprintf (stderr, "Can't write tree file.\n");
      return;
   }
   treeList = new TreeList;
   buildTree ();
   processTree (treeList, threshold, maxdepth, 0);
   dumpTree (file);
   fclose (file);
}

void readExcludeFile(const char *name)
{
   FILE *stream = fopen(name, "r");
   if (!stream)
   {
      fprintf(stderr, "Error: Can't open %s.\n", name);
      exit(1);
   }
   char line[1024];
   while(!feof(stream))
   {
      if (!fgets(line, 1023, stream)) break;
      if ((line[0] == 0) || (line[0] == '#')) continue;
      line[strlen(line)-1] = 0;
      excludes->append(line);
   }
   fclose(stream);
   excludes->sort();
}

int main(int argc, char *argv[])
{
  KComponentData componentData("kmtrace");

  KCmdLineArgs::init(argc, argv, "kmtrace", 0, ki18n("kmtrace"), "v1.0", ki18n("KDE Memory leak tracer"));


  KCmdLineOptions options;

  options.add("x");

  options.add("exclude <file>", ki18n("File containing symbols to exclude from output"));

  options.add("e");

  options.add("exe <file>", ki18n("Executable to use for looking up unknown symbols"));

  options.add("+<trace-log>", ki18n("Log file to investigate"));

  options.add("t");

  options.add("tree <file>", ki18n("File to write allocations tree"));

  options.add("th");

  options.add("treethreshold <value>", ki18n("Do not print subtrees which allocated less than <value> memory"));

  options.add("td");

  options.add("treedepth <value>", ki18n("Do not print subtrees that are deeper than <value>"));

  KCmdLineArgs::addCmdLineOptions(options);

  KCmdLineArgs *args = KCmdLineArgs::parsedArgs();

  (void) args->count();
  const char *logfile;
  if(args->count())
    logfile = args->arg(0).toLocal8Bit();
  else
    logfile = "ktrace.out";

  QString exe = args->getOption("exe");
  QByteArray exclude;

  excludes = new Q3StrList;

  exclude = QFile::encodeName(KStandardDirs::locate("data", "kmtrace/kde.excludes"));
  if(!exclude.isEmpty())
      readExcludeFile(exclude);

  exclude = args->getOption("exclude").toLocal8Bit().data();
  if (!exclude.isEmpty())
  {
     fprintf(stderr, "Reading %s\n", exclude.data());
     readExcludeFile(exclude);
  }

  FILE *stream = fopen(logfile, "r");
  if (!stream)
  {
     fprintf(stderr, "Can't open %s\n", logfile);
     exit(1);
  }

  entryDict = new Q3IntDict<Entry>(9973);
  symbolDict = new Q3IntDict<char>(9973);
  formatDict = new Q3IntDict<char>(9973);
  entryList = new Q3SortedList<Entry>;

  fprintf(stderr, "Running\n");
  QString line;
  char line2[1024];
  while(!feof(stream))
  {
     fgets(line2, 1023, stream);
     line2[strlen(line2)-1] = 0;
     if (line2[0] == '=')
     {
	printf("%s\n", line2);
        if( strcmp( line2, "= End" ) == 0 )
           break;
     }
     else if (line2[0] == '#')
     {
       QByteArray app(line2+1);
       if(exe.isEmpty())
       {
         exe = app.trimmed();
         fprintf(stderr, "ktrace.out: malloc trace of %s\n", exe.data());
       }
       else if(!app.contains(exe.toLocal8Bit().data()))
       {
         fprintf(stderr, "trace file was for application '%s', not '%s'\n", app.data(), exe.data());
         exit(1);
       }
     }
     else if (line2[0] == '@')
        line = QString();
     else if (line2[0] == '[')
        line = line + ' ' + line2;
     else if (line2[0] == '+')
     {
        allocCount++;
        line = line + ' ' + line2;
        parseLine(line, '+');
        line = QString();
        if (allocCount & 128)
        {
           fprintf(stderr, "\rTotal long term allocs: %d still allocated: %d   ", allocCount, entryDict->count());
        }
     }
     else if (line2[0] == '-')
     {
        line = line + ' ' + line2;
        parseLine(line, '-');
        line = QString();
     }
     else if (line2[0] == '<')
     {
        line2[0] = '-';
        // First part of realloc (free)
        QString reline = line + ' ' + line2;
        parseLine(reline, '-');
     }
     else if (line2[0] == '>')
     {
        line2[0] = '+';
        // Second part of realloc (alloc)
        line = line + ' ' + line2;
        parseLine(line, '+');
        line = QString();
     }
     else
     {
        const char *addr = index(line2,'[');
        if (addr)
        {
           line = line + QChar(' ') + addr;
        }
     }
  }
  leakedCount = count;
  fprintf(stderr, "\rTotal long term allocs: %d still allocated: %d(%d)   \n", allocCount, leakedCount, entryDict->count());
  printf("Totals allocated: %d bytes in %d blocks.\n", totalBytesAlloced, allocCount);
  printf("Maximum allocated: %d bytes / %d blocks.\n", maxBytes, maxCount);
  fprintf(stderr, "Collecting duplicates...\n");
  collectDupes();
  fprintf(stderr, "Sorting...\n");
  sortBlocks();
  printf("Totals leaked: %d bytes in %d blocks.\n", totalBytesLeaked, leakedCount);
  fprintf(stderr, "Looking up symbols...\n");
  rewind(stream);
  lookupSymbols(stream);
  fprintf(stderr, "Looking up unknown symbols...\n");
  lookupUnknownSymbols(exe.toLocal8Bit());
  fprintf(stderr, "Printing...\n");
  dumpBlocks();
  QString treeFile = args->getOption ("tree");
  if (!treeFile.isEmpty ())
  {
      fprintf (stderr, "Creating allocation tree...\n");
      createTree (treeFile, args->getOption ("treethreshold").toInt (),
		  args->getOption ("treedepth").toInt ());
  }
  fprintf(stderr, "Done.\n");
  return 0;
}
