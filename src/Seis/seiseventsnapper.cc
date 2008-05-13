/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Nanne Hemstra
 Date:		September 2006
 RCS:		$Id: seiseventsnapper.cc,v 1.5 2008-05-13 10:38:45 cvsjaap Exp $
________________________________________________________________________

-*/

#include "seiseventsnapper.h"
#include "seistrc.h"
#include "seisioobjinfo.h"
#include "seismscprov.h"
#include "seisselectionimpl.h"
#include "ioobj.h"
#include "cubesampling.h"
#include "binidvalset.h"


SeisEventSnapper::SeisEventSnapper( const IOObj& ioobj, BinIDValueSet& bvs )
    : Executor("Snapping to nearest event")
    , positions_(bvs)
{
    SeisIOObjInfo info( ioobj );
    CubeSampling cs;
    info.getRanges( cs );
    sd_.start = cs.zrg.start;
    sd_.step = cs.zrg.step;
    nrsamples_ = cs.nrZ();

    mscprov_ = new SeisMSCProvider( ioobj );
    mscprov_->prepareWork();

    const Interval<float> zrg = bvs.valRange( 0 );
    const Interval<float> extraz( cs.zrg.start-zrg.start, cs.zrg.stop-zrg.stop);
    mscprov_->setSelData( new Seis::TableSelData(bvs,&zrg) );
    totalnr_ = bvs.totalSize();
    nrdone_ = 0;
}


SeisEventSnapper::~SeisEventSnapper()
{
    delete mscprov_;
}


int SeisEventSnapper::nextStep()
{
    const SeisMSCProvider::AdvanceState res = mscprov_->advance();
    switch ( res )
    {
	case SeisMSCProvider::Error: return ErrorOccurred;
	case SeisMSCProvider::EndReached: return Finished;
	case SeisMSCProvider::NewPosition:
	{
	    SeisTrc* trc = mscprov_->get(0,0);
	    BinIDValueSet::Pos pos = positions_.findFirst( trc->info().binid );
	    if ( pos.valid() )
	    {
		BinID dummy; float zval;
		positions_.get( pos, dummy, zval );
		zval = findNearestEvent( *trc, zval );
		positions_.set( pos, zval );
		nrdone_++;
	    }
	}
	case SeisMSCProvider::Buffering:
	    return MoreToDo;
    }

    return Finished;
}


float SeisEventSnapper::findNearestEvent( const SeisTrc& trc, float tarz ) const
{
    SeisTrcValueSeries valseries( trc, 0 );
    ValueSeriesEvFinder<float,float> evfinder( valseries, nrsamples_, sd_ );
    if ( eventtype_ == VSEvent::GateMax || eventtype_ == VSEvent::GateMin )
    {
	Interval<float> gate( searchgate_ );
	gate.shift( tarz );
	return evfinder.find( eventtype_, gate ).pos;
    }

    Interval<float> gateabove( tarz, tarz+searchgate_.start );
    Interval<float> gatebelow( tarz, tarz+searchgate_.stop );
    const float eventposabove = evfinder.find( eventtype_, gateabove ).pos;
    const float eventposbelow = evfinder.find( eventtype_, gatebelow ).pos;
    if ( mIsUdf(eventposabove) )
	return eventposbelow;
    else if ( mIsUdf(eventposbelow) )
	return eventposabove;
    else
    {
	const float diffabove = tarz - eventposabove;
	const float diffbelow = eventposbelow - tarz;
	return diffabove < diffbelow ? eventposabove : eventposbelow;
    }

    return tarz;
}
