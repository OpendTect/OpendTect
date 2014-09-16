/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		September 2014
________________________________________________________________________

-*/

static const char* rcsID mUsedVar = "$Id$";


#include "uiicon.h"

#include "uipixmap.h"

#include "bufstringset.h"
#include "odiconfile.h"

#include <QIcon>

mUseQtnamespace

const char* uiIcon::save()		{ return "save"; }
const char* uiIcon::saveAs()		{ return "saveas"; }
const char* uiIcon::openObject()	{ return "open"; }
const char* uiIcon::newObject()		{ return "new"; }
const char* uiIcon::removeObject()	{ return "trashcan"; }
const char* uiIcon::None()		{ return "-"; }


uiIcon::uiIcon()
    : qicon_(*new QIcon)
{
}


uiIcon::uiIcon( const char* iconnm )
    : qicon_(*new QIcon)
    , srcnm_(iconnm)
{
    if ( srcnm_ == None() )
	return;

    setIdentifier( iconnm );
}


uiIcon::uiIcon( const uiPixmap& pm )
    : qicon_(*new QIcon(*pm.qpixmap()))
    , srcnm_(pm.source())
{
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
    OD::IconFile icfile( iconnm );
    const BufferStringSet& allfnms = icfile.fileNames();
    for ( int idx=0; idx<allfnms.size(); idx++ )
	qicon_.addFile( allfnms.get(idx).buf() );
}


bool uiIcon::isEmpty() const
{ return qicon_.isNull(); }


const char* uiIcon::source() const
{ return srcnm_.buf(); }
