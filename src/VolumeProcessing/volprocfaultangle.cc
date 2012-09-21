/*+
 *(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 *Author:	Yuancheng Liu
 *Date:		Aug 2012
-*/

static const char* rcsID mUnusedVar = "$Id$";

#include "volprocfaultangle.h"

#include "arraynd.h"
#include "fingervein.h"
#include "iopar.h"
#include "mousecursor.h"

namespace VolProc
{

FaultAngle::FaultAngle()
    : isdone_(false)
    , outopt_(Azimuth)  
    , minlength_(15)
    , dothinning_(true)
    , domerge_(true)
    , overlaprate_(1)
    , fltthreshold_(0)
    , isfltabove_(true)  
{}


FaultAngle::~FaultAngle()
{
    Step::releaseData();
}


bool FaultAngle::computeBinID( const BinID& bid, int )
{
    if ( !isOK() )
	return false;

    if ( !isdone_ )
    {
	isdone_ = true;

	MouseCursorChanger cursorlock( MouseCursor::Wait );
	
	const Array3D<float>& inputarr = input_->getCube( 0 );
	FaultOrientation fo;
	fo.setThreshold( fltthreshold_, isfltabove_ );
	fo.setMinFaultLength( minlength_ );
	fo.compute( inputarr, outopt_!=FaultFlag, outopt_==Dip, 0 );

	const int sz = output_->getCube(0).info().getTotalSz();
	float* result = output_->getCube(0).getData();
	
	if ( outopt_!=FaultFlag )
	{
	    const Array3D<float>* data = 
		outopt_==Azimuth ? fo.getAzimuth() : fo.getDip();
    	    if ( !data ) return false;

	    const float* angle = data->getData();
	    for ( int idx=0; idx<sz; idx++ )
    		result[idx] = angle[idx];
	}
	else
	{
    	    const Array3D<bool>* data = 
    		fo.getFaultConfidence(FaultOrientation::Median);
    	    if ( !data ) return false;
    
	    const bool* bina  = data->getData();
    	    for ( int idx=0; idx<sz; idx++ )
    		result[idx] = bina[idx] ? 1.f : 0.f;
	}
    }

    return true;
}


void FaultAngle::fillPar( IOPar& pars ) const
{
    Step::fillPar( pars );
    pars.setYN( sKeyThinning(), dothinning_ );
    pars.setYN( sKeyMerge(), domerge_ );
    pars.setYN( sKeyIsAbove(), isfltabove_ );

    pars.set( sKeyOutputOpt(), outopt_ );
    pars.set( sKeyFltLength(), minlength_ );
    pars.set( sKeyOverlapRate(), overlaprate_ );
    pars.set( sKeyThreshold(), fltthreshold_ );
}


bool FaultAngle::usePar( const IOPar& pars )
{
    if ( !Step::usePar( pars ) )
	return false;

    pars.getYN( sKeyThinning(), dothinning_ );
    pars.getYN( sKeyMerge(), domerge_ );
    pars.getYN( sKeyIsAbove(), isfltabove_ );

    int outputopt;
    pars.get( sKeyOutputOpt(), outputopt );
    outopt_ = (FaultAngle::OutputOption)outputopt;

    pars.get( sKeyFltLength(), minlength_ );
    pars.get( sKeyOverlapRate(), overlaprate_ );
    pars.get( sKeyThreshold(), fltthreshold_ );

    return true;
}


bool FaultAngle::isOK() const
{
    if ( !output_ || !output_->nrCubes() || !input_ )
	return false;

    return true;
}

}; //namespace
