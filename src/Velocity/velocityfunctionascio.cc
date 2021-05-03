
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Umesh Sinha
 Date:		Aug 2008
________________________________________________________________________

-*/

#include "velocityfunctionascio.h"

#include "binidvalset.h"
#include "survinfo.h"
#include "tabledef.h"
#include "tableconv.h"
#include "unitofmeasure.h"
#include "od_istream.h"

namespace Vel
{


FunctionAscIO::FunctionAscIO( const Table::FormatDesc& fd,
			      od_istream& stm,
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

    return getFValue( 0 );
}


bool FunctionAscIO::isXY() const
{
    return formOf( false, 0 ) == 0;
}


int FunctionAscIO::nextStep()
{
    float farr[3];

    const od_stream::Pos oldpos = strm_.position();
    const int ret = getNextBodyVals( strm_ );
    if ( ret < 0 ) return ErrorOccurred();
    if ( ret == 0) return Finished();

    const od_stream::Pos newpos = strm_.position();
    nrdone_ += newpos-oldpos;

    if ( first_ )
    {
	first_ = false;
	bool hasanisotropy = cnvrtr_->selcols_.size()>4;
	output_->setEmpty();
	output_->setNrVals( hasanisotropy ? 2 : 3, false );
    }

    BinID binid;
    if ( isXY() )
    {
	const Coord pos = getPos( 0, 1 );
	binid = SI().transform( pos );
    }
    else
	binid = getBinID(0, 1);

    if ( binid == BinID::udf() )
	return MoreToDo();

    farr[0] = getFValue(2);
    farr[1] = getFValue(3);
    farr[2] = output_->nrVals()==3 ? getFValue( 4 ) : mUdf(float);
    output_->add( binid, farr );

    return MoreToDo();
}

} // namespace Vel
