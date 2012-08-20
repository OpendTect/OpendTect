/*+
 *(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 *Author:	Yuancheng Liu
 *Date:		Aug 2012
-*/

static const char* rcsID mUnusedVar = "$Id: volprocfaultangle.cc,v 1.2 2012-08-20 21:14:07 cvsyuancheng Exp $";

#include "volprocfaultangle.h"

#include "arraynd.h"
#include "fingervein.h"
#include "iopar.h"
#include "mousecursor.h"

namespace VolProc
{

FaultAngle::FaultAngle()
    : isdone_(false)
    , isazimuth_(true) 
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
	::FaultAngle fa( inputarr );
	fa.setThreshold( fltthreshold_, isfltabove_ );
	fa.setMinFaultLength( minlength_ );
	fa.compute();

	const Array3D<float>* data = isazimuth_ ? fa.getAzimuth() : fa.getDip();

	float* result = output_->getCube(0).getData();
	const float* angle = data->getData();
	const int sz = output_->getCube(0).info().getTotalSz();
	for ( int idx=0; idx<sz; idx++ )
	    result[idx] = angle[idx];
    }

    return true;
}


void FaultAngle::fillPar( IOPar& pars ) const
{
    Step::fillPar( pars );
    pars.setYN( sKeyisAzimuth(), isazimuth_ );
    pars.setYN( sKeyThinning(), dothinning_ );
    pars.setYN( sKeyMerge(), domerge_ );
    pars.setYN( sKeyIsAbove(), isfltabove_ );

    pars.set( sKeyFltLength(), minlength_ );
    pars.set( sKeyOverlapRate(), overlaprate_ );
    pars.set( sKeyThreshold(), fltthreshold_ );
}


bool FaultAngle::usePar( const IOPar& pars )
{
    if ( !Step::usePar( pars ) )
	return false;

    pars.getYN( sKeyisAzimuth(), isazimuth_ );
    pars.getYN( sKeyThinning(), dothinning_ );
    pars.getYN( sKeyMerge(), domerge_ );
    pars.getYN( sKeyIsAbove(), isfltabove_ );

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
