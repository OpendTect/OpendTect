/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Jan 2011
-*/


#include "fourierinterpol.h"

#include "arrayndinfo.h"
#include "fourier.h"


FourierInterpolBase::FourierInterpolBase()
    : fft_( new Fourier::CC ) 
{}


FourierInterpolBase::~FourierInterpolBase()
{
    delete fft_;
}


void FourierInterpolBase::setTargetDomain( bool fourier )
{
    if ( fourier != ((bool) fft_ ) )
    return;

    if ( fft_ )
    {
	delete fft_;
	fft_ = 0;
    }
    else
    {
	fft_ = new Fourier::CC;
    }
}



#define mDoFFT( arr )\
{\
    if ( fft_ )\
    {\
	fft_->setInputInfo( arr->info() );\
	fft_->setDir( false );\
	fft_->setNormalization( true );\
	fft_->setInput( arr->getData() );\
	fft_->setOutput( arr->getData() );\
	fft_->run( true );\
    }\
}

FourierInterpol1D::FourierInterpol1D( const TypeSet<Point>& pts,
				    const StepInterval<float>& sampling)
    : sampling_(sampling)
    , pts_(pts)
{
    sz_ = sampling_.nrSteps()+1;
}


FourierInterpol1D::~FourierInterpol1D()
{
    deepErase( arrs_ );
}


bool FourierInterpol1D::doPrepare( int nrthreads )
{
    for ( int idthread=0; idthread<nrthreads; idthread++ )
	arrs_ += new Array1DImpl<float_complex>( sz_ );

    return true;
}


bool FourierInterpol1D::doWork( od_int64 start ,od_int64 stop, int thread )
{
    if ( !sz_ )
	return false;

    const float df = Fourier::CC::getDf( sampling_.step, sz_ );

    Array1D<float_complex>& interpvals = *arrs_[thread];

    for ( int idpt=start; idpt<=stop; idpt++ )
    {
	float_complex cplxval = pts_[idpt].val_;
	if ( mIsUdf( cplxval ) ) 
	    cplxval = float_complex( 0, 0 );

	const float time = pts_[idpt].pos_; 
	const float anglesampling = -time * df;

	for ( int idx=0; idx<sz_; idx++ )
	{
	    const float angle = (float) ( 2*M_PI *anglesampling*idx );
	    const float_complex cexp = float_complex( cos(angle), sin(angle) );
	    const float_complex cplxref = cexp*cplxval;
	    float_complex outpval = interpvals.get( idx );
	    outpval += cplxref; 
	    interpvals.set( idx, outpval );
	}
	addToNrDone( 1 );
    }
    return true;
}


bool FourierInterpol1D::doFinish( bool success )
{
    if ( !success || arrs_.isEmpty() )
	return false;

    while ( arrs_.size() > 1 )
    {
	Array1D<float_complex>& arr = *arrs_.remove(1);
	for ( int idx=0; idx<sz_; idx++ )
	{
	    float_complex val = arrs_[0]->get( idx );
	    val += arr.get( idx );
	    arrs_[0]->set( idx, val );
	}
	delete &arr;
    }

    mDoFFT( arrs_[0] )

    return true;
}




FourierInterpol2D::FourierInterpol2D( const TypeSet<Point>& pts,
			    const StepInterval<float>& xsampling,
			    const StepInterval<float>& ysampling )
    : xsampling_(xsampling)
    , ysampling_(ysampling)
    , pts_(pts)
{
    szx_ = xsampling_.nrSteps()+1;
    szy_ = ysampling_.nrSteps()+1;
}


FourierInterpol2D::~FourierInterpol2D()
{
    deepErase( arrs_ );
}


bool FourierInterpol2D::doPrepare( int nrthreads )
{
    for ( int idthread=0; idthread<nrthreads; idthread++ )
	arrs_ += new Array2DImpl<float_complex>( szx_, szy_ );

    return true;
}


bool FourierInterpol2D::doWork( od_int64 start ,od_int64 stop, int thread )
{
    if ( !szx_  || !szy_ )
	return false;

    const float dfx = Fourier::CC::getDf( xsampling_.step, szx_ );
    const float dfy = Fourier::CC::getDf( ysampling_.step, szy_ );

    Array2D<float_complex>& interpvals = *arrs_[thread];

    for ( int idpt=start; idpt<=stop; idpt++ )
    {
	float_complex cplxval = pts_[idpt].val_;
	if ( mIsUdf( cplxval ) ) 
	    cplxval = float_complex( 0, 0 );

	const float timex = pts_[idpt].xpos_; 
	const float timey = pts_[idpt].ypos_; 

	const float xanglesampling = -timex * dfx;
	const float yanglesampling = -timey * dfy;

	for ( int idx=0; idx<szx_; idx++ )
	{
	    const float anglex = (float) ( 2*M_PI *xanglesampling*idx );

	    for ( int idy=0; idy<szy_; idy++ )
	    {
		const float angley = (float) ( 2*M_PI *yanglesampling*idy );
		const float angle = anglex + angley;
		const float_complex cexp = float_complex( cos(angle), 
							    sin(angle) );
		const float_complex cplxref = cexp*cplxval;
		float_complex outpval = interpvals.get( idx, idy );
		outpval += cplxref; 
		interpvals.set( idx, idy, outpval );
	    }
	}
	addToNrDone( 1 );
    }
    return true;
}


