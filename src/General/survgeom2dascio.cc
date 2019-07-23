/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Satyaki Maitra
 * DATE     : September 2010
-*/

#include "survgeom2dascio.h"

#include "coordsystem.h"
#include "latlong.h"
#include "posinfo2d.h"
#include "survgeom2d.h"
#include "survinfo.h"
#include "tabledef.h"

Geom2DAscIO::Geom2DAscIO( const Table::FormatDesc& fd, od_istream& strm )
    : Table::AscIO( fd )
    , strm_( strm )
{
}


void Geom2DAscIO::fillDesc( Table::FormatDesc& fd, bool withline )
{
    fd.bodyinfos_.erase();
    if ( withline )
	fd.bodyinfos_ += new Table::TargetInfo( uiStrings::sLineName(),
						StringInpSpec(),
						Table::Required );
    fd.bodyinfos_ += new Table::TargetInfo( uiStrings::sTraceNumber(),
					    IntInpSpec() );
    fd.bodyinfos_ += new Table::TargetInfo( uiStrings::sSPNumber(),
					    IntInpSpec() );
    fd.bodyinfos_ += Table::TargetInfo::mkHorPosition( true, false, true );
}


Table::FormatDesc* Geom2DAscIO::getDesc( bool withline )
{
    Table::FormatDesc* fd = new Table::FormatDesc( "Geom2D" );
    fillDesc( *fd, withline );
    return fd;
}


bool Geom2DAscIO::readLine( int startidx, Coord& crd,
			    int& trcnr, int& spnr,
			    bool isxy, bool needsconv ) const
{
    trcnr = getIntValue( startidx );
    spnr = getIntValue( startidx+1 );
    if ( mIsUdf(trcnr) && mIsUdf(spnr) )
    {
	errmsg_ = tr("Trace number or Shotpoint number has to be defined.");
	return false;
    }

    if ( mIsUdf(spnr) )
	spnr = -1;

    if ( isxy )
    {
	crd.setXY( getDValue(startidx+2), getDValue(startidx+3) );
	if ( needsconv )
	    crd = fd_.bodyinfos_.last()->convert( crd );
    }
    else
    {
	LatLong ll;
	ll.setFromString( text(startidx+2), true );
	ll.setFromString( text(startidx+3), false );
	crd = LatLong::transform( ll );
    }

    return true;
}


bool Geom2DAscIO::getData( SurvGeom2D& geom ) const
{
    if ( !getHdrVals(strm_) )
	return false;

    const bool needsconv = fd_.bodyinfos_.last()->needsConversion();
    const bool isxy = fd_.bodyinfos_.last()->selection_.form_ == 0;
    int trcidx = 0;
    while ( true )
    {
	const int ret = getNextBodyVals( strm_ );
	if ( ret < 0 ) return false;
	if ( ret == 0 ) break;

	Coord crd;
	int trcnr, spnr;
	if ( !readLine(0,crd,trcnr,spnr,isxy,needsconv) )
	    continue;

	trcidx++;
	if ( mIsUdf(trcnr) )
	    trcnr = trcidx;

	geom.add( crd, trcnr, spnr );
    }

    geom.commitChanges();
    return geom.size();
}


bool Geom2DAscIO::getData( ObjectSet<SurvGeom2D>& geoms ) const
{
    if ( !getHdrVals(strm_) )
	return false;

    const bool needsconv = fd_.bodyinfos_.last()->needsConversion();
    const bool isxy = fd_.bodyinfos_.last()->selection_.form_ == 0;
    if ( !isxy && !SI().getCoordSystem() )
    {
	errmsg_ = tr("The input file has positions defined as Lat/Long.\n"
		     "Please select a CRS for this project first.");
	return false;
    }

    SurvGeom2D* geom = 0;
    int trcidx = 0;
    while ( true )
    {
	const int ret = getNextBodyVals( strm_ );
	if ( ret < 0 ) return false;
	if ( ret == 0 ) break;

	const BufferString linenm = text( 0 );
	if ( linenm.isEmpty() )
	    continue;

	if ( !geom || linenm != geom->name() )
	{
	    geom = new SurvGeom2D( linenm );
	    geom->ref();
	    geoms += geom;
	    trcidx = 0;
	}

	Coord crd;
	int trcnr, spnr;
	if ( !readLine(1,crd,trcnr,spnr,isxy,needsconv) )
	    continue;

	trcidx++;
	if ( mIsUdf(trcnr) )
	    trcnr = trcidx;

	geom->add( crd, trcnr, spnr );
    }

    for ( int idx=0; idx<geoms.size(); idx++ )
	geoms[idx]->commitChanges();

    return true;
}
