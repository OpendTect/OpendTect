/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Umesh Sinha
 Date:		June 2008
________________________________________________________________________

-*/

#include "prestackmuteasciio.h"

#include "od_istream.h"
#include "prestackmutedef.h"
#include "survinfo.h"
#include "tabledef.h"
#include "unitofmeasure.h"

namespace PreStack
{

MuteAscIO::MuteAscIO( const Table::FormatDesc& fd, od_istream& stm )
    : Table::AscIO(fd)
    , strm_(stm)
{}


Table::FormatDesc* MuteAscIO::getDesc()
{
    Table::FormatDesc* fd = new Table::FormatDesc( "Mute" );
    createDescBody( *fd, true );
    return fd;
}


void MuteAscIO::updateDesc( Table::FormatDesc& fd, bool haveposinfo )
{
    fd.bodyinfos_.erase();
    createDescBody( fd, haveposinfo );
}


void MuteAscIO::createDescBody( Table::FormatDesc& fd, bool haveposinfo )
{
    if ( haveposinfo )
	fd.bodyinfos_ += Table::TargetInfo::mkHorPosition( true );

    fd.bodyinfos_ += new Table::TargetInfo( sKey::Offset(), FloatInpSpec(),
					    Table::Required );

    fd.bodyinfos_ += Table::TargetInfo::mkZPosition( true );
}


float MuteAscIO::getUdfVal() const
{
    if( !getHdrVals(strm_) )
	return mUdf(float);

    return getFValue( 0 );
}


bool MuteAscIO::isXY() const
{
    return formOf( false, 0 ) == 0;
}


bool MuteAscIO::getMuteDef( MuteDef& mutedef, bool extrapol,
			   PointBasedMathFunction::InterpolType iptype )
{
    while ( true )
    {
	const int ret = getNextBodyVals( strm_ );
	if ( ret < 0 ) return false;
	if ( ret == 0) break;

	BinID binid( getBinID(0,1) );

	const PointBasedMathFunction::ExtrapolType et = extrapol
	    ? PointBasedMathFunction::EndVal
	    : PointBasedMathFunction::None;


	if ( mutedef.indexOf(binid) < 0 )
	    mutedef.add( new PointBasedMathFunction(iptype, et ), binid );

	mutedef.getFn(mutedef.indexOf(binid)).add( getFValue(2), getFValue(3) );
    }

    return true;
}


bool MuteAscIO::getMuteDef( MuteDef& mutedef, const BinID& binid, bool extrapol,
			    PointBasedMathFunction::InterpolType iptype )
{
    if ( mutedef.indexOf(binid) < 0 )
    {
	const PointBasedMathFunction::ExtrapolType et = extrapol
	    ? PointBasedMathFunction::EndVal
	    : PointBasedMathFunction::None;

	mutedef.add( new PointBasedMathFunction(iptype,et), binid );
    }

    while ( true )
    {
	const int ret = getNextBodyVals( strm_ );
	if ( ret < 0 ) return false;
	if ( ret == 0) break;

	mutedef.getFn(mutedef.indexOf(binid)).add( getFValue(0), getFValue(1) );
    }

    return true;
}

} // namespace PreStack
