#include "kdeaccountsformat.h"

#include <qbytearray.h>
#include <qfile.h>

#include <kabc/addressbook.h>
#include <kabc/addressee.h>

extern "C"
{
  KDE_EXPORT KABC::Format *format()
  {
    return new KDEAccountsFormat();
  }
}

/**
 * Loads addresses of the kde-common/accounts file-> The format is
 * pfeiffer    Carsten Pfeiffer               pfeiffer@kde.org
 */

bool KDEAccountsFormat::loadAll( KABC::AddressBook *book,
                              KABC::Resource *resource,
                              QFile *file )
{
    if ( !book || !file ) // eh?
        return false;

    QString uuid = "KDEAccountsEntry.";
    int id = 0;

    QByteArray array = file->readAll();
    file->close();

    QByteArray::ConstIterator it = array.begin();
    QByteArray::ConstIterator end = array.end();
    QByteArray::ConstIterator startLine;
    QString line;
    char eol = '\n';
    char delim = ' ';

    for ( ; it < end; it++ )
    {
        startLine = it;

        for ( ; it && it < end && *it != eol; it++ )
        { } // find eol

        uint length = it - startLine;
        line = QString::fromUtf8( startLine, length ).simplified();

        QString nickName;
        QString name;
        QString email;

        int firstSpace = line.indexOf( delim );
        if ( firstSpace > 0 )
        {
            nickName = line.left( firstSpace );

            int lastSpace = line.lastIndexOf( delim );
            if ( lastSpace > firstSpace )
            {
                email = line.mid( lastSpace +1 );

                int start = firstSpace + 1;
                int length = lastSpace - start;
                name = line.mid( start, length );

                if ( !email.isEmpty() )
                {
                    KABC::Addressee address;
                    address.setNickName( nickName  );
                    address.setNameFromString( name );
                    address.setOrganization("KDE Project");
                    address.insertCategory("KDE Developer");
                    address.insertEmail( email  );
                    address.setUid( uuid + QString::number( id++ ));

                    address.setResource( resource );
                    book->insertAddressee( address );
                }
            }
        }
    }

    return true;
}

