/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Helene Huck
 Date:		July 2017
________________________________________________________________________

-*/

#include "volprochilbert.h"
#include "arrayndslice.h"
#include "posinfo.h"
#include "seisdatapack.h"
#include "survinfo.h"
#include "trckeyzsampling.h"


static bool hasTraceData( const PosInfo::CubeData& trcposns,
			  const TrcKeySampling& tks, od_int64 trcidx )
{
    const BinID bid( tks.atIndex(trcidx) );
    const PosInfo::CubeDataPos cdatapos( trcposns.cubeDataPos( bid ) );

    return trcposns.isValid( cdatapos );
}


static bool traceIsUdf( const Array1D<float>& in )
{
    const int nrz = in.getSize( 0 );
    const float* inptr = in.getData();
    for ( int idz=0; idz<nrz; idz++ )
    {
	const bool valueisudf = inptr ? mIsUdf( inptr[idz] )
				      : mIsUdf( in.get(idz) );
	if ( !valueisudf )
	    return false;
    }

    return true;
}


static void zeroTrace( Array1D<float>& in )
{
    const int nrz = in.getSize( 0 );
    float* inptr = in.getData();
    ValueSeries<float>* instor = in.getStorage();
    if ( inptr )
    {
	for ( int idz=0; idz<nrz; idz++ )
	    inptr[idz] = 0.f;
    }
    else if ( instor )
    {
	for ( int idz=0; idz<nrz; idz++ )
	    instor->setValue( idz, 0.f );
    }
    else
    {
	for ( int idz=0; idz<nrz; idz++ )
	    in.set( idz, 0.f );
    }
}


//--------Hilbert Calculator Volume Processing Step-------------
ReportingTask* VolProc::HilbertCalculator::createTask()
{
    if ( !prepareWork() )
	return 0;

    const RegularSeisDataPack* inputdatapack = inputs_[0];
    RegularSeisDataPack* outputdatapack = getOutput();

    const Array3D<float>& seis = inputdatapack->data();
    Array3D<float>& hilbert = outputdatapack->data();

    return new HilbertCalculatorTask( seis, hilbert );
}


//--------Hilbert Calculator Task-------------

HilbertCalculatorTask::HilbertCalculatorTask( const Array2D<float>& in,
					      Array2D<float>& out )
    : ParallelTask("Hilbert Transform")
    , realdata_(in)
    , imagdata_(out)
    , is3d_(false)
    , trcposns_(0)
    , tks_(0)
{
    totalnr_ = realdata_.getSize( 0 );

    msg_ = tr("Creating imaginary traces");
}


HilbertCalculatorTask::HilbertCalculatorTask( const Array3D<float>& in,
					      Array3D<float>& out )
    : ParallelTask("Hilbert Transform")
    , realdata_(in)
    , imagdata_(out)
    , trcposns_(0)
    , tks_(0)
    , is3d_(true)
{
    totalnr_ = mCast(od_int64,realdata_.getSize( 0 ) ) *
	       mCast(od_int64,realdata_.getSize( 1 ) );

    msg_ = tr("Creating imaginary traces");
}


void HilbertCalculatorTask::setTracePositions(
					const PosInfo::CubeData& trcposns,
					const TrcKeySampling& tks )
{
    if ( trcposns.isFullyRegular() )
	return;

    trcposns_ = &trcposns;
    tks_ = &tks;
}


bool HilbertCalculatorTask::doPrepare( int )
{
    if ( imagdata_.info() != realdata_.info() )
    {
	msg_ = tr("Input and output arrays do not have the same dimension");
	return false;
    }

    nexttrcidx_ = nrIterations() - 1;

    return true;
}


bool HilbertCalculatorTask::doWork( od_int64 start, od_int64 stop, int )
{
    HilbertTransform ht;
    ht.setHalfLen( 30 );
    ht.init();

    Array1DSlice<float> realtrc( realdata_ );
    Array1DSlice<float> imagtrc( imagdata_ );

    const ArrayNDInfo& infoin = realdata_.info();
    const dim_idx_type lastdim = realdata_.get1DDim();
    realtrc.setDimMap( 0, lastdim );
    imagtrc.setDimMap( 0, lastdim );
    const int nrlines = is3d_ ? infoin.getSize( 0 ) : 0;
    const int nrtrcs = is3d_ ? infoin.getSize( 1 ) : infoin.getSize( 0 );
    ArrayNDInfo* info = 0;
    if ( is3d_ )
	info = new Array2DInfoImpl( nrlines, nrtrcs );
    else
	info = new Array1DInfoImpl( nrtrcs );

    ArrayNDIter iter( *info );
    while ( shouldContinue() )
    {
	const od_int64 trcidx = nexttrcidx_--;
	if ( trcidx<0 )
	    break;

	iter.setGlobalPos( trcidx );
	const int idx = iter[0];
	realtrc.setPos( 0, idx );
	imagtrc.setPos( 0, idx );
	if ( is3d_ )
	{
	    const int idy = iter[1];
	    realtrc.setPos( 1, idy );
	    imagtrc.setPos( 1, idy );
	}

	if ( !realtrc.init() || !imagtrc.init() )
	{
	    quickAddToNrDone( trcidx );
	    continue;
	}

	const bool hastrc = !trcposns_ || !tks_ ? true
			  : hasTraceData( *trcposns_, *tks_, trcidx );
	const bool alludfvals = hastrc ? traceIsUdf( realtrc ) : true;
	if ( !hastrc || alludfvals )
	{
	    zeroTrace( realtrc );
	    zeroTrace( imagtrc ); //Only really required for the FFT
	    quickAddToNrDone( trcidx );
	    continue;
	}

	ht.transform( realtrc, imagtrc );
	muteHeadTailHilbert( realtrc, imagtrc );

	quickAddToNrDone( trcidx );
    }

    delete info;

    return true;
}


#define mPropagateMute() \
{ \
    const float realval = realtrcptr ? realtrcptr[idz] \
				     : instor ? instor->value( idz ) \
					      : real.get( idz ); \
    if ( realval == 0.f ) \
    { \
	if ( imagtrcptr ) \
	    imagtrcptr[idz] = 0.f; \
	else if ( outstor ) \
	    outstor->setValue( idz, 0.f ); \
	else \
	    imag.set( idz, 0.f ); \
\
	continue; \
    } \
\
    break; \
}

void HilbertCalculatorTask::muteHeadTailHilbert( const Array1D<float>& real,
						 Array1D<float>& imag )
{
    const int nrz = real.getSize( 0 );
    const float* realtrcptr = real.getData();
    float* imagtrcptr = imag.getData();
    const ValueSeries<float>* instor = real.getStorage();
    ValueSeries<float>* outstor = imag.getStorage();
    for ( int idz=0; idz<nrz; idz++ )
	{ mPropagateMute() }

    for ( int idz=nrz-1; idz>=0; idz-- )
	{ mPropagateMute() }
}
