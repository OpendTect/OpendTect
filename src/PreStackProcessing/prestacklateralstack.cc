/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : April 2005
-*/

static const char* rcsID = "$Id$";

#include "prestacklateralstack.h"

#include "iopar.h"
#include "prestackgather.h"
#include "seistrc.h"
#include "seisinfo.h"
#include "separstr.h"


using namespace PreStack;

void LateralStack::initClass()
{
    SeparString names( sFactoryKeyword(), FactoryBase::cSeparator() );
    names += "VerticalStack";
    factory().addCreator( LateralStack::createInstance, names.buf(),
	             sFactoryDisplayName() );
}


Processor* LateralStack::createInstance()
{
    return new LateralStack;
}


LateralStack::LateralStack()
    : Processor( sFactoryKeyword() )
    , patternstepout_( 1, 1 )
    , iscross_( true )
{ }


LateralStack::~LateralStack()
{ }


bool LateralStack::reset()
{
    inputstepout_ = BinID(0,0);
    freeArray( inputs_ );
    return Processor::reset();
}


bool LateralStack::setPattern( const BinID& stepout, bool cross )
{
    if ( stepout.inl<0 || stepout.crl<0 )
	return false;

    patternstepout_ = stepout;
    iscross_ = cross;

    freeArray( inputs_ );
    inputstepout_ = BinID(0,0);

    return true;
}


bool LateralStack::setOutputInterest( const BinID& relbid, bool yn )
{
    if ( !Processor::setOutputInterest( relbid, yn ) )
	return false;

    const BinID minoutstepout( abs(relbid.inl), abs(relbid.crl) );
    const BinID mininstepout = minoutstepout+patternstepout_;
    if ( mininstepout.inl<=inputstepout_.inl &&
	 mininstepout.crl<=inputstepout_.crl )
	return true;

    mPSProcAddStepoutStep( inputs_, ObjectSet<Gather>,
			   inputstepout_, mininstepout );

    inputstepout_ = mininstepout;
    return true;
}


bool LateralStack::wantsInput( const BinID& relbid ) const
{
    for ( int oinl=-outputstepout_.inl; oinl<=outputstepout_.inl; oinl++ )
    {
	for ( int ocrl=-outputstepout_.crl; ocrl<=outputstepout_.crl; ocrl++ )
	{
	    const BinID outputbid( oinl, ocrl );
	    const int outputoffset = getRelBidOffset(outputbid,outputstepout_);

	    if ( !outputinterest_[outputoffset] )
		continue;

	    for ( int pinl=-patternstepout_.inl; pinl<=patternstepout_.inl;
		  pinl++ )
	    {
		for ( int pcrl=-patternstepout_.crl; pcrl<=patternstepout_.crl;
		      pcrl++ )
		{
		    const BinID patternbid( pinl, pcrl );
		    const BinID bid = outputbid + patternbid;
		    if ( bid!=relbid )
			continue;

		    if ( isInPattern( patternbid ) )
			return true;
		}
	    }
	}
    }

    return false;
}


bool LateralStack::isInPattern( const BinID& relbid ) const
{
    if ( abs(relbid.inl)>patternstepout_.inl ||
	 abs(relbid.crl)>patternstepout_.crl )
	return false;

    if ( !iscross_ )
	return true;

    return !relbid.inl || !relbid.crl;
}


bool LateralStack::prepareWork()
{
    if ( !Processor::prepareWork() )
	return false;

    const int centeroffset = getRelBidOffset( BinID(0,0), outputstepout_ );
    Gather* centergather = outputs_[centeroffset];
    if ( !centergather )
	return false;

    offsetazi_.erase();
    const int nroffsets = centergather->size(Gather::offsetDim()==0);
    for ( int idx=0; idx<nroffsets; idx++ )
	offsetazi_ += centergather->getOffsetAzimuth( idx );
    
    return true;
}


void LateralStack::fillPar( IOPar& par ) const
{
    par.set( sKeyStepout(), patternstepout_ );
    par.setYN( sKeyCross(), iscross_ );
}


bool LateralStack::usePar( const IOPar& par )
{
    BinID stepout;
    bool cross;

    return par.get( sKeyStepout(), stepout ) &&
	   par.getYN( sKeyCross(), cross ) &&
	   setPattern( stepout, cross );
}


bool LateralStack::doWork( od_int64 start, od_int64 stop, int )
{
    for ( int ioffs=start; ioffs<=stop; ioffs++, addToNrDone(1) )
    {
	for ( int oinl=-outputstepout_.inl; oinl<=outputstepout_.inl; oinl++ )
	{
	    for ( int ocrl=-outputstepout_.crl; ocrl<=outputstepout_.crl;ocrl++)
	    {
		processOutput( offsetazi_[ioffs], BinID(oinl,ocrl) );
	    }
	}
    }

    return true;
}


bool LateralStack::processOutput( const OffsetAzimuth& oa,const BinID& center )
{
    const int outputidx = getRelBidOffset( center, outputstepout_ );
    if ( !outputinterest_[outputidx] )
	return true;

    Gather* outputgather = outputs_[outputidx];
    if ( !outputgather )
	return false;

    float* outputtrc = 0;
    for ( int idx=outputgather->size(Gather::offsetDim()==0)-1; idx>=0; idx-- )
    {
	if ( outputgather->getOffsetAzimuth(idx)==oa )
	{
	    outputtrc = outputgather->data().getData();
	    if ( outputtrc )
    		outputtrc += outputgather->data().info().getOffset( idx, 0 );

	    break;
	}
    }

    if ( !outputtrc )
	return false;

    for ( int idx=outputgather->size(Gather::zDim()==0)-1; idx>=0; idx-- )
	outputtrc[idx] = 0;

    TypeSet<int> nrvals( outputgather->size(Gather::zDim()==0), 0 );

    for ( int oinl=-patternstepout_.inl; oinl<=patternstepout_.inl; oinl++ )
    {
	for ( int ocrl=-patternstepout_.crl; ocrl<=patternstepout_.crl; ocrl++ )
	{
	    const BinID patternbid( oinl, ocrl );
	    if ( !isInPattern( patternbid ) )
		continue;

	    const BinID relbid = center + patternbid;
	    const int gatheroffset = getRelBidOffset(relbid,getInputStepout());
	    
	    const Gather* gather = inputs_[gatheroffset];
	    if ( !gather )
		continue;

	    int trcidx = -1;
	    for ( int idx=gather->size(gather->offsetDim()==0)-1;idx>=0; idx-- )
	    {
		if ( gather->getOffsetAzimuth(idx)==oa )
		{ trcidx = idx; break; }
	    }

	    if ( trcidx==-1 )
		continue;

	    const float* trc = gather->data().getData();
	    if ( !trc )
		continue;

	    trc += gather->data().info().getOffset( trcidx, 0 );

	    for ( int idx=gather->size(Gather::zDim()==0)-1; idx>=0; idx-- )
	    {
		const float val = trc[idx];
		if ( mIsUdf(val) )
		    continue;

		outputtrc[idx] += val;
		nrvals[idx]++;
	    }
	}
    }

    for ( int idx=outputgather->size(Gather::zDim()==0)-1; idx>=0; idx-- )
    {
	if ( nrvals[idx] ) outputtrc[idx] /= nrvals[idx];
    }

    return true;
}
