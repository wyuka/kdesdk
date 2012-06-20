/****************************************************************************
** $Id: kdeaccountsformat.h 980058 2009-06-11 01:08:14Z dfaure $
**
** Created : 2001
**
** Copyright (C) 2001 Carsten Pfeiffer <pfeiffer@kde.org>
**
****************************************************************************/

#ifndef KDEACCOUNTSFORMAT_H
#define KDEACCOUNTSFORMAT_H

#include <kabc/format.h>

namespace KABC {
    class AddressBook;
}

class KDEAccountsFormat : public KABC::Format
{
public:
    KDEAccountsFormat() {}
    ~KDEAccountsFormat() {}

    virtual bool loadAll( KABC::AddressBook *,
                          KABC::Resource *resource, QFile *file );

    virtual bool load( KABC::Addressee&, QFile *)
    {
        qDebug("*** KDE Accounts format: load single entry not supported.");
        return false;
    }
    virtual void save( const KABC::Addressee&, QFile *)
    {
        qDebug("*** KDE Accounts format: save not supported.");
    }
    virtual void saveAll( KABC::AddressBook *, KABC::Resource *, QFile *)
    {
        qDebug("*** KDE Accounts format: save not supported.");
    }
    virtual bool checkFormat( QFile *file ) const
    {
        if ( file->fileName().endsWith( "/accounts" ) )
            return true; // lame, but works for me :)

        return false;
    }

};

#endif // KDEACCOUNTSFORMAT_H
