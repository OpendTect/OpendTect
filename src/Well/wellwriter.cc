/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : Aug 2003
-*/

static const char* rcsID = "$Id: wellwriter.cc,v 1.1 2003-08-21 15:47:15 bert Exp $";

#include "wellwriter.h"
#include "welldata.h"
#include "welltrack.h"
#include "welllog.h"
#include "welllogset.h"
#include "welld2tmodel.h"
#include "wellmarker.h"
#include "ascstream.h"
#include "errh.h"
#include "strmprov.h"
#include "keystrs.h"
#include "separstr.h"
#include "iopar.h"
#include <fstream>


Well::Writer::Writer( const char* f, const Well::Data& w )
	: Well::IO(f,false)
    	, wd(w)
{
}


bool Well::Writer::wrHdr( ostream& strm, const char* fileky ) const
{
    ascostream astrm( strm );
    if ( !astrm.putHeader(fileky) )
    {
	BufferString msg( "Cannot write to " );
	msg += fileky;
	msg += " file";
	ErrMsg( msg );
	return false;
    }
    return true;
}


bool Well::Writer::put() const
{
    return putInfo()
	&& putLogs()
	&& putMarkers()
	&& putD2T();
}


bool Well::Writer::putInfo() const
{
    StreamData sd = mkSD( sExtWell );
    if ( !sd.usable() ) return false;

    const bool isok = putInfo( *sd.ostrm );
    sd.close();
    return isok;
}


bool Well::Writer::putInfo( ostream& strm ) const
{
    if ( !wrHdr(strm,sKeyWell) ) return false;

    ascostream astrm( strm );
    astrm.put( Well::Info::sKeyuwid, wd.info().uwid );
    astrm.put( Well::Info::sKeyoper, wd.info().oper );
    astrm.put( Well::Info::sKeystate, wd.info().state );
    astrm.put( Well::Info::sKeycounty, wd.info().county );
    char str[80]; wd.info().surfacecoord.fill( str );
    astrm.put( Well::Info::sKeycoord, str );
    astrm.put( Well::Info::sKeyelev, wd.info().surfaceelev );
    astrm.newParagraph();

    return putTrack( strm );
}


bool Well::Writer::putTrack( ostream& strm ) const
{
    for ( int idx=0; idx<wd.track().size(); idx++ )
    {
	const Coord3& c = wd.track().pos(idx);
	strm << wd.track().dah(idx) << '\t';
	BufferString bs( c.x ); strm << bs << '\t';
	bs = c.y; strm << bs << '\t';
	bs = c.z; strm << bs << '\n';
    }
    return strm.good();
}


bool Well::Writer::putLogs() const
{
    for ( int idx=0; idx<wd.logs().size(); idx++ )
    {
	StreamData sd = mkSD( sExtLog, idx+1 );
	if ( !sd.usable() ) break;

	const Well::Log& wl = wd.logs().getLog(idx);
	if ( !putLog(*sd.ostrm,wl) )
	{
	    BufferString msg( "Could not write log: '" );
	    msg += wl.name();
	    msg += "'";
	    ErrMsg( msg );
	    sd.close();
	    return false;
	}
	sd.close();
    }

    return true;
}


bool Well::Writer::putLog( ostream& strm, const Well::Log& wl ) const
{
    if ( !wrHdr(strm,sKeyLog) ) return false;

    ascostream astrm( strm );
    astrm.put( sKey::Name, wl.name() );
    astrm.newParagraph();

    for ( int idx=0; idx<wl.size(); idx++ )
	strm << wl.dah(idx) << '\t' << wl.value(idx) << '\n';
    return true;
}


bool Well::Writer::putMarkers() const
{
    StreamData sd = mkSD( sExtMarkers );
    if ( !sd.usable() ) return false;

    const bool isok = putMarkers( *sd.ostrm );
    sd.close();
    return isok;
}


bool Well::Writer::putMarkers( ostream& strm ) const
{
    if ( !wrHdr(strm,sKeyMarkers) ) return false;

    ascostream astrm( strm );
    for ( int idx=0; idx<wd.markers().size(); idx++ )
    {
	BufferString basekey; basekey += idx+1;
	const Well::Marker& wm = *wd.markers()[idx];

	BufferString key = IOPar::compKey( basekey, sKey::Name );
	astrm.put( key, wm.name() );
	key = IOPar::compKey( basekey, sKey::Desc );
	FileMultiString fms;
	fms += wm.istop ? "T" : "B"; fms += wm.desc;
	astrm.put( key, fms );
	key = IOPar::compKey( basekey, sKey::Color );
	BufferString bs; wm.color.fill( bs.buf() );
	astrm.put( key, bs );
	key = IOPar::compKey( basekey, Well::Marker::sKeyDah );
	astrm.put( key, wm.dah );
    }

    return strm.good();
}


bool Well::Writer::putD2T() const
{
    if ( !wd.d2TModel() ) return true;

    StreamData sd = mkSD( sExtD2T );
    if ( !sd.usable() ) return false;

    const bool isok = putD2T( *sd.ostrm );
    sd.close();
    return isok;
}


bool Well::Writer::putD2T( ostream& strm ) const
{
    if ( !wrHdr(strm,sKeyD2T) ) return false;

    ascostream astrm( strm );
    const Well::D2TModel& d2t = *wd.d2TModel();
    astrm.put( sKey::Name, d2t.name() );
    astrm.put( sKey::Desc, d2t.desc );
    astrm.put( D2TModel::sKeyDataSrc, d2t.datasource );
    astrm.newParagraph();

    for ( int idx=0; idx<d2t.size(); idx++ )
	strm << d2t.dah(idx) << '\t' << d2t.t(idx) << '\n';
    return strm.good();
}
