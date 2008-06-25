/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		June 2008
 RCS:		$Id: prestackmuteasciio.cc,v 1.3 2008-06-25 06:40:28 cvsumesh Exp $
________________________________________________________________________

-*/

#include "prestackmuteasciio.h"
#include "tabledef.h"
#include "prestackmutedef.h"
#include "unitofmeasure.h"
#include "survinfo.h"

static const char* extrapoltypes[] = {"Linear","Poly","Snap",0};

namespace PreStack
{

Table::FormatDesc* MuteAscIO::getDesc()
{
    Table::FormatDesc* fd = new Table::FormatDesc( "Mute" );
    createDescBody( *fd, true);
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
    {
	Table::TargetInfo* posinfo =
	    new Table::TargetInfo( "X/Y", FloatInpSpec(), Table::Required );
	posinfo->form(0).add( FloatInpSpec() );
	posinfo->add( posinfo->form(0).duplicate("Inl/Crl") );
	fd.bodyinfos_ += posinfo;
    }

    fd.bodyinfos_ += new Table::TargetInfo( "Offset", FloatInpSpec(),
					    Table::Required );

    Table::TargetInfo* bti = new Table::TargetInfo( "Z Values" , FloatInpSpec(),
	                                           Table::Required,
					           PropertyRef::surveyZType() );
    bti->selection_.unit_ = UoMR().get( SI().zIsTime() ? "Milliseconds" :
	    				(SI().zInFeet() ? "Feet" : "Meter") );
    fd.bodyinfos_ += bti;
}


float MuteAscIO::getUdfVal() const
{
    if( !getHdrVals(strm_) )
	return mUdf(float);

    return getfValue( 0 );
}


bool MuteAscIO::isXY() const
{
    const Table::TargetInfo* xinfo = fd_.bodyinfos_[0];
    return xinfo ? xinfo->selection_.form_ == 0 : false;
}


bool MuteAscIO::getMuteDef( MuteDef& mutedef, bool extrapol, 
			   PointBasedMathFunction::InterpolType iptype )
{
    bool isxy = isXY();

    while ( true )
    {
	const int ret = getNextBodyVals( strm_ );
	if ( ret < 0 ) return false;
	if ( ret == 0) break;

	BinID binid;
	if ( isxy )
	    binid = SI().transform( Coord(getfValue(0),getfValue(1)) );
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

	mutedef.getFn(mutedef.indexOf(binid)).add( getfValue(0), getfValue(1) );
    }

    return true;
}

} // namespace PreStack
