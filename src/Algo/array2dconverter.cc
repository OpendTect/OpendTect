/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "array2dconverter.h"

#include "arrayndimpl.h"
#include "interpol2d.h"
#include "posidxpair2coord.h"
#include "survinfo.h"


Array2DConverter::Array2DConverter( const Array2D<float>& arrin )
    : ParallelTask("Converting grid")
    , arrin_(arrin)
{}


Array2DConverter::~Array2DConverter()
{}



Array2DFromXYConverter::Array2DFromXYConverter( const Array2D<float>& arrin,
						const Coord& origin,
						const Coord& step )
    : Array2DConverter(arrin)
    , origin_(origin), step_(step)
    , tks_(false)
{
}


Array2DFromXYConverter::~Array2DFromXYConverter()
{}


void Array2DFromXYConverter::setOutputSampling( const TrcKeySampling& tks )
{ tks_ = tks; }


od_int64 Array2DFromXYConverter::nrIterations() const
{
    return tks_.totalNr();
}


uiString Array2DFromXYConverter::uiNrDoneText() const
{
    return tr("# nodes done");
}


int Array2DFromXYConverter::maxNrThreads() const
{
    return ParallelTask::maxNrThreads();
}


bool Array2DFromXYConverter::doPrepare( int nrthreads )
{
    arrout_ = new Array2DImpl<float>( tks_.nrInl(), tks_.nrCrl() );
    arrout_->setAll( mUdf(float) );
    return true;
}


bool Array2DFromXYConverter::doWork( od_int64 start, od_int64 stop, int )
{
    const SamplingData<double> sdx( origin_.x, step_.x );
    const SamplingData<double> sdy( origin_.y, step_.y );
    for ( auto idx=start; idx<=stop; idx++, addToNrDone(1) )
    {
	const TrcKey tk = tks_.trcKeyAt( idx );
	const Coord crd = SI().transform( tk.binID() );

	const float xidxf = sdx.getfIndex( crd.x );
	const float yidxf = sdy.getfIndex( crd.y );
	if ( xidxf<0 || yidxf<0 )
	    continue;

	const int yidx = sCast(int,yidxf);
	const int xidx = sCast(int,xidxf);
	const int row = yidx;
	const int col = xidx;
	if ( !arrin_.info().validPos(row,col) ||
	     !arrin_.info().validPos(row+1,col+1) )
	    continue;

	const float dx = xidxf - xidx;
	const float dy = yidxf - yidx;

	const float z00 = arrin_.get( row, col );
	if ( mIsUdf(z00) )
	    continue;

	const float z01 = arrin_.get( row, col+1 );
	const float z10 = arrin_.get( row+1, col );
	const float z11 = arrin_.get( row+1, col+1 );

	float zval = Interpolate::linearReg2DWithUdf( z00, z01,
						      z10, z11, dy, dx );
	arrout_->set( tks_.inlIdx(tk.inl()), tks_.crlIdx(tk.crl()), zval );
    }

    return true;
}
