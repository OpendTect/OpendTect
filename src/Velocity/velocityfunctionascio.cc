/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
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
			      Pos::GeomID geomid,
			      od_int64 nrkbytes )
    : Table::AscIO(fd)
    , strm_(stm)
    , geomid_(geomid)
    , nrkbytes_( nrkbytes )
{}


FunctionAscIO::~FunctionAscIO()
{}


Table::FormatDesc* FunctionAscIO::getDesc( bool is2d )
{
    Table::FormatDesc* fd = new Table::FormatDesc( "Velocity Function" );
    createDescBody( *fd, is2d );
    return fd;
}


void FunctionAscIO::updateDesc( Table::FormatDesc& fd, bool is2d )
{
    fd.bodyinfos_.erase();
    createDescBody( fd, is2d );
}


void FunctionAscIO::createDescBody( Table::FormatDesc& fd, bool is2d )
{
    if ( is2d )
	fd.bodyinfos_ += Table::TargetInfo::mk2DHorPosition( true );
    else
	fd.bodyinfos_ += Table::TargetInfo::mkHorPosition( true );

    fd.bodyinfos_ += Table::TargetInfo::mkZPosition( true );
    fd.bodyinfos_ += new Table::TargetInfo( "Velocity", FloatInpSpec(),
					    Table::Required );
}


void FunctionAscIO::setOutput( BinIDValueSet& bvs )
{
    output_ = &bvs;
    first_ = true;
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
	output_->setIs2D( geomid_.is2D() );
    }

    BinID binid;
    int col = 0;
    if ( isXY() )
    {
	const Coord pos = getPos( 0, 1 );
	if ( geomid_.is2D() )
	{
	    ConstRefMan<Survey::Geometry> geom =
				Survey::GM().getGeometry( geomid_ );
	    binid.lineNr() = geomid_.asInt();
	    float dist;
	    const TrcKey tk =
		geom ? geom->nearestTrace( pos, &dist ) : TrcKey::udf();
	    binid.trcNr() = tk.trcNr();
	}
	else
	    binid = SI().transform( pos );

	col += 2;
    }
    else
    {
	if ( geomid_.is2D() )
	{
	    binid.lineNr() = geomid_.asInt();
	    binid.trcNr() = getIntValue( 0 );
	    col += 1;
	}
	else
	{
	    binid = getBinID( 0, 1 );
	    col += 2;
	}
    }

    if ( binid == BinID::udf() )
	return MoreToDo();

    farr[0] = getFValue( col );
    farr[1] = getFValue( ++col );
    farr[2] = output_->nrVals()==3 ? getFValue( ++col ) : mUdf(float);
    output_->add( binid, farr );

    return MoreToDo();
}

} // namespace Vel
