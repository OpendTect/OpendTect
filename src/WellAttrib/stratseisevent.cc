/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Jul 2010
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";


#include "stratseisevent.h"
#include "survinfo.h"
#include "seistrc.h"


Strat::SeisEvent::SeisEvent( const Strat::Level* lvl, VSEvent::Type evtyp )
    : level_(lvl)
    , evtype_(evtyp)
    , offs_(0)
    , extrwin_(0,0,4)
    , downtolevel_(0)
{
    if ( SI().zIsTime() )
	extrwin_.step = SI().zStep();
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
