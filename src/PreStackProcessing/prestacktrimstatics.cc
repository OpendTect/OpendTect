/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : April 2005
-*/


#include "prestacktrimstatics.h"

#include "arrayndimpl.h"
#include "iopar.h"
#include "prestackgather.h"


namespace PreStack
{

static const char* sKeyNrIterations	= "Nr Iterations";
static const char* sKeyIteration	= "Iteration";
static const char* sKeyPilotTrace	= "Pilot trace";
static const char* sKeyTrimStatics	= "Trim statics";
static const char* sKeyOffsetRange	= "Offset range";
static const char* sKeyMaxShift		= "Maximum shift";

TrimStatics::Iteration::Iteration()
    : ptoffsetrg_(mUdf(float),mUdf(float))
    , tsoffsetrg_(mUdf(float),mUdf(float))
    , maxshift_(mUdf(float))
{}


bool TrimStatics::Iteration::operator==( const Iteration& it ) const
{
    return ptoffsetrg_.isEqual(it.ptoffsetrg_,mDefEps) &&
	   tsoffsetrg_.isEqual(it.tsoffsetrg_,mDefEps) &&
	   mIsEqual(maxshift_,it.maxshift_,mDefEps);
}


bool TrimStatics::Iteration::operator!=( const Iteration& it ) const
{ return !(*this==it); }



TrimStatics::TrimStatics()
    : Processor(sFactoryKeyword())
    , output_(2)
{}


TrimStatics::~TrimStatics()
{
    deepErase( pilottrcs_ );
}


void TrimStatics::addIteration( const Iteration& it )
{ iterations_ += it; }

void TrimStatics::removeIteration( int idx )
{ iterations_.removeSingle( idx ); }

void TrimStatics::removeAllIterations()
{ iterations_.erase(); }

const TypeSet<TrimStatics::Iteration>& TrimStatics::getIterations() const
{ return iterations_; }

TypeSet<TrimStatics::Iteration>& TrimStatics::getIterations()
{ return iterations_; }


od_int64 TrimStatics::nrIterations() const
{
    return Processor::nrIterations();
}


class PilotTraceExtractor : public ParallelTask
{
public:
PilotTraceExtractor( const TrimStatics::Iteration& it, const Gather& gth )
    : gather_(gth)
    , it_(it)
{
    nrz_ = gather_.data().info().getSize( Gather::zDim() );
    pilottrc_ = new Array1DImpl<float>( mCast(int,nrz_) );
    pilottrc_->setAll( mUdf(float) );
}

od_int64 nrIterations() const
{ return nrz_; }

bool doWork( od_int64 start, od_int64 stop, int )
{
    const int nroffsets = gather_.data().info().getSize( Gather::offsetDim() );
    for ( int idz=mCast(int,start); idz<=stop; idz++, addToNrDone(1) )
    {
	int nrvals = 0;
	float stack = mUdf(float);
	for ( int ido=0; ido<nroffsets; ido++ )
	{
	    const float offset = gather_.getOffset(ido);
	    if ( !it_.tsoffsetrg_.includes(offset,true) )
		continue;

	    const float val = gather_.data().get( ido, idz );
	    if ( mIsUdf(val) )
		continue;

	    if ( !nrvals ) stack = val;
	    else stack += val;

	    nrvals++;
	}

	pilottrc_->set( idz, nrvals==0 ? mUdf(float) : stack/nrvals );
    }

    return true;
}


Array1D<float>* getPilotTrace() const
{ return pilottrc_; }


protected:
    const Gather&			gather_;
    const TrimStatics::Iteration&	it_;
    od_int64				nrz_;
    Array1D<float>*			pilottrc_;
};


bool TrimStatics::prepareWork()
{
    if ( !Processor::prepareWork() )
	return false;

    deepErase( pilottrcs_ );
    for ( int idx=0; idx<iterations_.size(); idx++ )
    {
	PilotTraceExtractor task( iterations_[idx], *inputs_[0] );
	task.execute();
	pilottrcs_ += task.getPilotTrace();
    }

    return true;
}


bool TrimStatics::doWork( od_int64 start, od_int64 stop, int )
{
    if ( inputs_.isEmpty() || outputs_.isEmpty() )
	return false;

    if ( output_ == 0 )
	return doPilotTraceOutput( start, stop );
    if ( output_ == 1 )
	return doShiftOutput( start, stop );

    return doTrimStaticsOutput( start, stop );
}


bool TrimStatics::doPilotTraceOutput( od_int64 start, od_int64 stop )
{
    Gather* output = outputs_[0];
    const int nrz = output->data().info().getSize( Gather::zDim() );

    for ( int ido=mCast(int,start); ido<=stop; ido++, addToNrDone(1) )
    {
	const float offset = output->getOffset( ido );
	for ( int idx=0; idx<iterations_.size(); idx++ )
	{
	    Iteration& it = iterations_[idx];
	    if ( !it.tsoffsetrg_.includes(offset,true) )
		continue;

	    for ( int idz=0; idz<nrz; idz++ )
		output->data().set( ido, idz, pilottrcs_[idx]->get(idz) );
	}
    }

    return true;
}


bool TrimStatics::doShiftOutput( od_int64 start, od_int64 stop )
{
    return true;
}


bool TrimStatics::doTrimStaticsOutput( od_int64 start, od_int64 stop )
{
    return true;
}


static BufferString getKey( int idx, const char* key1, const char* key2 )
{
    BufferString key = IOPar::compKey( sKeyIteration, idx );
    key = IOPar::compKey( key, key1 );
    if ( key2 ) key = IOPar::compKey( key, key2 );
    return key;
}


void TrimStatics::fillPar( IOPar& par ) const
{
    const TypeSet<Iteration>& its = iterations_;
    par.set( sKeyNrIterations, its.size() );
    for ( int idx=0; idx<its.size(); idx++ )
    {
	par.set( getKey(idx,sKeyPilotTrace,sKeyOffsetRange),
		 its[idx].ptoffsetrg_ );
	par.set( getKey(idx,sKeyTrimStatics,sKeyOffsetRange),
		 its[idx].tsoffsetrg_ );
	par.set( getKey(idx,sKeyMaxShift,0),
		 its[idx].maxshift_ );
    }

    par.set( sKey::Output(), output_ );
}


bool TrimStatics::usePar( const IOPar& par )
{
    iterations_.erase();
    int nrits = 0;
    par.get( sKeyNrIterations, nrits );
    for ( int idx=0; idx<nrits; idx++ )
    {
	Iteration it;
	par.get( getKey(idx,sKeyPilotTrace,sKeyOffsetRange), it.ptoffsetrg_ );
	par.get( getKey(idx,sKeyTrimStatics,sKeyOffsetRange), it.tsoffsetrg_ );
	par.get( getKey(idx,sKeyMaxShift,0), it.maxshift_ );
	iterations_ += it;
    }

    par.get( sKey::Output(), output_ );
    return true;
}

} // namespace PreStack
