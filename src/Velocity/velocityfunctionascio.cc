
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Umesh Sinha
 Date:		Aug 2008
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id$";

#include "velocityfunctionascio.h"

#include "binidvalset.h"
#include "survinfo.h"
#include "tabledef.h"
#include "tableconv.h"
#include "unitofmeasure.h"

namespace Vel
{


FunctionAscIO::FunctionAscIO( const Table::FormatDesc& fd,
			      std::istream& stm,
       			      od_int64 nrkbytes )
    : Table::AscIO(fd)
    , strm_(stm)
    , nrdone_( 0 )
    , nrkbytes_( nrkbytes )
    , output_( 0 )
    , first_( true )
{}


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
    fd.bodyinfos_ += Table::TargetInfo::mkHorPosition( true );
    fd.bodyinfos_ += Table::TargetInfo::mkZPosition( true );
    fd.bodyinfos_ += new Table::TargetInfo( "Velocity", FloatInpSpec(),
	   				    Table::Required );
}


float FunctionAscIO::getUdfVal() const
{
    if( !getHdrVals(strm_) )
	return mUdf(float);

    return getfValue( 0 );
}


bool FunctionAscIO::isXY() const
{
    return formOf( false, 0 ) == 0;
}


int FunctionAscIO::nextStep()
{
    const bool isxy = isXY();
    float farr[3];

    const int oldpos = strm_.tellg();
    const int ret = getNextBodyVals( strm_ );
    if ( ret < 0 ) return ErrorOccurred();
    if ( ret == 0) return Finished();

    const int newpos = strm_.tellg();
    nrdone_ += newpos-oldpos;

    if ( first_ )
    {
	first_ = false;
	bool hasanisotropy = cnvrtr_->selcols_.size()>4;
	output_->setEmpty();
	output_->setNrVals( hasanisotropy ? 2 : 3, false );
    }

    BinID binid;
    if ( isxy )
    {
	 const Coord crd( getdValue(0), getdValue(1) );
	 if ( crd == Coord::udf() )
	     return MoreToDo();
	binid = SI().transform( crd );
    }
    else
    { 
	binid.inl = getIntValue(0); binid.crl = getIntValue(1);
	if ( binid == BinID::udf() )
	    return MoreToDo();
    }

    farr[0] = getfValue(2);
    farr[1] = getfValue(3);
    farr[2] = output_->nrVals()==3 ? getfValue( 4 ) : mUdf(float);

    output_->add( binid, farr );

    return MoreToDo();
}

} //namespace Vel
