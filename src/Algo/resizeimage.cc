/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : August 2010
-*/

static const char* rcsID mUnusedVar = "$Id$";

#include "resizeimage.h"

#include "genericnumer.h"
#include "odimage.h"

ImageResizer::ImageResizer( const OD::RGBImage& input, OD::RGBImage& output )
    : lanczossize_( 2 )
    , input_( input )
    , inputowner_( false )
    , inputimage_( 0 )
    , output_( output )
    , outputimage_( 0 )
{
    inputimage_ = input_.getData();
    if ( !inputimage_ )
    {
	unsigned char* buf = new unsigned char[input_.bufferSize()];
	inputimage_ = buf;
	inputowner_ = true;
	input_.fill( buf );
    }

    inputsize_[0] = input.getSize(true);
    inputsize_[1] = input.getSize(false);

    outputimage_ = output_.getData();
    if ( !outputimage_ )
    {
	outputimage_ = new unsigned char[output_.bufferSize()];
	outputowner_ = true;
	output_.fill( outputimage_ );
    }

    outputsize_[0] = output.getSize(true);
    outputsize_[1] = output.getSize(false);
}


ImageResizer::~ImageResizer()
{
    if ( inputowner_ )
	delete [] inputimage_;

    if ( outputowner_ )
	delete [] outputimage_;
}


od_int64 ImageResizer::nrIterations() const
{ return outputsize_[0]*outputsize_[1]; }


bool ImageResizer::doPrepare(int)
{
    nrcomponents_ = input_.nrComponents();
    if ( nrcomponents_!=output_.nrComponents() )
	return false;

    factor0_ = outputsize_[0] / (double) inputsize_[0];
    factor1_ = outputsize_[1] / (double) inputsize_[1];

    scale0_ = mMIN(factor0_, 1 );
    scale1_ = mMIN(factor1_, 1 );
    support0_ = lanczossize_ / scale0_; //radius of filter on source
    support1_ = lanczossize_ / scale1_; //radius of filter on source

    if ( support0_<=0.5 ) { support0_ = 0.5 + 1E-12; scale0_ = 1; }
    if ( support1_<=0.5 ) { support1_ = 0.5 + 1E-12; scale1_ = 1; }

    return true;
}


bool ImageResizer::doWork( od_int64 start, od_int64 stop,int)

{
    int idx0 = start/outputsize_[1];
    int idx1 = start%outputsize_[1];
    int idx = start;
    Color colres;
    for ( ; idx<=stop; idx0++ )
    {
	const double center0 = (idx0+0.5) / factor0_; //position in source
	const size_t start0 = (size_t)std::max(center0-support0_+0.5, (double)0);
	const size_t stop0  = (size_t)std::min(center0+support0_+0.5, (double)inputsize_[0] );
	const size_t nmax0 = stop0-start0;
	for ( ; idx<=stop && idx1<outputsize_[1]; idx1++ )
	{
	    const double center1 = (idx1+0.5) / factor1_; //position in source
	    const size_t start1 = (size_t)std::max(center1-support1_+0.5, (double)0);
	    const size_t stop1  = (size_t)std::min(center1+support1_+0.5, (double)inputsize_[1] );
	    const size_t nmax1 = stop1-start1;

	    double s0 = (start0 - center0+0.5)*scale0_;
	    double s1 = (start1 - center1+0.5)*scale1_;

	    double wsum = 0;
	    double sums[] = { 0, 0, 0, 0 };

	    for ( unsigned int k0=0; k0<nmax0; s0+=scale0_, k0++ )
	    {
		for ( unsigned int k1=0; k0<nmax1; s1+=scale1_, k1++ )
		{
		    const double weight = LanczosKernel( lanczossize_, Math::Sqrt(s0*s0+s1*s1) );
		    const int inputoffset = ((start0+k0)*inputsize_[1] + start1+k1)*nrcomponents_;
		    const unsigned char* col = inputimage_+inputoffset;
		    for ( char idc=nrcomponents_-1; idc>=0; idc-- )
			sums[idc] += col[idc]*weight;

		    wsum += weight;
		}
	    }

	    for ( char idc=nrcomponents_-1; idc>=0; idc-- )
	    {
		double res = sums[idc];
		if ( wsum!=0 )
		    res /= wsum;

		int ires = mNINT32( res );
		if ( ires<0 ) ires=0; else if ( ires>256 ) ires = 256;

		outputimage_[idx*nrcomponents_+idc] = ires;
	    }

	    idx++;
	}

	idx1 = 0;
    }

    return true;
}


bool ImageResizer::doFinish( bool success )
{
    if ( !success )
	return false;

    if ( !outputowner_ )
	return true;

    return output_.put( outputimage_ );
}


void ImageResizer::setLanczosSize( int n )
{ lanczossize_ = n; }
