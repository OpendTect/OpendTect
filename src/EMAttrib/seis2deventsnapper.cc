/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "seis2deventsnapper.h"

#include "emhor2dseisiter.h"
#include "emhorizon2d.h"
#include "ioobj.h"
#include "seistrc.h"
#include "seisread.h"


SeisEventSnapper2D::SeisEventSnapper2D( const IOObj& seisobj,
					Pos::GeomID geomid,
					const EM::Horizon2D& in,
					EM::Horizon2D& out,
					const Interval<float>& gate,
					bool eraseundef )
    : SeisEventSnapper(gate,eraseundef)
    , geomid_(geomid)
    , inhorizon_(&in),outhorizon_(&out)
    , seisrdr_(nullptr)
{
    const Seis::GeomType gt = Seis::Line;
    seisrdr_ = new SeisTrcReader( seisobj, geomid_, &gt );
    seisrdr_->prepareWork();
    totalnr_ = seisrdr_->expectedNrTraces();
}


SeisEventSnapper2D::~SeisEventSnapper2D()
{
    delete seisrdr_;
}


int SeisEventSnapper2D::nextStep()
{
    if ( !seisrdr_ )
	return ErrorOccurred();

    const int res = seisrdr_->get( trc_.info() );
    if ( res == -1 )
	return ErrorOccurred();
    else if ( res == 0 )
	return Finished();

    if ( !seisrdr_->get(trc_) )
	return MoreToDo();

    const float inzval = inhorizon_->getZ( trc_.info().trcKey() );
    float outzval = inzval;
    if ( !mIsUdf(inzval) )
    {
	outzval = findNearestEvent( trc_, inzval );
	if ( !eraseundef_ && mIsUdf(outzval) )
	    outzval = inzval;
    }

    outhorizon_->setZ( trc_.info().trcKey(), outzval, false );
    nrdone_ ++;
    return MoreToDo();
}

