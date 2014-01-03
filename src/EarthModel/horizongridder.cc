/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Raman K Singh
 Date:          Mar 2013
________________________________________________________________________

-*/

static const char* rcsID mUsedVar = "$Id$";

#include "horizongridder.h"

#include "arrayndimpl.h"
#include "faulttrace.h"
#include "statruncalc.h"
#include "survinfo.h"

const char* HorizonGridder::sKeyMethod()	{ return "Method"; }
const char* HorizonGridder::sKeyNrFaults()	{ return "Nr Faults"; }
const char* HorizonGridder::sKeyFaultID()	{ return "Fault ID"; }


mImplFactory( HorizonGridder, HorizonGridder::factory );

HorizonGridder::HorizonGridder()
    : fltdataprov_(0)
{}

HorizonGridder::~HorizonGridder()
{ delete fltdataprov_; }


void HorizonGridder::setFaultIds( const TypeSet<MultiID>& mids )
{ faultids_ = mids; }


void HorizonGridder::setHorSampling( const HorSampling& hs )
{
    hs_ = hs;
}

bool HorizonGridder::init( TaskRunner* tr )
{
    infomsg_ = "";
    if ( !fltdataprov_ && !faultids_.isEmpty() )
	fltdataprov_ = new FaultTrcDataProvider();

    if ( fltdataprov_ && !fltdataprov_->init(faultids_,hs_,tr) )
    {
	infomsg_ = "Cannot read Faults";
	return false;
    }

    return true;
}


bool HorizonGridder::blockSrcPoints( const float* data,
				      const od_int64* sources, int nrsrc,
				ObjectSet< TypeSet<int> >& blocks ) const
{
    if ( !nrsrc )
	return false;

    deepErase( blocks );
    blocks += new TypeSet<int>( 1, 0 );
    for ( int idx=1; idx<nrsrc; idx++ )
    {
	const BinID curbid = hs_.atIndex( sources[idx] );
	const float curz = data[sources[idx]];
	bool isassigned = false;
	for ( int idy=0; idy<blocks.size(); idy++ )
	{
	    TypeSet<int>& curblock = *blocks[idy];
	    bool iscrossing = false;
	    for ( int idz=0; idz<curblock.size(); idz++ )
	    {
		const BinID bid = hs_.atIndex( sources[curblock[idz]] );
		const float zval = data[sources[curblock[idz]]];
		if ( fltdataprov_->isCrossingFault(curbid,curz,bid,zval) )
		{
		    iscrossing = true;
		    break;
		}
	    }

	    if ( !iscrossing )
	    {
		curblock += idx;
		isassigned = true;
	    }
	}

	if ( !isassigned )
	    blocks += new TypeSet<int>( 1, idx );
    }

    return true;
}


bool HorizonGridder::setFrom( float* data, od_int64 target,
			       const od_int64* sources, const float* weights,
			       int nrsrc )
{
    ObjectSet< TypeSet<int> > blocks;
    blockSrcPoints( data, sources, nrsrc, blocks );
    const BinID tgtbid = hs_.atIndex( target );
    int maxnrsrc = 0;
    float tgtz = mUdf(float);
    for ( int idx=0; idx<blocks.size(); idx++ )
    {
	const TypeSet<int>& curblock = *blocks[idx];
	Stats::RunCalc<float> calc( Stats::CalcSetup(true)
				    .require( Stats::Average ) );
	for ( int idy=0; idy<curblock.size(); idy++ )
	    calc.addValue( data[sources[curblock[idy]]],
		    	   weights[curblock[idy]] );

	const float calcz = (float) calc.average();
	const BinID firstbid = hs_.atIndex( sources[curblock[0]] );
	const float firstz = data[sources[curblock[0]]];
	if ( !fltdataprov_->isCrossingFault(tgtbid,calcz,firstbid,firstz)
		&& curblock.size() > maxnrsrc )
	{
	    tgtz = calcz;
	    maxnrsrc = curblock.size();
	}
    }

    data[target] = tgtz;
    return true;
}


bool HorizonGridder::setArray2D( Array2D<float>& arr, TaskRunner* tr )
{
    return true;
}


bool HorizonGridder::fillPar( IOPar& par ) const
{
    par.set( sKeyNrFaults(), faultids_.size() );
    for ( int idx=0; idx<faultids_.size(); idx++ )
	par.set( IOPar::compKey(sKeyFaultID(),idx), faultids_[idx] );

    return true;
}


