#ifndef KSPY_H
#define KSPY_H

#include <klibloader.h>

/**
 * Loader for the QObject debugging tool. The usage is simple, just call
 * KSpy::invoke(), then use the spy window to examine the state of your
 * QObjects.
 *
 * @author Richard Moore, rich@kde.org
 */
class KSpy
{
public:
  /**
   * Loads and invokes the KSpy utility.
   */
  static void invoke() {
    KLibLoader *loader = KLibLoader::self();
    KLibrary *lib = loader->library( "libkspy" );

    if ( !lib ) {
      qWarning( "Unable to load KSpy library\n" );
      return;
    }

    lib->factory(); // Ensure the factory is loaded

    // We don't need to do any more, KSpy is fired up by the loader hook
    // in the shared library.
  }

private:
  // Prevent instantiation.
  KSpy() {}
  ~KSpy() {}
};

#endif // KSPY_H
