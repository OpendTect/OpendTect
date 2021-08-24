/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : 3-8-1994
-*/


#include "dirlist.h"

#include "filepath.h"
#include "perthreadrepos.h"

#ifndef OD_NO_QT
# include <QDir>
#endif

DirList::DirList( const char* dirname, File::DirListType t, const char* msk )
    : dir_(dirname?dirname:".")
    , type_(t)
    , mask_(msk)
{
    update();
}


DirList::DirList( const char* dirname, DirList::Type t, const char* msk )
    : dir_(dirname?dirname:".")
    , type_((DLType)t)
    , mask_(msk)
{
    update();
}


void DirList::update()
{
    erase();
    File::listDir( dir_, type_, *this, mask_ );
}


const char* DirList::fullPath( int idx ) const
{
    mDeclStaticString( ret );
    if ( idx < size() )
	ret = FilePath( dirName(), get(idx) ).fullPath();
    else
	ret.setEmpty();
    return ret.buf();
}