bool FourierInterpol2D::doFinish( bool success )
{
    if ( !success || arrs_.isEmpty() )
	return false;

    while ( arrs_.size() > 1 )
    {
	Array2D<float_complex>& arr = *arrs_.remove(1);
	for ( int idx=0; idx<szx_; idx++ )
	{
	    for ( int idy=0; idy<szy_; idy++ )
	    {
		float_complex val = arrs_[0]->get( idx, idy );
		val += arr.get( idx, idy );
		arrs_[0]->set( idx, idy, val );
	    }
	}
	delete &arr;
    }

    mDoFFT( arrs_[0] )

    return true;
}




FourierInterpol3D::FourierInterpol3D( const TypeSet<Point>& pts,
			    const StepInterval<float>& xsampling,
			    const StepInterval<float>& ysampling,
			    const StepInterval<float>& zsampling )
    : xsampling_(xsampling)
    , ysampling_(ysampling)
    , zsampling_(zsampling)
    , pts_(pts)
{
    szx_ = xsampling_.nrSteps()+1;
    szy_ = ysampling_.nrSteps()+1;
    szz_ = zsampling_.nrSteps()+1;
}


FourierInterpol3D::~FourierInterpol3D()
{
    deepErase( arrs_ );
}


bool FourierInterpol3D::doPrepare( int nrthreads )
{
    for ( int idthread=0; idthread<nrthreads; idthread++ )
	arrs_ += new Array3DImpl<float_complex>( szx_, szy_, szz_ );

    return true;
}


bool FourierInterpol3D::doWork( od_int64 start ,od_int64 stop, int thread )
{
    if ( !szx_  || !szy_ || !szz_ )
	return false;

    const float dfx = Fourier::CC::getDf( xsampling_.step, szx_ );
    const float dfy = Fourier::CC::getDf( ysampling_.step, szy_ );
    const float dfz = Fourier::CC::getDf( zsampling_.step, szz_ );

    Array3DImpl<float_complex>& interpvals = *arrs_[thread];

    for ( int idpt=start; idpt<=stop; idpt++ )
    {
	float_complex cplxval = pts_[idpt].val_;
	if ( mIsUdf( cplxval ) ) 
	    cplxval = float_complex( 0, 0 );

	const float timex = pts_[idpt].xpos_-xsampling_.start; 
	const float timey = pts_[idpt].ypos_-ysampling_.start; 
	const float timez = pts_[idpt].zpos_-zsampling_.start;

	const float xanglesampling = -timex * dfx;
	const float yanglesampling = -timey * dfy;
	const float zanglesampling = -timez * dfz;

	for ( int idx=0; idx<szx_; idx++ )
	{
	    const float anglex = (float) ( 2*M_PI *xanglesampling*idx );

	    for ( int idy=0; idy<szy_; idy++ )
	    {
		const float angley = (float) ( 2*M_PI *yanglesampling*idy );

		for ( int idz=0; idz<szz_; idz++ )
		{
		    const float anglez = (float) ( 2*M_PI *zanglesampling*idz );

		    const float angle = anglex+angley+anglez;

		    const float_complex cexp =
			float_complex( cos(angle), sin(angle) );

		    const float_complex cplxref = cexp*cplxval;
		    float_complex outpval = interpvals.get( idx, idy, idz );
		    outpval += cplxref;
		    interpvals.set( idx, idy, idz, outpval );
		}
	    }
	}
	addToNrDone( 1 );
    }
    return true;
}


bool FourierInterpol3D::doFinish( bool success )
{
    if ( !success || arrs_.isEmpty() )
	return false;

    while ( arrs_.size() > 1 )
    {
	Array3DImpl<float_complex>& arr = *arrs_.remove(1);
	for ( int idx=0; idx<szx_; idx++ )
	{
	    for ( int idy=0; idy<szy_; idy++ )
	    {
		for ( int idz=0; idz<szz_; idz++ )
		{
		    float_complex val = arrs_[0]->get( idx, idy, idz );
		    val += arr.get( idx, idy, idz );
		    arrs_[0]->set( idx, idy, idz, val );
		}
	    }
	}
	delete &arr;
    }

    mDoFFT( arrs_[0] )

    return true;
}
