/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Aug 2003
-*/

static const char* rcsID mUnusedVar = "$Id$";

#include "wellwriter.h"
#include "welldata.h"
#include "welltrack.h"
#include "welllog.h"
#include "welllogset.h"
#include "welld2tmodel.h"
#include "wellmarker.h"
#include "welldisp.h"
#include "ascstream.h"
#include "errh.h"
#include "strmprov.h"
#include "keystrs.h"
#include "envvars.h"
#include <iostream>


Well::Writer::Writer( const char* f, const Well::Data& w )
	: Well::IO(f,false)
    	, wd(w)
	, binwrlogs_(false)
{
}


bool Well::Writer::wrHdr( std::ostream& strm, const char* fileky ) const
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
    return putInfoAndTrack()
	&& putLogs()
	&& putMarkers()
	&& putD2T()
	&& putCSMdl()
	&& putDispProps();
}


bool Well::Writer::putInfoAndTrack() const
{
    StreamData sd = mkSD( sExtWell() );
    if ( !sd.usable() ) return false;

    const bool isok = putInfoAndTrack( *sd.ostrm );
    sd.close();
    return isok;
}


bool Well::Writer::putInfoAndTrack( std::ostream& strm ) const
{
    if ( !wrHdr(strm,sKeyWell()) ) return false;

    ascostream astrm( strm );
    astrm.put( Well::Info::sKeyuwid(), wd.info().uwid );
    astrm.put( Well::Info::sKeyoper(), wd.info().oper );
    astrm.put( Well::Info::sKeystate(), wd.info().state );
    astrm.put( Well::Info::sKeycounty(), wd.info().county );
    if ( wd.info().surfacecoord != Coord(0,0) )
    {
	char str[80]; wd.info().surfacecoord.fill( str );
	astrm.put( Well::Info::sKeycoord(), str );
    }
    astrm.put( Well::Info::sKeyelev(), wd.info().surfaceelev );
    astrm.newParagraph();

    return putTrack( strm );
}


bool Well::Writer::putTrack( std::ostream& strm ) const
{
    for ( int idx=0; idx<wd.track().size(); idx++ )
    {
	const Coord3& c = wd.track().pos(idx);
	    // don't try to do the following in one statement
	    // (unless for educational purposes)
	strm << toString(c.x) << '\t';
	strm << toString(c.y) << '\t';
	strm << toString(c.z) << '\t';
	strm << wd.track().dah(idx) << '\n';
    }
    return strm.good();
}


bool Well::Writer::putTrack() const
{
    StreamData sd = mkSD( sExtTrack() );
    if ( !sd.usable() ) return false;

    const bool isok = putTrack( *sd.ostrm );
    sd.close();
    return isok;
}


bool Well::Writer::putLogs() const
{
    for ( int idx=0; idx<wd.logs().size(); idx++ )
    {
	StreamData sd = mkSD( sExtLog(), idx+1 );
	if ( !sd.usable() ) break;

	const Well::Log& wl = wd.logs().getLog(idx);
	if ( !putLog(*sd.ostrm,wl) )
	{
	    ErrMsg( BufferString("Could not write log: '",wl.name(),"'") );
	    sd.close();
	    return false;
	}
	sd.close();
    }

    return true;
}


bool Well::Writer::putLog( std::ostream& strm, const Well::Log& wl ) const
{
    if ( !wrHdr(strm,sKeyLog()) ) return false;

    ascostream astrm( strm );
    astrm.put( sKey::Name(), wl.name() );
    const bool haveunits = *wl.unitMeasLabel();
    const bool havepars = !wl.pars().isEmpty();
    if ( haveunits )
	astrm.put( Well::Log::sKeyUnitLbl(), wl.unitMeasLabel() );
    astrm.putYN( Well::Log::sKeyHdrInfo(), havepars );
    const char* stortyp = binwrlogs_ ? (__islittle__ ? "Binary" : "Swapped")
				     : "Ascii";
    astrm.put( Well::Log::sKeyStorage(), stortyp );
    astrm.newParagraph();
    if ( havepars )
	wl.pars().putTo( astrm );

    Interval<int> wrintv( 0, wl.size()-1 );
    float dah, val;
    for ( ; wrintv.start<wl.size(); wrintv.start++ )
    {
	dah = wl.dah(wrintv.start); val = wl.value(wrintv.start);
	if ( !mIsUdf(dah) && !mIsUdf(val) )
	    break;
    }
    for ( ; wrintv.stop>=0; wrintv.stop-- )
    {
	dah = wl.dah(wrintv.stop); val = wl.value(wrintv.stop);
	if ( !mIsUdf(dah) && !mIsUdf(val) )
	    break;
    }

    float v[2];
    for ( int idx=wrintv.start; idx<=wrintv.stop; idx++ )
    {
	v[0] = wl.dah(idx); v[1] = wl.value(idx);
	if ( mIsUdf(v[0]) )
	    continue;

	if ( binwrlogs_ )
	    strm.write( (char*)v, 2*sizeof(float) );
	else
	{
	    if ( mIsUdf(v[1]) )
		strm << v[0] << '\t' << sKey::FloatUdf() << '\n';
	    else
		strm << v[0] << '\t' << v[1] << '\n';
	}
    }

    return strm.good();
}


