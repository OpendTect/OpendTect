/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		September 2006
________________________________________________________________________

-*/

#include "seiseventsnapper.h"
#include "seistrc.h"
#include "seisioobjinfo.h"
#include "seismscprov.h"
#include "seistableseldata.h"
#include "ioobj.h"
#include "uistrings.h"
#include "binnedvalueset.h"


SeisEventSnapper::SeisEventSnapper( const Interval<float>& gate	)
    : Executor("Snapping to nearest event")
    , searchgate_(gate)
{
    searchgate_.sort();
}


float SeisEventSnapper::findNearestEvent( const SeisTrc& trc, float tarz,
		const Interval<float>& searchgate, EvType evtyp )
{
    SeisTrcValueSeries valseries( trc, 0 );
    ValueSeriesEvFinder<float,float> evfinder( valseries, trc.size(),
					       trc.info().sampling_ );
    if ( evtyp == VSEvent::GateMax || evtyp == VSEvent::GateMin )
    {
	Interval<float> gate( searchgate );
	gate.shift( tarz );
	return evfinder.find( evtyp, gate ).pos;
    }

    if ( searchgate.stop<0 )
    {
	Interval<float> gate( tarz+searchgate.stop, tarz+searchgate.start );
	return evfinder.find( evtyp, gate ).pos;
    }

    if ( searchgate.start>0 )
    {
	Interval<float> gate( tarz+searchgate.start, tarz+searchgate.stop );
	return evfinder.find( evtyp, gate ).pos;
    }

    Interval<float> gateabove( tarz, tarz+searchgate.start );
    Interval<float> gatebelow( tarz, tarz+searchgate.stop );
    const float eventposabove = evfinder.find( evtyp, gateabove ).pos;
    const float eventposbelow = evfinder.find( evtyp, gatebelow ).pos;
    if ( mIsUdf(eventposabove) && mIsUdf(eventposbelow) )
    {
	Interval<float> gate( tarz+searchgate.start, tarz+searchgate.stop );
	return evfinder.find( evtyp, gate ).pos;
    }
    else if ( mIsUdf(eventposabove) )
	return eventposbelow;
    else if ( mIsUdf(eventposbelow) )
	return eventposabove;
   const float diffabove = tarz - eventposabove;
   const float diffbelow = eventposbelow - tarz;
   return diffabove < diffbelow ? eventposabove : eventposbelow;
}


float SeisEventSnapper::findNearestEvent( const SeisTrc& trc, float tarz ) const
{
    return findNearestEvent( trc, tarz, searchgate_, eventtype_ );
}


SeisEventSnapper3D::SeisEventSnapper3D( const IOObj& ioobj, BinnedValueSet& bvs,
				    const Interval<float>& gate	)
    : SeisEventSnapper(gate)
    , positions_(bvs)
{
    mscprov_ = new Seis::MSCProvider( ioobj.key() );
    const Interval<float> zrg = bvs.valRange( 0 );
    mscprov_->setSelData( new Seis::TableSelData(bvs) );
    mscprov_->setZExtension( gate );
    totalnr_ = mCast( int, bvs.totalSize() );
    nrdone_ = 0;
}


SeisEventSnapper3D::~SeisEventSnapper3D()
{
    delete mscprov_;
}

uiString SeisEventSnapper3D::message() const
{ return uiStrings::phrHandling(uiStrings::sPosition(mPlural)); }
uiString SeisEventSnapper3D::nrDoneText() const
{ return uiStrings::phrHandled(uiStrings::sPosition(mPlural)); }


int SeisEventSnapper3D::nextStep()
{
    const Seis::MSCProvider::AdvanceState res = mscprov_->advance();
    switch ( res )
    {
	case Seis::MSCProvider::Error: return ErrorOccurred();
	case Seis::MSCProvider::EndReached: return Finished();
	case Seis::MSCProvider::NewPosition:
	{
	    SeisTrc* trc = mscprov_->curTrc();
	    BinnedValueSet::SPos pos = positions_.find( trc->info().binID() );
	    if ( pos.isValid() )
	    {
		BinID dummy; float zval;
		positions_.get( pos, dummy, zval );
		zval = findNearestEvent( *trc, zval );
		positions_.set( pos, zval );
		nrdone_++;
	    }
	}
	case Seis::MSCProvider::Buffering:
	    return MoreToDo();
    }

    return Finished();
}
