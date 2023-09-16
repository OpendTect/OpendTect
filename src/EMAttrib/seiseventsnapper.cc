/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "seiseventsnapper.h"
#include "seistrc.h"
#include "seisioobjinfo.h"
#include "seiscubeprov.h"
#include "seisselectionimpl.h"
#include "emhorizon3d.h"
#include "ioobj.h"
#include "trckeyzsampling.h"
#include "binidvalset.h"


SeisEventSnapper::SeisEventSnapper( const Interval<float>& gate,
				    bool eraseundef )
    : Executor("Snapping to nearest event")
    , searchgate_(gate)
    , eraseundef_(eraseundef)
{
    searchgate_.sort();
}


SeisEventSnapper::~SeisEventSnapper()
{}


float SeisEventSnapper::findNearestEvent( const SeisTrc& trc, float tarz ) const
{
    SeisTrcValueSeries valseries( trc, 0 );
    ValueSeriesEvFinder<float,float> evfinder( valseries, trc.size(),
					       trc.info().sampling );
    if ( eventtype_ == VSEvent::GateMax || eventtype_ == VSEvent::GateMin )
    {
	Interval<float> gate( searchgate_ );
	gate.shift( tarz );
	return evfinder.find( eventtype_, gate ).pos;
    }

    if ( searchgate_.stop<0 )
    {
	Interval<float> gate( tarz+searchgate_.stop, tarz+searchgate_.start );
	return evfinder.find( eventtype_, gate ).pos;
    }

    if ( searchgate_.start>0 )
    {
	Interval<float> gate( tarz+searchgate_.start, tarz+searchgate_.stop );
	return evfinder.find( eventtype_, gate ).pos;
    }

    Interval<float> gateabove( tarz, tarz+searchgate_.start );
    Interval<float> gatebelow( tarz, tarz+searchgate_.stop );
    const float eventposabove = evfinder.find( eventtype_, gateabove ).pos;
    const float eventposbelow = evfinder.find( eventtype_, gatebelow ).pos;
    if ( mIsUdf(eventposabove) && mIsUdf(eventposbelow) )
    {
	Interval<float> gate( tarz+searchgate_.start, tarz+searchgate_.stop );
	return evfinder.find( eventtype_, gate ).pos;
    }
    else if ( mIsUdf(eventposabove) )
	return eventposbelow;
    else if ( mIsUdf(eventposbelow) )
	return eventposabove;
   const float diffabove = tarz - eventposabove;
   const float diffbelow = eventposbelow - tarz;
   return diffabove < diffbelow ? eventposabove : eventposbelow;
}


SeisEventSnapper3D::SeisEventSnapper3D( const IOObj& ioobj,
					const EM::Horizon3D& in,
					EM::Horizon3D& out,
					const Interval<float>& gate,
					bool eraseundef )
    : SeisEventSnapper(gate,eraseundef)
    , inhorizon_(&in),outhorizon_(&out)
{
    mscprov_ = new SeisMSCProvider( ioobj );
    mscprov_->prepareWork();

    TrcKeyZSampling tkzs( true );
    tkzs.hsamp_ = in.range();
    mscprov_->setSelData( new Seis::RangeSelData(tkzs) );
    totalnr_ = tkzs.hsamp_.totalNr();

    outhorizon_->setBurstAlert( true );
}


SeisEventSnapper3D::~SeisEventSnapper3D()
{
    outhorizon_->setBurstAlert( false );
    delete mscprov_;
}


uiString SeisEventSnapper::uiNrDoneText() const
{
    return tr("Nr positions done");
}


int SeisEventSnapper3D::nextStep()
{
    const SeisMSCProvider::AdvanceState res = mscprov_->advance();
    switch ( res )
    {
	case SeisMSCProvider::Error: return ErrorOccurred();
	case SeisMSCProvider::EndReached: return Finished();
	case SeisMSCProvider::NewPosition:
	{
	    SeisTrc* trc = mscprov_->get(0,0);
	    const BinID pos = trc->info().binID();
	    const float inzval = inhorizon_->getZ( pos );
	    float outzval = inzval;
	    if ( !mIsUdf(inzval) )
	    {
		outzval = findNearestEvent( *trc, inzval );
		if ( !eraseundef_ && mIsUdf(outzval) )
		    outzval = inzval;
	    }

	    outhorizon_->setZ( pos, outzval, false );
	    nrdone_++;
	}

	case SeisMSCProvider::Buffering:
	    return MoreToDo();
    }

    return Finished();
}
