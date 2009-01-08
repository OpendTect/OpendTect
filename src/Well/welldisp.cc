/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : Aug 2003
-*/

static const char* rcsID = "$Id: welldisp.cc,v 1.5 2009-01-08 10:35:13 cvsbruno Exp $";

#include "welldisp.h"
#include "settings.h"
#include "keystrs.h"

static const char* sKeyDisplayPos = "DisplayPos";
static const char* sKeyShape = "Shape";
static const char* sKeyStyle = "Log Style";
static const char* sKeySingleColor = "Single Marker Color";
static const char* sKeyFill = "Fill Log";
static const char* sKeySingleCol = "Single Fill Color";
static const char* sKeyDataRange = "Data Range Bool";
static const char* sKeyOverlapp = "Log Overlapp";
static const char* sKeyRepeatLog = "Log Number";
static const char* sKeySeisColor = "Log Seismic Style Color";


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
    iop.getYN( IOPar::compKey(subjectName(),sKeySingleColor),
	     issinglecol_ );
}


void Well::DisplayProperties::Markers::doFillPar( IOPar& iop ) const
{
    iop.set( IOPar::compKey(subjectName(),sKeyShape),
	     circular_ ? "Circular" : "Square" );
    iop.setYN( IOPar::compKey(subjectName(),sKeySingleColor),
	     issinglecol_ );
}


void Well::DisplayProperties::Log::doUsePar( const IOPar& iop )
{
    iop.getYN( IOPar::compKey(subjectName(),sKeyStyle),
	       iswelllog_ );
    iop.getYN( IOPar::compKey(subjectName(),sKeyFill),
	       islogfill_ );
    iop.getYN( IOPar::compKey(subjectName(),sKeySingleCol),
	       issinglecol_ );
    iop.getYN( IOPar::compKey(subjectName(),sKeyDataRange),
	       isdatarange_ );
    iop.get( IOPar::compKey(subjectName(),sKeyRepeatLog),
	       repeat_ );
    iop.get( IOPar::compKey(subjectName(),sKeyOverlapp),
	       repeatovlap_ );
    iop.get( IOPar::compKey(subjectName(),sKeySeisColor),
	       seiscolor_ );
}


void Well::DisplayProperties::Log::doFillPar( IOPar& iop ) const
{
    iop.setYN( IOPar::compKey(subjectName(),sKeyStyle),
	       iswelllog_ );
    iop.setYN( IOPar::compKey(subjectName(),sKeyFill),
	       islogfill_ );
    iop.setYN( IOPar::compKey(subjectName(),sKeySingleCol),
	       issinglecol_ );
    iop.setYN( IOPar::compKey(subjectName(),sKeyDataRange),
	       isdatarange_ );
    iop.set( IOPar::compKey(subjectName(),sKeyRepeatLog),
	       repeat_ );
    iop.set( IOPar::compKey(subjectName(),sKeyOverlapp),
	       repeatovlap_ );
    iop.set( IOPar::compKey(subjectName(),sKeySeisColor),
	       seiscolor_ );
}


void Well::DisplayProperties::usePar( const IOPar& iop )
{
    track_.usePar( iop );
    markers_.usePar( iop );
    left_.usePar( iop );
    right_.usePar( iop );
}


void Well::DisplayProperties::fillPar( IOPar& iop ) const
{
    track_.fillPar( iop );
    markers_.fillPar( iop );
    left_.fillPar( iop );
    right_.fillPar( iop );
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
