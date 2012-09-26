/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Satyaki Maitra
 * DATE     : September 2010
-*/

static const char* rcsID mUsedVar = " $";

#include "geom2dascio.h"
#include "posinfo2d.h"
#include "tabledef.h"

Geom2dAscIO::Geom2dAscIO( const Table::FormatDesc& fd, std::istream& strm )
    : Table::AscIO( fd )
    , strm_( strm )
{
}


Table::FormatDesc* Geom2dAscIO::getDesc()
{
    Table::FormatDesc* fd = new Table::FormatDesc( "Geom2D" );
    fd->bodyinfos_ += new Table::TargetInfo( "Trace Nr", IntInpSpec() );
    fd->bodyinfos_ += Table::TargetInfo::mkHorPosition( true );

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

	 PosInfo::Line2DPos pos( getIntValue(0) );
	 pos.coord_.x = getdValue( 1 );
	 pos.coord_.y = getdValue( 2 );
	 geom.add( pos );
     }

     return geom.positions().size();
}
