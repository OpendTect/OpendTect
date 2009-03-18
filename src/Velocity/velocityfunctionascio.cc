
/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		Aug 2008
________________________________________________________________________

-*/
static const char* rcsID = "$Id: velocityfunctionascio.cc,v 1.1 2009-03-18 18:45:26 cvskris Exp $";

#include "velocityfunctionascio.h"

#include "binidvalset.h"
#include "survinfo.h"
#include "tabledef.h"
#include "tableconv.h"
#include "unitofmeasure.h"

namespace Vel
{

Table::FormatDesc* FunctionAscIO::getDesc()
{
    Table::FormatDesc* fd = new Table::FormatDesc( "Velocity Function" );
    createDescBody( *fd );
    return fd;
}


void FunctionAscIO::updateDesc( Table::FormatDesc& fd )
{
    fd.bodyinfos_.erase();
    createDescBody( fd );
}


void FunctionAscIO::createDescBody( Table::FormatDesc& fd )
{
    Table::TargetInfo* posinfo =
	new Table::TargetInfo( "X/Y", FloatInpSpec(), Table::Required );
    posinfo->form(0).add( FloatInpSpec() );
    posinfo->add( posinfo->form(0).duplicate("Inl/Crl") );
    fd.bodyinfos_ += posinfo;

    Table::TargetInfo* bti = new Table::TargetInfo( "Z Values" , FloatInpSpec(),
	    					   Table::Required,
						   PropertyRef::surveyZType() );
    bti->selection_.unit_ = UoMR().get( SI().zIsTime() ? "Milliseconds" :
	    				(SI().zInFeet() ? "Feet" : "Meter") );
    fd.bodyinfos_ += bti;

    fd.bodyinfos_ += new Table::TargetInfo( "Velocity", FloatInpSpec(),
	   				    Table::Required );
    fd.bodyinfos_ += new Table::TargetInfo( "Anisotropy", FloatInpSpec(),
	   				    Table::Optional );
}


float FunctionAscIO::getUdfVal() const
{
    if( !getHdrVals(strm_) )
	return mUdf(float);

    return getfValue( 0 );
}


bool FunctionAscIO::isXY() const
{
    const Table::TargetInfo* xinfo = fd_.bodyinfos_[0];
    return xinfo ? xinfo->selection_.form_ == 0 : false;
}


bool FunctionAscIO::getVelocityData( BinIDValueSet& bidvalset )
{
    bool isxy = isXY();
    float farr[3];
    bool first = true;
    bool hasanisotropy;

    while ( true )
    {
	const int ret = getNextBodyVals( strm_ );
	if ( ret < 0 ) return false;
	if ( ret == 0) break;

	if ( first )
	{
	    first = false;
	    hasanisotropy = cnvrtr_->selcols_.size()>4;
	    bidvalset.empty();
	    bidvalset.setNrVals( hasanisotropy ? 2 : 3, false );
	}

	 BinID binid;
	 if ( isxy )
	      binid = SI().transform( Coord(getfValue(0),getfValue(1)) );
	 else
	 {
	     binid.inl = getIntValue(0);
	     binid.crl = getIntValue(1);
	 }

	 farr[0] = getfValue(2);
	 farr[1] = getfValue(3);

	 if ( hasanisotropy )
	     farr[2] = getfValue( 4 );

	 bidvalset.add( binid, farr );
    }

    return true;
}

} //namespace Vel
