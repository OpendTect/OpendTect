/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "arrayndimpl.h"
#include "sorting.h"
#include "seistrc.h"
#include "welltiepickset.h"
#include "bufstringset.h"
#include "welltiedata.h"


WellTie::PickSetMgr::PickSetMgr( PickData& pd )
    : evtype_(VSEvent::Extr)
    , pickadded(this)
    , synthpickset_(pd.synthpicks_)
    , seispickset_(pd.seispicks_)
{
}


WellTie::PickSetMgr::~PickSetMgr()
{
}


void WellTie::PickSetMgr::setEventType( const char* ev )
{
    if ( !VSEvent::parseEnum(ev, evtype_) )
        evtype_ = VSEvent::None;
}


const char* WellTie::PickSetMgr::getEventType() const
{
    return VSEvent::toString( evtype_ );
}


void WellTie::PickSetMgr::getEventTypes( BufferStringSet& bss ) const
{
    bss.erase();

    bss.add( VSEvent::toString(VSEvent::None) );
    bss.add( VSEvent::toString(VSEvent::Extr) );
    bss.add( VSEvent::toString(VSEvent::Max) );
    bss.add( VSEvent::toString(VSEvent::Min) );
    bss.add( VSEvent::toString(VSEvent::ZC) );
}


void WellTie::PickSetMgr::addPick( float zpos, bool issynth, const SeisTrc* trc)
{
    TypeSet<Marker>& curpickset = issynth ? synthpickset_ : seispickset_;
    TypeSet<Marker>& altpickset = issynth ? seispickset_ : synthpickset_;
    const int curpicksetsz = curpickset.size();
    const int altpicksetsz = altpickset.size();
    if ( abs(curpicksetsz-altpicksetsz)<2 )
    {
	if ( (issynth==lastpicksynth_) && curpicksetsz-altpicksetsz>0 )
	    if ( curpicksetsz )
		curpickset.pop();

	Marker marker( trc ? findEvent( *trc, zpos ) : zpos );
	marker.color_ = OD::Color::DgbColor();
	curpickset += marker;
	lastpicksynth_ = issynth;
    }

    pickadded.trigger();
}



#define mTimeGate 0.02f
float WellTie::PickSetMgr::findEvent( const SeisTrc& trc, float zpos ) const
{
    if ( evtype_ == VSEvent::None ) return zpos;

    const int maxidx = trc.size();
    Interval<float> intvup ( zpos, zpos - mTimeGate );
    Interval<float> intvdown ( zpos, zpos + mTimeGate );
    SamplingData<float> sd = trc.info().sampling_;
    Array1DImpl<float> vals( trc.size() );
    for ( int idx=0; idx<trc.size(); idx++ )
	vals.set( idx, trc.get( idx, 0 ) );

    ValueSeriesEvFinder<float,float> evf( vals, maxidx, sd );
    const float evposup =  evf.find( evtype_, intvup ).pos;
    const float evposdown =  evf.find( evtype_, intvdown ).pos;
    float evpos = evposdown;
    if ( (mIsUdf(evposup) || evposup<0) && (mIsUdf(evposdown) || evposdown<0) )
	evpos = zpos;
    else if ( mIsUdf(evposdown) || evposdown<0 )
	evpos = evposup;
    else if ( fabs(zpos-evposup)<fabs(zpos-evposdown) )
	evpos = evposup;

    return evpos;
}


void WellTie::PickSetMgr::clearAllPicks()
{
    seispickset_.erase();
    synthpickset_.erase();
}


void WellTie::PickSetMgr::clearLastPicks()
{
    if ( isSynthSeisSameSize() )
    {
	if ( lastpicksynth_ )
	    synthpickset_.removeSingle( synthpickset_.size()-1 );
	else
	    seispickset_.removeSingle( seispickset_.size()-1 );
    }
    else if ( seispickset_.size() > synthpickset_.size() )
	seispickset_.removeSingle( seispickset_.size()-1 );
    else if ( seispickset_.size() < synthpickset_.size() )
	synthpickset_.removeSingle( synthpickset_.size()-1 );
    lastpicksynth_ = !lastpicksynth_;
}


bool WellTie::PickSetMgr::isPick() const
{
    return ( !seispickset_.isEmpty() || !synthpickset_.isEmpty() );
}


bool WellTie::PickSetMgr::isSynthSeisSameSize() const
{
    return ( seispickset_.size() == synthpickset_.size() );
}


void WellTie::PickSetMgr::setPickSetPos( bool issynth, int idx, float z )
{
    TypeSet<Marker>& pickset = issynth ? synthpickset_ : seispickset_;
    pickset[idx].zpos_ = z;
}


void WellTie::PickSetMgr::sortByPos()
{
    sortByPos( synthpickset_ );
    sortByPos( seispickset_ );
}


void WellTie::PickSetMgr::sortByPos( TypeSet<Marker>& pickset )
{
    const int sz = pickset.size();
    TypeSet<float> zvals;
    for ( int idx=0; idx<sz; idx++ )
	zvals += pickset[idx].zpos_;

    mAllocVarLenArr( int, zidxs, sz );
    for ( int idx=0; idx<sz; idx++ )
	zidxs[idx] = idx;

    sort_coupled( zvals.arr(), mVarLenArr(zidxs), sz );

    for ( int idx=0; idx<sz; idx++ )
	pickset[idx].zpos_ = zvals[idx];
}