bool Well::Writer::putMarkers() const
{
    StreamData sd = mkSD( sExtMarkers() );
    if ( !sd.usable() ) return false;

    const bool isok = putMarkers( *sd.ostrm );
    sd.close();
    return isok;
}


bool Well::Writer::putMarkers( std::ostream& strm ) const
{
    if ( !wrHdr(strm,sKeyMarkers()) ) return false;

    ascostream astrm( strm );
    for ( int idx=0; idx<wd.markers().size(); idx++ )
    {
	BufferString basekey; basekey += idx+1;
	const Well::Marker& wm = *wd.markers()[idx];
	astrm.put( IOPar::compKey(basekey,sKey::Name()), wm.name() );
	astrm.put( IOPar::compKey(basekey,Well::Marker::sKeyDah()), wm.dah() );
	astrm.put( IOPar::compKey(basekey,sKey::StratRef()), wm.levelID() );
	BufferString bs; wm.color().fill( bs.buf() );
	astrm.put( IOPar::compKey(basekey,sKey::Color()), bs );
    }

    return strm.good();
}


bool Well::Writer::putD2T() const	{ return doPutD2T( false ); }
bool Well::Writer::putCSMdl() const	{ return doPutD2T( true ); }
bool Well::Writer::doPutD2T( bool csmdl ) const
{
    if ( (csmdl && !wd.checkShotModel()) || (!csmdl && !wd.d2TModel()) )
	return true;

    StreamData sd = mkSD( csmdl ? sExtCSMdl() : sExtD2T() );
    if ( !sd.usable() ) return false;

    const bool isok = doPutD2T( *sd.ostrm, csmdl );
    sd.close();
    return isok;
}


bool Well::Writer::putD2T( std::ostream& strm ) const
{ return doPutD2T( strm, false ); }
bool Well::Writer::putCSMdl( std::ostream& strm ) const
{ return doPutD2T( strm, true ); }
bool Well::Writer::doPutD2T( std::ostream& strm, bool csmdl ) const
{
    if ( !wrHdr(strm,sKeyD2T()) ) return false;

    ascostream astrm( strm );
    const Well::D2TModel& d2t = *(csmdl ? wd.checkShotModel(): wd.d2TModel());
    astrm.put( sKey::Name(), d2t.name() );
    astrm.put( sKey::Desc(), d2t.desc );
    astrm.put( D2TModel::sKeyDataSrc(), d2t.datasource );
    astrm.newParagraph();

    for ( int idx=0; idx<d2t.size(); idx++ )
	strm << d2t.dah(idx) << '\t' << d2t.t(idx) << '\n';
    return strm.good();
}


bool Well::Writer::putDispProps() const
{
    StreamData sd = mkSD( sExtDispProps() );
    if ( !sd.usable() ) return false;

    const bool isok = putDispProps( *sd.ostrm );
    sd.close();
    return isok;
}


bool Well::Writer::putDispProps( std::ostream& strm ) const
{
    if ( !wrHdr(strm,sKeyDispProps()) ) return false;

    ascostream astrm( strm );
    IOPar iop; 
    wd.displayProperties(true).fillPar( iop );
    wd.displayProperties(false).fillPar( iop );
    iop.putTo( astrm );
    return strm.good();
}