bool HorizonGridder::usePar( const IOPar& par )
{
    faultids_.erase();
    int nrfaults = 0;
    if ( !par.get(sKeyNrFaults(),nrfaults) )
	return true;

    for ( int idx=0; idx<nrfaults; idx++ )
    {
	MultiID fltid;
	if ( !par.get(IOPar::compKey(sKeyFaultID(),idx),fltid) )
	    return false;

	faultids_ += fltid;
    }

    return true;
}


void InvDistHor3DGridder::setHorSampling( const HorSampling& hs )
{
    HorizonGridder::setHorSampling( hs );
    setRowStep( SI().inlDistance() * hs.step.inl() );
    setColStep( SI().crlDistance() * hs.step.crl() );
    setOrigin( hs.start );
}


bool InvDistHor3DGridder::setArray2D( Array2D<float>& arr, TaskRunner* tr )
{
    return setArray( arr, tr );
}


bool InvDistHor3DGridder::initFromArray( TaskRunner* tr )
{
    if ( !InverseDistanceArray2DInterpol::initFromArray(tr) )
	return false;

    return HorizonGridder::init( tr );
}


void InvDistHor3DGridder::setFrom( od_int64 target,
					  const od_int64* sources,
					  const float* weights, int nrsrc )
{
    float* ptr = arr_->getData();
    if ( !ptr || !fltdataprov_ )
	Array2DInterpol::setFrom( target, sources, weights, nrsrc );
    else
	HorizonGridder::setFrom( ptr, target, sources, weights, nrsrc );
}


bool InvDistHor3DGridder::fillPar( IOPar& par ) const
{
    return InverseDistanceArray2DInterpol::fillPar(par)
	&& HorizonGridder::fillPar(par);
}


bool InvDistHor3DGridder::usePar( const IOPar& par )
{
    return InverseDistanceArray2DInterpol::usePar(par)
	&& HorizonGridder::usePar(par);
}


void TriangulationHor3DGridder::setHorSampling( const HorSampling& hs )
{
    HorizonGridder::setHorSampling( hs );
    setRowStep( SI().inlDistance() * hs.step.inl() );
    setColStep( SI().crlDistance() * hs.step.crl() );
    setOrigin( hs.start );
}


bool TriangulationHor3DGridder::setArray2D( Array2D<float>& arr,
					    TaskRunner* tr )
{
    return setArray( arr, tr );
}


bool TriangulationHor3DGridder::initFromArray( TaskRunner* tr )
{
    if ( !TriangulationArray2DInterpol::initFromArray(tr) )
	return false;

    return HorizonGridder::init( tr );
}


void TriangulationHor3DGridder::setFrom( od_int64 target,
					  const od_int64* sources,
					  const float* weights, int nrsrc )
{
    float* ptr = arr_->getData();
    if ( !ptr || !fltdataprov_ )
	Array2DInterpol::setFrom( target, sources, weights, nrsrc );
    else
	HorizonGridder::setFrom( ptr, target, sources, weights, nrsrc );
}


bool TriangulationHor3DGridder::fillPar( IOPar& par ) const
{
    return TriangulationArray2DInterpol::fillPar(par)
	&& HorizonGridder::fillPar(par);
}


bool TriangulationHor3DGridder::usePar( const IOPar& par )
{
    return TriangulationArray2DInterpol::usePar(par)
	&& HorizonGridder::usePar(par);
}


void ExtensionHor3DGridder::setHorSampling( const HorSampling& hs )
{
    HorizonGridder::setHorSampling( hs );
    setRowStep( SI().inlDistance() * hs.step.inl() );
    setColStep( SI().crlDistance() * hs.step.crl() );
    setOrigin( hs.start );
}


bool ExtensionHor3DGridder::setArray2D( Array2D<float>& arr, TaskRunner* tr )
{
    return setArray( arr, tr );
}


bool ExtensionHor3DGridder::fillPar( IOPar& par ) const
{
    return ExtensionArray2DInterpol::fillPar(par)
	&& HorizonGridder::fillPar(par);
}


bool ExtensionHor3DGridder::usePar( const IOPar& par )
{
    return ExtensionArray2DInterpol::usePar(par)
	&& HorizonGridder::usePar(par);
}
