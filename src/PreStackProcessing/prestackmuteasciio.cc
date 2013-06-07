/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Umesh Sinha
 Date:		June 2008
________________________________________________________________________

-*/
static const char* rcsID = "$Id$";

#include "prestackmuteasciio.h"
#include "prestackmutedef.h"
#include "survinfo.h"
#include "tabledef.h"
#include "unitofmeasure.h"

namespace PreStack
{

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

    fd.bodyinfos_ += new Table::TargetInfo( sKey::Offset, FloatInpSpec(),
					    Table::Required );

    fd.bodyinfos_ += Table::TargetInfo::mkZPosition( true );
}


float MuteAscIO::getUdfVal() const
{
    if( !getHdrVals(strm_) )
	return mUdf(float);

    return getfValue( 0 );
}


bool MuteAscIO::isXY() const
{
    return formOf( false, 0 ) == 0;
}


bool MuteAscIO::getMuteDef( MuteDef& mutedef, bool extrapol, 
			   PointBasedMathFunction::InterpolType iptype )
{
    const bool isxy = isXY();

    while ( true )
    {
	const int ret = getNextBodyVals( strm_ );
	if ( ret < 0 ) return false;
	if ( ret == 0) break;

	BinID binid;
	if ( isxy )
	    binid = SI().transform( Coord(getdValue(0),getdValue(1)) );
	else
	{
   	    binid.inl = getIntValue(0);
   	    binid.crl = getIntValue(1);
	}

	if ( mutedef.indexOf(binid) < 0 )
	    mutedef.add( new PointBasedMathFunction(iptype,extrapol), binid );

	mutedef.getFn(mutedef.indexOf(binid)).add( getfValue(2), getfValue(3) );
    }

    return true;
}


bool MuteAscIO::getMuteDef( MuteDef& mutedef, const BinID& binid, bool extrapol,
			    PointBasedMathFunction::InterpolType iptype)
{
    if ( mutedef.indexOf(binid) < 0 )
	mutedef.add( new PointBasedMathFunction(iptype,extrapol), binid );

    while ( true )
    {
	const int ret = getNextBodyVals( strm_ );
	if ( ret < 0 ) return false;
	if ( ret == 0) break;

	mutedef.getFn( mutedef.indexOf(binid)).add( getfValue(0), 
						    getfValue(1) );
    }

    return true;
}

} // namespace PreStack
