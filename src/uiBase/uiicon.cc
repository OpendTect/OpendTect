/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiicon.h"

#include "uipixmap.h"

#include "bufstringset.h"
#include "odiconfile.h"

#include <QIcon>

mUseQtnamespace

const char* uiIcon::None()
{
    return OD::IconFile::getIdentifier( OD::NoIcon );
}


uiIcon::uiIcon()
    : qicon_(*new QIcon)
{
}


uiIcon::uiIcon( const char* iconnm )
    : qicon_(*new QIcon)
    , srcnm_(iconnm)
{
    setIdentifier( iconnm );
}


uiIcon::uiIcon( const uiIcon& icn )
    : qicon_(*new QIcon(icn.qicon()))
{
}


uiIcon::~uiIcon()
{
    delete &qicon_;
}


void uiIcon::setIdentifier( const char* iconnm )
{
    const StringView iconstr = iconnm;
    if ( iconstr == None() )
    {
	if ( !qicon_.isNull() )
	    qicon_ = QIcon();
	return;
    }

    OD::IconFile icfile( iconnm );
    if ( !icfile.haveData() )
    {
	qicon_.addFile( OD::IconFile::notFoundIconFileName() );
	return;
    }

    const BufferStringSet& allfnms = icfile.fileNames();
    for ( int idx=0; idx<allfnms.size(); idx++ )
	qicon_.addFile( allfnms.get(idx).buf() );
}


bool uiIcon::isEmpty() const
{ return qicon_.isNull(); }


const char* uiIcon::source() const
{ return srcnm_.buf(); }
