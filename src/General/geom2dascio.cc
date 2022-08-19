/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "geom2dascio.h"

#include "posinfo2d.h"
#include "survinfo.h"
#include "survgeom2d.h"
#include "tabledef.h"

Geom2dAscIO::Geom2dAscIO( const Table::FormatDesc& fd, od_istream& strm )
    : Table::AscIO( fd )
    , strm_( strm )
{
}


void Geom2dAscIO::fillDesc( Table::FormatDesc& fd, bool withline )
{
    fd.bodyinfos_.erase();
    if ( withline )
	fd.bodyinfos_ += new Table::TargetInfo( "Line Name", StringInpSpec(),
						Table::Required );
    fd.bodyinfos_ += new Table::TargetInfo( "Trace Nr", IntInpSpec() );
    fd.bodyinfos_ += new Table::TargetInfo( "SP Nr", IntInpSpec() );
    fd.bodyinfos_ +=
	Table::TargetInfo::mkHorPosition( true, false, true );
}


Table::FormatDesc* Geom2dAscIO::getDesc( bool withline )
{
    Table::FormatDesc* fd = new Table::FormatDesc( "Geom2D" );
    fillDesc( *fd, withline );
    return fd;
}


bool Geom2dAscIO::getData( PosInfo::Line2DData& geom )
{
    if ( !getHdrVals(strm_) )
	return false;

     while ( true )
     {
	const int ret = getNextBodyVals( strm_ );
	if ( ret < 0 ) return false;
	if ( ret == 0 ) break;

	const int trcnr = getIntValue(0);
	if ( mIsUdf(trcnr) )
	    continue;

	PosInfo::Line2DPos pos( trcnr );
	pos.coord_.x = getDValue( 2 );
	pos.coord_.y = getDValue( 3 );
	geom.add( pos );
     }

     return geom.positions().size();
}


bool Geom2dAscIO::readLine( int startidx, Coord& crd,
			    int& trcnr, float& spnr,
			    bool isxy, bool needsconv ) const
{
    trcnr = getIntValue( startidx );
    spnr = getFValue( startidx+1 );
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
	ll.setFromString( getText(startidx+2).buf(), true );
	ll.setFromString( getText(startidx+3), false );
	crd = LatLong::transform( ll );
    }

    return true;
}


bool Geom2dAscIO::getData( Survey::Geometry2D& geom ) const
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
	int trcnr; float spnr;
	if ( !readLine(0,crd,trcnr,spnr,isxy,needsconv) )
	    continue;

	trcidx++;
	if ( mIsUdf(trcnr) )
	    trcnr = trcidx;

	geom.add( crd, trcnr, spnr );
    }

    geom.touch();
    return geom.size();
}


bool Geom2dAscIO::getData( ObjectSet<Survey::Geometry2D>& geoms ) const
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

    Survey::Geometry2D* geom = 0;
    int trcidx = 0;
    while ( true )
    {
	const int ret = getNextBodyVals( strm_ );
	if ( ret < 0 ) return false;
	if ( ret == 0 ) break;

	const BufferString linenm = getText( 0 );
	if ( linenm.isEmpty() )
	    continue;

	if ( !geom || linenm != geom->getName() )
	{
	    geom = new Survey::Geometry2D( linenm );
	    geom->ref();
	    geoms += geom;
	    trcidx = 0;
	}

	Coord crd;
	int trcnr; float spnr;
	if ( !readLine(1,crd,trcnr,spnr,isxy,needsconv) )
	    continue;

	trcidx++;
	if ( mIsUdf(trcnr) )
	    trcnr = trcidx;

	geom->add( crd, trcnr, spnr );
    }

    for ( int idx=0; idx<geoms.size(); idx++ )
	geoms[idx]->touch();

    return true;
}
