/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : Aug 2003
-*/

static const char* rcsID = "$Id: welldisp.cc,v 1.1 2008-12-05 12:58:58 cvsbert Exp $";

#include "welldisp.h"
#include "settings.h"
#include "keystrs.h"

static const char* sKeyDisplayPos = "DisplayPos";


void Well::DisplayProperties::BasicProps::usePar( const IOPar& iop )
{
    iop.get( IOPar::compKey(sIOParKey(),sKey::Color), color_ );
    iop.get( IOPar::compKey(sIOParKey(),sKey::Size), size_ );
}


void Well::DisplayProperties::BasicProps::fillPar( IOPar& iop ) const
{
    iop.set( IOPar::compKey(sIOParKey(),sKey::Color), color_ );
    iop.set( IOPar::compKey(sIOParKey(),sKey::Size), size_ );
}


void Well::DisplayProperties::Track::usePar( const IOPar& iop )
{
    iop.getYN( IOPar::compKey(sIOParKey(),sKeyDisplayPos),
	       dispabove_, dispbelow_ );
}


void Well::DisplayProperties::Track::fillPar( IOPar& iop ) const
{
    iop.setYN( IOPar::compKey(sIOParKey(),sKeyDisplayPos),
	       dispabove_, dispbelow_ );
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
