/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "stratseisevent.h"
#include "survinfo.h"
#include "seistrc.h"


// Strat::SeisEvent

Strat::SeisEvent::SeisEvent( Strat::LevelID lvlid, VSEvent::Type evtyp )
    : levelid_(lvlid)
    , evtype_(evtyp)
    , extrwin_(0.f,0.f)
{
}


Strat::SeisEvent::~SeisEvent()
{
}


bool Strat::SeisEvent::snapPick( SeisTrc& trc ) const
{
    float reftm = snappedTime( trc );
    if ( mIsUdf(reftm) )
	return false;
    trc.info().pick = reftm;
    return true;
}


float Strat::SeisEvent::snappedTime( const SeisTrc& trc ) const
{
    float reftm = trc.info().pick;
    reftm += offs_;
    if ( evtype_ == VSEvent::None )
	return reftm;

    const SeisTrcValueSeries tvs( trc, 0 );
    const SamplingData<float> sd( trc.info().sampling );
    const int trcsz = trc.size();

    ValueSeriesEvFinder<float,float> evf( tvs, trcsz-1, sd );
    const Interval<float> trcwin( trc.startPos(), trc.samplePos(trcsz-1) );
    for ( int iwdth=1; iwdth<trcsz; iwdth++ )
    {
	Interval<float> findwin( reftm - iwdth*sd.step, reftm + iwdth*sd.step );
	if ( findwin.start < trcwin.start ) findwin.start = trcwin.start;
	if ( findwin.stop > trcwin.stop ) findwin.stop = trcwin.stop;

	ValueSeriesEvent<float,float> ev = evf.find( evtype_, findwin );
	if ( !mIsUdf(ev.pos) )
	    return ev.pos;
    }

    return mUdf(float);
}
