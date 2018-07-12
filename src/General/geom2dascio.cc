/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Satyaki Maitra
 * DATE     : September 2010
-*/

#include "geom2dascio.h"

#include "posinfo2d.h"
#include "survgeom2d.h"
#include "tabledef.h"

Geom2DAscIO::Geom2DAscIO( const Table::FormatDesc& fd, od_istream& strm )
    : Table::AscIO( fd )
    , strm_( strm )
{
}


Table::FormatDesc* Geom2DAscIO::getDesc()
{
    Table::FormatDesc* fd = new Table::FormatDesc( "Geom2D" );
    fd->bodyinfos_ += new Table::TargetInfo( uiStrings::sTraceNumber(),
					     IntInpSpec() );
    fd->bodyinfos_ += new Table::TargetInfo( uiStrings::sSPNumber(),
					     IntInpSpec() );
    fd->bodyinfos_ += Table::TargetInfo::mkHorPosition( true );
    return fd;
}


bool Geom2DAscIO::getData( Survey::Geometry2D& geom )
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

	geom.add( getDValue(2), getDValue(3), getIntValue(0), getIntValue(1) );
     }

     return geom.size();
}
