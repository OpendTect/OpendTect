/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : Aug 2003
-*/

static const char* rcsID = "$Id: welldisp.cc,v 1.2 2008-12-05 14:43:58 cvsbert Exp $";

#include "welldisp.h"
#include "settings.h"
#include "keystrs.h"

static const char* sKeyDisplayPos = "DisplayPos";
static const char* sKeyShape = "Shape";


void Well::DisplayProperties::BasicProps::usePar( const IOPar& iop )
{
    iop.get( IOPar::compKey(subjectName(),sKey::Color), color_ );
    iop.get( IOPar::compKey(subjectName(),sKey::Size), size_ );
    doUsePar( iop );
}


void Well::DisplayProperties::BasicProps::fillPar( IOPar& iop ) const
{
    iop.set( IOPar::compKey(subjectName(),sKey::Color), color_ );
    iop.set( IOPar::compKey(subjectName(),sKey::Size), size_ );
    doFillPar( iop );
}


void Well::DisplayProperties::Track::doUsePar( const IOPar& iop )
{
    iop.getYN( IOPar::compKey(subjectName(),sKeyDisplayPos),
	       dispabove_, dispbelow_ );
}


void Well::DisplayProperties::Track::doFillPar( IOPar& iop ) const
{
    iop.setYN( IOPar::compKey(subjectName(),sKeyDisplayPos),
	       dispabove_, dispbelow_ );
}


void Well::DisplayProperties::Markers::doUsePar( const IOPar& iop )
{
    const char* res = iop.find( IOPar::compKey(subjectName(),sKeyShape) );
    if ( !res || !*res ) return;
    circular_ = *res != 'S';
}


void Well::DisplayProperties::Markers::doFillPar( IOPar& iop ) const
{
    iop.set( IOPar::compKey(subjectName(),sKeyShape),
	     circular_ ? "Circular" : "Square" );
}


void Well::DisplayProperties::usePar( const IOPar& iop )
{
    track_.usePar( iop );
    markers_.usePar( iop );
}


void Well::DisplayProperties::fillPar( IOPar& iop ) const
{
    track_.fillPar( iop );
    markers_.fillPar( iop );
}


Well::DisplayProperties& Well::DisplayProperties::defaults()
{
    static Well::DisplayProperties* ret = 0;

    if ( !ret )
    {
	Settings& setts = Settings::fetch( "welldisp" );
	ret = new DisplayProperties;
	ret->usePar( setts );
    }

    return *ret;
}


void Well::DisplayProperties::commitDefaults()
{
    Settings& setts = Settings::fetch( "welldisp" );
    defaults().fillPar( setts );
    setts.write();
}
