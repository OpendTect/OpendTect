/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl/Y.Liu
 * DATE     : 8-20-2010
-*/

static const char* rcsID = "$Id: fourier.cc,v 1.9 2011/09/16 10:54:03 cvskris Exp $";

#include "fourier.h"
#include "odmemory.h"

namespace Fourier
{


mImplFactory( CC, CC::factory );

CC* CC::createDefault()
{
    CC* res = factory().create( factory().getDefaultName() );

    if ( !res )
	res = new CC;

    return res;
}
 


CC::CC()
   : normalize_( false ) 
{}


bool CC::setup()
{
    if ( !GenericTransformND::setup() )
	return false;
    
    for ( int idx=0; idx<transforms_.size(); idx++ )
    {
	static_cast<CC1D*>(transforms_[idx])->
	    setNormalization( normalize_ );
    }

    return true;
}


void CC::setNormalization( bool yn )
{ curdim_ = -1; normalize_ = yn; }


float CC::getNyqvist( float samplespacing )
{ return 0.5 / samplespacing; }


float CC::getDf( float samplespacing, int nrsamples )
{ return 1. / (samplespacing * nrsamples); }


bool CC::isFast( int sz ) const 
{ return sz==getFastSize(sz); }


int CC::getFastSize( int nmin ) const
{
    return  CC::CC1D::getFastSize( nmin );
}


CC::CC1D::CC1D()
    : higheststart_( -1 )
    , normalize_( false )
    , direction_( -1 )
    , dopfa_( true )
{}


CC::CC1D::~CC1D()
{
    deepErase( ffts_ );
}


bool CC::CC1D::init()
{
    dopfa_ = getFastSize(sz_)==sz_;
    direction_ = forward_ ? -1 : 1;
    return true;
}


bool CC::CC1D::doPrepare( int nrthreads )
{
    if ( !cinput_ || !coutput_ )
	return false;

    init();

    if ( cinput_!=coutput_ )
    {
	if ( higheststart_==-1 )
	{
	    if ( batchstarts_ )
	    {
		for ( int idx=0; idx<nr_; idx++ )
		{
		    if ( batchstarts_[idx]>higheststart_ )
			higheststart_ = batchstarts_[idx];
		}
	    }
	    else
	    {
		higheststart_ = batchsampling_ * ( nr_ - 1 );
	    }
	}

	const int lastsample = higheststart_ + ( sz_ - 1 ) * sampling_;
	MemCopier<float_complex> copier( coutput_, cinput_, lastsample+1 );
	copier.execute();
    }

    if ( !dopfa_ )
    {
	deepErase( ffts_ );
	for ( int idx=0; idx<nrthreads; idx++ )
	{
    	    Fourier::FFTCC1D* cc1d = new Fourier::FFTCC1D();
    	    cc1d->setSample( sampling_ );
    	    cc1d->setDir( forward_ );
    	    cc1d->setSize( sz_ );
    	    ffts_ += cc1d;
	}
    }

    return true;
}


bool CC::CC1D::doWork( od_int64 start, od_int64 stop, int threadidx )
{
    if ( dopfa_ && start!=stop )
    {
	const int nr = stop-start+1;
	if ( batchstarts_ )
	{
	    const int* offsets = batchstarts_ + start;
	    pfacc( direction_, sz_, sampling_, nr, offsets, coutput_ );
	}
	else
	{
	    pfacc( direction_, sz_, sampling_, nr, batchsampling_, coutput_ );
	}

	if ( !normalize_ || direction_!=1 )
	    return true;

	for ( int idx=start; idx<=stop; idx++ )
	{
	    const int offset = batchstarts_[idx];

	    float_complex* output = coutput_ + offset;
	
	    for ( int idy=0; idy<sz_; idy++, output += sampling_ )
		*output /= sz_;
	}

	return true;
    }

    for ( int idx=start; idx<=stop; idx++ )
    {
	const int offset = batchstarts_
	    ? batchstarts_[idx] : idx*batchsampling_;

	float_complex* output = coutput_ + offset;

	if ( dopfa_ )
	{
	    if ( sampling_==1 )
		pfacc( direction_, sz_, output );
	    else
		pfacc( direction_, sz_, sampling_, output );
	}
	else
	{
	    ffts_[threadidx]->run( output );
	}

	if ( normalize_ && direction_==1 )
	{
	    for ( int idy=0; idy<sz_; idy++, output += sampling_ )
		*output /= sz_;
	}
    }

    return true;
}

GenericTransformND::Transform1D* CC::createTransform() const
{
    return new CC::CC1D();
}


# define mSin60	0.86602540378443865	
# define mCos72	0.30901699437494742
# define mSin72	0.95105651629515357

FFTCC1D::FFTCC1D()
    : forward_( true )
    , normalize_( false )
    , size_( -1 )
    , permutation0_( 0 )
    , permutation1_( 0 )
    , rtmp_( 0 )
    , itmp_( 0 )
    , cosv_( 0 )
    , sinv_( 0 )
    , rmfid_( 0 )
    , sample_( 1 )
    , totalsmp_( 0 )
    , data_( 0 )
    , extsz_( 0 )
    , curf_( 0 )
    , exp_( 0 )
    , sin2_( 0 )
    , cycleid_( 0 )
{}


void FFTCC1D::cleanUp()
{
    delete [] rtmp_; rtmp_ = 0;
    delete [] itmp_; itmp_ = 0;
    delete [] cosv_; cosv_ = 0;
    delete [] sinv_; sinv_ = 0;
    delete [] permutation0_; permutation0_ = 0;
    delete [] permutation1_; permutation1_ = 0;
}


bool FFTCC1D::setSize( int sz )
{
    if ( sz<1 )
	return false;

    if ( rtmp_ )
    	cleanUp();

    size_ = sz;
    totalsmp_ = size_ * sample_;
    mTryAlloc( permutation0_, int[sz] );
    mTryAlloc( rtmp_,  float[sz] );
    mTryAlloc( itmp_,  float[sz] );
    mTryAlloc( cosv_,  float[sz] );
    mTryAlloc( sinv_,  float[sz] );

    return permutation0_ && rtmp_ && itmp_ && cosv_ && sinv_ && getSizeFactors()
	&& setupPermutation();
}


bool FFTCC1D::run( float_complex* data ) 
{
    if ( !data ) 
	return false;
    
    if ( size_<2 ) 
	return true;

    data_ = data-1;
    rdata_ = (float*) data_;
    idata_ = rdata_+1;

    totalsmp_ = size_ * sample_;
    extsz_ = totalsmp_;
    curf_ = 0;   
    const double smppi = (forward_ ? -M_PI : M_PI) * sample_;
    const int nrfactors = factors_.size();
    
    for ( int fidx=0; fidx<nrfactors; fidx++ )
    {
 	exp_ = smppi / extsz_; 
 	sin2_ = 2 * sin(exp_) * sin(exp_);
 	exp_ = sin(2*exp_);
	
 	if ( factors_[fidx]==2 )
 	{
	    extsz_ /= 2;
 	    if ( doFactor2() )
 		return doFinish();
 	}
 	else if ( factors_[fidx]==4 )
 	{
	    extsz_ /= 4;
 	    if ( doFactor4() )
 		return doFinish();
 	}
 	else
 	{
 	    const int psz = extsz_;
 	    extsz_ /= factors_[fidx];
 	    if ( factors_[fidx]==3 )
 		doFactor3();
 	    else if ( factors_[fidx]==5 )
 		doFactor5();
 	    else
 		doOtherFactor( factors_[fidx], psz );
	    
 	    if ( fidx == nrfactors-1 ) 
 		return doFinish();

	    doRotation( psz );	       
 	}
    }

   return doFinish();
}


bool FFTCC1D::getSizeFactors()
{
    int remain = size_;          
    while ( !(remain%16) ) 
    {
	factors_ += 4;           
	remain /= 16;
    }
   
    int factor = 3;
    int f2 = 9;
    do 
    {
	while ( !(remain % f2) ) 
	{
	    factors_ += factor;   
	    remain /= f2;
	} 
       
	factor += 2;
	f2 = factor * factor;
    } while ( f2 <= remain );
   
    if ( remain<=4 ) 
    {
	rmfid_ = factors_.size(); 
        if ( remain != 1 ) 
	    factors_ += remain; 		
    } 
    else 
    {
	if ( remain==(remain / 4 << 2) ) 
	{
	    factors_ += 2; 
	    remain /= 4;
	}

	rmfid_ = factors_.size(); 
	factor = 2;
	do 
	{
	    if ( remain % factor == 0 ) 
	    {
		factors_ += factor; 	
		remain /= factor;
	    }
	    factor = ((factor + 1) / 2 << 1) + 1;
	} while ( factor <= remain );
    }

    if ( rmfid_ ) 
    {
	factor = rmfid_;
	do
	{ 
	    factors_ += factors_[--factor];
	} while ( factor );
    }

    permutfactors_ = factors_;    
    return true;
}


bool FFTCC1D::doFactor2() const
{
    int cidx = 1;
    const int lastsmp = sample_ * (size_-1);
    const int extsz2 = 2 * extsz_;
 
    do 
    {
	do 
	{
	    const int cidx2 = 2 * cidx;
	    const int idx = cidx2 + extsz2;
	    const float ar = rdata_[idx];
	    const float ai = idata_[idx];
	    rdata_[idx] = rdata_[cidx2] - ar;
	    idata_[idx] = idata_[cidx2] - ai;
	    rdata_[cidx2] += ar;
	    idata_[cidx2] += ai;
	    cidx += extsz2;
	} while ( cidx <= lastsmp );

	cidx -= lastsmp;
    } while ( cidx <= sample_ );
    
    if ( cidx > extsz_ )
	return true; 	

    int cnt = extsz_ + 2;
    const int samp2 = sample_ + sample_;

    do 
    {
	float c1 = 1.0 - sin2_;
	float s1 = exp_;
	int idx;
	do 
	{	       
	    do 
	    {
		const float_complex c1s1( c1, s1 );
		do 
		{
		    idx = cidx + extsz_;
		    const int cidx2 = cidx * 2;
		    const int idx2 = idx *2;
		    const float ar = rdata_[cidx2] - rdata_[idx2];
		    const float ai = idata_[cidx2] - idata_[idx2];
		    rdata_[cidx2] += rdata_[idx2];
		    idata_[cidx2] += idata_[idx2];
		    rdata_[idx2] = ar * c1 - ai * s1;
		    idata_[idx2] = ar * s1 + ai * c1;
		    cidx = idx + extsz_;
		} while ( cidx < totalsmp_ );
	       	
		idx = cidx - totalsmp_;
		c1 = -c1;
		cidx = cnt - idx;
	    } while ( cidx > idx );
	    
	    const float val = c1 - ( sin2_ * c1 + exp_ * s1 );
	    s1 = exp_ * c1 - sin2_ * s1 + s1;
	    c1 = 2.0 - ( val * val + s1 * s1 );
	    s1 *= c1;
	    c1 *= val;
	    cidx += sample_;
	} while ( cidx < idx );
	
	cnt += samp2;
	cidx = (cnt-extsz_) / 2 + sample_;
    } while ( cidx <= samp2 );   

   return false; 
}


void FFTCC1D::doFactor3() const
{
    int cidx = 2;
    const float s60 = forward_ ? -mSin60 : mSin60;   
    const int lastsmp = sample_ * (size_-1) * 2;
    const int extsz2 = extsz_ * 2;

    do 
    {
 	do 
 	{
 	    const int idx0 = cidx + extsz2;
 	    const int idx1 = idx0 + extsz2;

	    float ar = rdata_[cidx];
	    float ai = idata_[cidx];
	    float br = rdata_[idx0] + rdata_[idx1];
	    float bi = idata_[idx0] + idata_[idx1];
	    rdata_[cidx] = ar + br;
	    idata_[cidx] = ai + bi;
	    ar -= 0.5 * br;
	    ai -= 0.5 * bi;
	    br = (rdata_[idx0] - rdata_[idx1]) * s60;
	    bi = (idata_[idx0] - idata_[idx1]) * s60;
	    rdata_[idx0] = ar - bi;
	    rdata_[idx1] = ar + bi;
	    idata_[idx0] = ai + br;
	    idata_[idx1] = ai - br;
	    cidx = idx1 + extsz2;

 	} while ( cidx < lastsmp );
	
 	cidx -= lastsmp;
    } while ( cidx <= extsz2 );
}

	  
bool FFTCC1D::doFactor4() const
{ 
    int cidx = 1;
    const int extsz2 = extsz_ * 2;
    const int extsz4 = extsz_ * 4;
    const int extsz6 = extsz_ * 6;
    const int smpdiffextsz = sample_ - extsz_;
    const int smpdiffsz = sample_ - totalsmp_;
    
    do 
    {
 	float c0 = 1;
 	float c1 = 0;
	float c2 = 0;
	float s0 = 0;
	float s1 = 0;
	float s2 = 0;
 	do 
 	{
 	    do 
 	    {
		const int cidx2 = 2 * cidx;
 		const int idx0 = cidx2 + extsz2;
 		const int idx1 = cidx2 + extsz4;
 		const int idx2 = cidx2 + extsz6;
 		float a0r = rdata_[cidx2] + rdata_[idx1];
 		float a1r = rdata_[cidx2] - rdata_[idx1];
 		float a2r = rdata_[idx0] + rdata_[idx2];
 		float a3r = rdata_[idx0] - rdata_[idx2];
 		float a0i = idata_[cidx2] + idata_[idx1];
 		float a1i = idata_[cidx2] - idata_[idx1];
 		float a2i = idata_[idx0] + idata_[idx2];
 		float a3i = idata_[idx0] - idata_[idx2];
 		rdata_[cidx2] = a0r + a2r;
 		idata_[cidx2] = a0i + a2i;
 		a2r = a0r - a2r;
 		a2i = a0i - a2i;
 		if ( forward_ ) 
 		{
 		    a0r = a1r + a3i;
 		    a0i = a1i - a3r;
 		    a1r -= a3i;
 		    a1i += a3r;
 		} 
 		else 
 		{
 		    a0r = a1r - a3i;
 		    a0i = a1i + a3r;
 		    a1r += a3i;
 		    a1i -= a3r;
 		}
		
		if ( mIsZero(s0,1e-8) )
		{
		    rdata_[idx0] = a0r;
		    rdata_[idx1] = a2r;
		    rdata_[idx2] = a1r;
		    idata_[idx0] = a0i;
		    idata_[idx1] = a2i;
		    idata_[idx2] = a1i;
		}
		else
		{
	    	    rdata_[idx0] = a0r * c0 - a0i * s0; 
    		    rdata_[idx1] = a2r * c1 - a2i * s1;
    		    rdata_[idx2] = a1r * c2 - a1i * s2;
	    	    idata_[idx0] = a0r * s0 + a0i * c0; 
    		    idata_[idx1] = a2r * s1 + a2i * c1;
    		    idata_[idx2] = a1r * s2 + a1i * c2;
		}
		cidx += extsz4;
    		    
 	    } while ( cidx <= totalsmp_ );
 	    
 	    c1 = c0 - (sin2_ * c0 + exp_ * s0);
 	    s0 = exp_ * c0 - sin2_ * s0 + s0;
 	    c0 = 2.0 - (c1 * c1 + s0 * s0);
 	    s0 *= c0;
 	    c0 *= c1;
 	    c1 = c0 * c0 - s0 * s0;
 	    s1 = 2.0 * c0 * s0;
 	    c2 = c1 * c0 - s1 * s0;
 	    s2 = c1 * s0 + s1 * c0;
 	    cidx += smpdiffsz;
 	} while ( cidx <= extsz_ );
	
 	cidx += smpdiffextsz;
    } while ( cidx <= sample_ );
    
    if ( extsz_ == sample_ )
 	return true; 

    return false;
}


void FFTCC1D::doFactor5() const
{
    const int lastsmp = sample_ * (size_-1) * 2;
    const int extsz2 = extsz_ * 2;
    const float s72 = forward_ ? -mSin72 : mSin72;
    const float c2 = mCos72 * mCos72 - s72 * s72;
    const float s2 = 2.0 * mCos72 * s72;
    
    int cidx = 2;
    
    do 
    {
 	do 
 	{
 	    const int idx0 = cidx + extsz2;
 	    const int idx1 = idx0 + extsz2;
 	    const int idx2 = idx1 + extsz2;
 	    const int idx3 = idx2 + extsz2;
 	    const float ar0 = rdata_[idx0] + rdata_[idx3];
 	    const float ai0 = idata_[idx0] + idata_[idx3];
 	    const float ar1 = rdata_[idx0] - rdata_[idx3];
 	    const float ai1 = idata_[idx0] - idata_[idx3];
 	    const float ar2 = rdata_[idx1] + rdata_[idx2];
 	    const float ai2 = idata_[idx1] + idata_[idx2];
 	    const float ar3 = rdata_[idx1] - rdata_[idx2];
 	    const float ai3 = idata_[idx1] - idata_[idx2];
 	    const float ar4 = rdata_[cidx];
 	    const float ai4 = idata_[cidx];
	    
 	    rdata_[cidx] = ar0 + ar2 +ar4; 
 	    idata_[cidx] = ai0 + ai2 +ai4; 
 	    
	    float ar5 = ar0 * mCos72 + ar2 * c2 + ar4;	
	    float ai5 = ai0 * mCos72 + ai2 * c2 + ai4;	
 	    float ar6 = ar1 * s72 + ar3 * s2;
 	    float ai6 = ai1 * s72 + ai3 * s2;
 	    
	    rdata_[idx0] = ar5 - ai6;
	    idata_[idx0] = ai5 + ar6;
 	    rdata_[idx3] = ar5 + ai6;
 	    idata_[idx3] = ai5 - ar6;
 	    
	    ar5 = ar0 * c2 + ar2 * mCos72 + ar4;	
	    ai5 = ai0 * c2 + ai2 * mCos72 + ai4;	
 	    ar6 = ar1 * s2 - ar3 * s72;
 	    ai6 = ai1 * s2 - ai3 * s72;
 	    
	    rdata_[idx1] = ar5 - ai6;
	    idata_[idx1] = ai5 + ar6;
 	    rdata_[idx2] = ar5 + ai6;
 	    idata_[idx2] = ai5 - ar6;
 	    cidx = idx3 + extsz2;
 	} while ( cidx < lastsmp );
	
 	cidx -= lastsmp;
    } while ( cidx <= extsz2 );
}


void FFTCC1D::doOtherFactor( int factor, int psz )
{
    if ( factor != curf_ ) 
    {
 	curf_ = factor;
 	float s1 = 2*M_PI / (double) factor;
 	const float c1 = cos(s1);
 	s1 = forward_ ? -sin(s1) : sin(s1);
 	if ( curf_ > size_ )
 	    return;
      	
 	cosv_[curf_-1] = 1;
 	sinv_[curf_-1] = 0;
 	int idx = 0;
 	do 
 	{
 	    factor--;
	    cosv_[idx] = cosv_[factor] * c1 + sinv_[factor] * s1;
	    sinv_[idx] = cosv_[factor] * s1 - sinv_[factor] * c1;
 	    cosv_[factor-1] = cosv_[idx];
 	    sinv_[factor-1] = -sinv_[idx];
 	    idx++;
 	} while ( idx < factor-1 );
    }

    int cidx = 2;
    const int lastsmp = sample_ * (size_-1) * 2;   
    const int extsz2 = extsz_ * 2;
    const int localpsz = psz * 2;
  
    do 
    {
 	do 
 	{
	    float ar0 = rdata_[cidx];
	    float ai0 = idata_[cidx];
	    float ar1 = ar0;
	    float ai1 = ai0;
 	    int idx0 = 1;
 	    int idx1 = cidx;
 	    int idx2 = cidx + localpsz;
 	    idx1 += extsz2;
 	    do 
 	    {
 		idx2 -= extsz2;
 		rtmp_[idx0] = rdata_[idx1] + rdata_[idx2];
 		itmp_[idx0] = idata_[idx1] + idata_[idx2];
		ar0 += rtmp_[idx0];
 		ai0 += itmp_[idx0];
 		idx0++;
 		rtmp_[idx0] = rdata_[idx1] - rdata_[idx2];
 		itmp_[idx0] = idata_[idx1] - idata_[idx2];
 		idx0++;
 		idx1 += extsz2;
 	    } while ( idx1 < idx2 );
 	    
 	    rdata_[cidx] = ar0;
 	    idata_[cidx] = ai0;
 	    idx1 = cidx;
 	    idx2 = cidx + localpsz;
 	    idx0 = 1;
	    int fidx;
 	    do 
 	    {
 		int pidx = idx0;
 		idx1 += extsz2;
 		idx2 -= extsz2;
 		ar0 = ar1;
 		ai0 = ai1;
 		float ra = 0, ia = 0;
 		fidx = 1;
 		do 
 		{
		    const int pidx1 = pidx - 1;
 		    ar0 += rtmp_[fidx] * cosv_[pidx1];
 		    ai0 += itmp_[fidx] * cosv_[pidx1];
		    fidx++;
 		    ra += rtmp_[fidx] * sinv_[pidx1];
 		    ia += itmp_[fidx] * sinv_[pidx1];
		    fidx++;
 		    pidx += idx0;
 		    if ( pidx > curf_ ) 
			pidx -= curf_;
 		} while ( fidx < curf_ );
		
 		fidx = curf_ - idx0;
 		rdata_[idx1] = ar0 - ia;
 		idata_[idx1] = ai0 + ra;
 		rdata_[idx2] = ar0 + ia;
 		idata_[idx2] = ai0 - ra;
 		idx0++;
 	    } while ( idx0 < fidx );
	    
 	    cidx += localpsz;
 	} while ( cidx <= lastsmp );
	
 	cidx -= lastsmp;
    } while ( cidx <= extsz2 );
}


void FFTCC1D::doRotation( int psz ) const
{
    int cidx = sample_ + 1;
    const int smp2 = 2 * sample_;
    const int sizediff = extsz_ - totalsmp_;
    const int sampleminuspsz = sample_ - psz;
    const int smp2minusextsz = smp2 - extsz_;
    const float startc1 = 1.0 - sin2_;
    
    do 
    {
 	float c1 = startc1;
 	float s0 = exp_;
 	do 
 	{
 	    float c0 = c1;
 	    float s1 = s0;
 	    cidx += extsz_;
 	    do 
 	    {
		const float_complex cs1(c1, s1);
 		do 
 		{
 		    data_[cidx] *= cs1;
 		    cidx += psz;
 		} while ( cidx <= totalsmp_ );
		
 		const float tt = s0 * s1;
 		s1 = s0 * c1 + c0 * s1;
 		c1 = c0 * c1 - tt;
 		cidx += sizediff;
 	    } while ( cidx <= psz );
 
	    c1 = c0 - (sin2_ * c0 + exp_ * s0);
 	    s0 += exp_ * c0 - sin2_ * s0;
 	    c0 = 2.0 - (c1 * c1 + s0 * s0);
 	    s0 *= c0;
 	    c1 *= c0;
 	    cidx += sampleminuspsz;
 	} while ( cidx <= extsz_ );
 	cidx += smp2minusextsz;
    } while ( cidx <= smp2 );
}


bool FFTCC1D::setupPermutation()
{
    /*  permutation for square factors of n */
    const int nrfactors = permutfactors_.size();
    int tmpidy = sample_;
    permutation0_[0] = totalsmp_;
    
    if ( rmfid_ ) 
    {
 	int fidx = 2 * rmfid_ + 1;
 	if ( nrfactors < fidx )
	    fidx--;
 
 	int idx = 0;
 	permutation0_[fidx] = tmpidy;
 	do 
 	{
 	    permutation0_[idx+1] = permutation0_[idx] / permutfactors_[idx];
 	    permutation0_[fidx-1] = permutation0_[fidx] * permutfactors_[idx];
 	    idx++;
 	    fidx--;
 	} while ( idx < fidx-1 );
    }
    
    if ( (rmfid_ << 1) + 1 >= nrfactors )
 	return true;

    mTryAlloc( permutation1_, int[size_] );
    memcpy( permutation1_, permutation0_, size_*sizeof(int) ); 
    
    /* permutation for square-free factors of n */
    if ( !rmfid_ ) permutfactors_ += 1;
    int dd = nrfactors - rmfid_;
    permutfactors_[dd] = 1;
    do 
    {
 	permutfactors_[dd-1] *= permutfactors_[dd];
 	dd--;
    } while ( dd != rmfid_ );
   
    const int nn = permutfactors_[rmfid_] - 1;
    const int rmfid = rmfid_+1;
    for ( int idx=0, idx0=0; idx<nn; idx++ ) 
    {
  	int f0 = permutfactors_[rmfid_];
  	int f1 = permutfactors_[rmfid];
  	int fidx = rmfid;
  	idx0 += f1;
  	while ( idx0 >= f0 ) 
  	{
   	    idx0 -= f0;
   	    f0 = f1;
   	    fidx++;
   	    f1 = permutfactors_[fidx];
   	    idx0 += f1;
  	}
  	permutation1_[idx] = idx0;
    }
    
    /* determine the permutation cycles of length greater than 1 */
    for ( int idx=0; ; ) 
    {
 	int tidx;
 	do 
 	{
 	    tidx = permutation1_[idx++];
 	} while ( tidx < 0 );
 	
 	if ( tidx != idx ) 
 	{
   	    do 
   	    {
   		const int pidx = tidx-1;
   		tidx = permutation1_[pidx];
   		permutation1_[pidx] = -tidx;
   	    } while ( tidx != idx );
   	    cycleid_ = tidx;
 	} 
 	else 
 	{
 	    permutation1_[idx-1] = -idx;
 	    if ( idx == nn )
 		break;		
 	}
    }

    return true;
}


#define mRetFinish( yn ) \
{ \
    if (  !forward_ & normalize_ ) \
    { \
	const float scaling = 1.0 / size_; \
	for ( int idy = 0; idy < size_; idy++ ) \
	    data_[idy] *= scaling; \
    } \
    return yn; \
}


bool FFTCC1D::doFinish()
{   
    const int nrfactors = factors_.size();
    int tmpidy = sample_;
    
    if ( rmfid_ ) 
    {
 	int fidx = 2 * rmfid_ + 1;
 	if ( nrfactors < fidx )
	    fidx--;

	fidx -= fidx/2;
	tmpidy = permutation0_[fidx];
 
  	int idx = 0;
  	const int idx0 = permutation0_[1];
  	int idx1 = idx0 + 1;
  	int tidx = sample_ + 1;
	
	bool cont = true;
	while ( cont )
	{
	    cont = false;
	    do 
	    {
		swap( data_[tidx], data_[idx1] );
		tidx += sample_;
		idx1 += idx0;
	    } while ( idx1 < totalsmp_ );

	    do 
	    {
		do 
		{
		    idx1 -= permutation0_[idx];
		    idx++;
		    idx1 = permutation0_[idx+1] + idx1;
		} while ( idx1 > permutation0_[idx] );

		idx = 0;
		do 
		{
		    if ( tidx < idx1 )
		    {
			cont = true;
			break;
		    }

		    tidx += sample_;
		    idx1 += idx0;
		} while ( idx1 < totalsmp_ );

		if ( cont ) 
		    break;
	    } while ( tidx < totalsmp_ );
	}
    }
    
    if ( !permutation1_ )
 	mRetFinish( true );

    int nt = totalsmp_;
    for ( ; ; ) 
    {
 	int idx = cycleid_ + 1;
 	nt -= permutation0_[rmfid_];
 	if ( nt < 0 )
 	    break;	
 	
	const int idy0 = nt - sample_ + 1;
 	do 
 	{
 	    do 
 	    {
 		idx--;
 	    } while ( permutation1_[idx - 1] < 0 );
 	    
 	    int idx0 = tmpidy;
 	    do 
 	    {
 		const int minfidx = mMIN(idx0,totalsmp_);
 		idx0 -= minfidx;
 		int pidx = permutation1_[idx-1];
 		int tidx = tmpidy * pidx + idy0 + idx0;
 		int kidx = tidx + minfidx;
 		int idx1 = 0;
 		do 
 		{
 		    rtmp_[idx1] = data_[kidx].real();
 		    itmp_[idx1] = data_[kidx].imag();
 		    idx1++;
 		    kidx -= sample_;
 		} while ( kidx != tidx );
 		
 		do 
 		{
 		    kidx = tidx + minfidx;
 		    idx1 = kidx - tmpidy * (pidx + permutation1_[pidx-1]);
 		    pidx = -permutation1_[pidx-1];
 		    do 
 		    {
 			data_[kidx] = data_[idx1];
 			kidx -= sample_;
 			idx1 -= sample_;
 		    } while ( kidx != tidx );
 		    tidx = idx1;
 		} while ( pidx != idx );
 		kidx = tidx + minfidx;
 		
		idx1 = 0;
 		do 
 		{
#ifdef __win__
		    data_[kidx].real( rtmp_[idx1] );
		    data_[kidx].imag( itmp_[idx1] );
#else
 		    data_[kidx].real() = rtmp_[idx1];
 		    data_[kidx].imag() = itmp_[idx1];
#endif
 		    idx1++;
 		    kidx -= sample_;		    
 		} while ( kidx != tidx );
 	    } while ( idx0 );
 	} while ( idx != 1 );
    }
    
    mRetFinish( true );
}



/* The reminer of this file contains code  modified from Seismic Un*x, and may
   be used the following conditions:

    Copyright 2007, Colorado School of Mines,
    All rights reserved.


    Redistribution and use in source and binary forms, with or 
    without modification, are permitted provided that the following 
    conditions are met:

    *	Redistributions of source code must retain the above copyright 
	notice, this list of conditions and the following disclaimer.
    *   Redistributions in binary form must reproduce the above 
	copyright notice, this list of conditions and the following 
	disclaimer in the documentation and/or other materials provided 
	with the distribution.
    *   Neither the name of the Colorado School of Mines nor the names of
	its contributors may be used to endorse or promote products 
	derived from this software without specific prior written permission.

    Warranty Disclaimer:
    THIS SOFTWARE IS PROVIDED BY THE COLORADO SCHOOL OF MINES AND CONTRIBUTORS 
    "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
    LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS 
    FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE 
    COLORADO SCHOOL OF MINES OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
    INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, 
    BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; 
    LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER 
    CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, 
    STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING 
    IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE 
    POSSIBILITY OF SUCH DAMAGE.


    Export Restriction Disclaimer:
    We believe that CWP/SU: Seismic Un*x is a low technology product that does
    not appear on the Department of Commerce CCL list of restricted exports.
    Accordingly, we believe that our product meets the qualifications of
    an ECCN (export control classification number) of EAR99 and we believe
    it fits the qualifications of NRR (no restrictions required), and
    is thus not subject to export restrictions of any variety.

    Approved Reference Format:
    In publications, please refer to SU as per the following example:
    Cohen, J. K. and Stockwell, Jr. J. W., (200_), CWP/SU: Seismic Un*x 
    Release No. __: an open source software  package for seismic 
    research and processing, 
    Center for Wave Phenomena, Colorado School of Mines.
*/

#define P120 0.120536680
#define P142 0.142314838
#define P173 0.173648178
#define P222 0.222520934
#define P239 0.239315664
#define P281 0.281732557
#define P342 0.342020143
#define P354 0.354604887
#define P382 0.382683432
#define P415 0.415415013
#define P433 0.433883739
#define P464 0.464723172
#define P540 0.540640817
#define P559 0.559016994
#define P568 0.568064747
#define P587 0.587785252
#define P623 0.623489802
#define P642 0.642787610
#define P654 0.654860734
#define P663 0.663122658
#define P707 0.707106781
#define P748 0.748510748
#define P755 0.755749574
#define P766 0.766044443
#define P781 0.781831482
#define P822 0.822983866
#define P841 0.841253533
#define P866 0.866025404
#define P885 0.885456026
#define P900 0.900968868
#define P909 0.909631995
#define P923 0.923879533
#define P935 0.935016243
#define P939 0.939692621
#define P951 0.951056516
#define P959 0.959492974
#define P970 0.970941817
#define P974 0.974927912
#define P984 0.984807753
#define P989 0.989821442
#define P992 0.992708874
#define NFAX 10


#define mPFACCIMPL( nextsampleinc ) \
    register float *z=(float*)cz; \
    register int j00,j01,j2,j3,j4,j5,j6,j7,j8,j9,j10,j11,j12,j13,j14,j15; \
    int nleft,jfax,ifac,jfac,jinc,jmax,ndiv,m,mm=0,mu=0,l; \
    float t1r,t1i,t2r,t2i,t3r,t3i,t4r,t4i,t5r,t5i, \
	    t6r,t6i,t7r,t7i,t8r,t8i,t9r,t9i,t10r,t10i, \
	    t11r,t11i,t12r,t12i,t13r,t13i,t14r,t14i,t15r,t15i, \
	    t16r,t16i,t17r,t17i,t18r,t18i,t19r,t19i,t20r,t20i, \
	    t21r,t21i,t22r,t22i,t23r,t23i,t24r,t24i,t25r,t25i, \
	    t26r,t26i,t27r,t27i,t28r,t28i,t29r,t29i,t30r,t30i, \
	    t31r,t31i,t32r,t32i,t33r,t33i,t34r,t34i,t35r,t35i, \
	    t36r,t36i,t37r,t37i,t38r,t38i,t39r,t39i,t40r,t40i, \
	    t41r,t41i,t42r,t42i, \
	    y1r,y1i,y2r,y2i,y3r,y3i,y4r,y4i,y5r,y5i, \
	    y6r,y6i,y7r,y7i,y8r,y8i,y9r,y9i,y10r,y10i, \
	    y11r,y11i,y12r,y12i,y13r,y13i,y14r,y14i,y15r,y15i, \
	    c1,c2,c3,c4,c5,c6,c7,c8,c9,c10,c11,c12; \
 \
    /* keep track of n left after dividing by factors */ \
    nleft = n; \
 \
    /* begin loop over possible factors (from biggest to smallest) */ \
    for (jfax=0; jfax<NFAX; jfax++) { \
 \
	    /* skip if not a mutually prime factor of n */ \
    ifac = kfax[jfax]; \
    ndiv = nleft/ifac; \
    if (ndiv*ifac!=nleft) continue; \
 \
	    /* update n left and determine n divided by factor */ \
    nleft = ndiv; \
    m = n/ifac; \
 \
	    /* determine rotation factor mu and stride mm */ \
    for (jfac=1; jfac<=ifac; jfac++) { \
		    mu = jfac; \
		    mm = jfac*m; \
		    if (mm%ifac==1) break; \
	    } \
 \
	    /* adjust rotation factor for sign of transform */ \
    if (isign<0) mu = ifac-mu; \
 \
	    /* compute stride, limit, and pointers */ \
    jinc = (nextsampleinc)*mm; \
    jmax = (nextsampleinc)*n; \
    j00 = 0; \
    j01 = j00+jinc; \
 \
	    /* if factor is 2 */ \
    if (ifac==2) { \
		    for (l=0; l<m; l++) { \
			    const int j00i = j00+1; \
			    const int j01i = j01+1; \
			    t1r = z[j00]-z[j01]; \
			    t1i = z[j00i]-z[j01i]; \
			    z[j00] = z[j00]+z[j01]; \
			    z[j00i] = z[j00i]+z[j01i]; \
			    z[j01] = t1r; \
			    z[j01i] = t1i; \
			    const int jt = j01+(nextsampleinc); \
			    j01 = j00+(nextsampleinc); \
			    j00 = jt; \
		    } \
		    continue; \
	    } \
    j2 = j01+jinc; \
    if (j2>=jmax) j2 = j2-jmax; \
 \
	    /* if factor is 3 */ \
    if (ifac==3) { \
		    if (mu==1) \
			    c1 = P866; \
		    else \
			    c1 = -P866; \
		    for (l=0; l<m; l++) { \
			    const int j00i = j00+1; \
			    const int j01i = j01+1; \
			    const int j2i = j2+1; \
			    t1r = z[j01]+z[j2]; \
			    t1i = z[j01i]+z[j2i]; \
			    y1r = z[j00]-0.5*t1r; \
			    y1i = z[j00i]-0.5*t1i; \
			    y2r = c1*(z[j01]-z[j2]); \
			    y2i = c1*(z[j01i]-z[j2i]); \
			    z[j00] = z[j00]+t1r; \
			    z[j00i] = z[j00i]+t1i; \
			    z[j01] = y1r-y2i; \
			    z[j01i] = y1i+y2r; \
			    z[j2] = y1r+y2i; \
			    z[j2i] = y1i-y2r; \
			    const int jt = j2+(nextsampleinc); \
			    j2 = j01+(nextsampleinc); \
			    j01 = j00+(nextsampleinc); \
			    j00 = jt; \
		    } \
		    continue; \
	    } \
	    j3 = j2+jinc; \
	    if (j3>=jmax) j3 = j3-jmax; \
 \
	    /* if factor is 4 */ \
	    if (ifac==4) { \
		    if (mu==1) \
			    c1 = 1.0; \
		    else \
			    c1 = -1.0; \
		    for (l=0; l<m; l++) { \
			    const int j00i = j00+1; \
			    const int j01i = j01+1; \
			    const int j2i = j2+1; \
			    const int j3i = j3+1; \
			    t1r = z[j00]+z[j2]; \
			    t1i = z[j00i]+z[j2i]; \
			    t2r = z[j01]+z[j3]; \
			    t2i = z[j01i]+z[j3i]; \
			    y1r = z[j00]-z[j2]; \
			    y1i = z[j00i]-z[j2i]; \
			    y3r = c1*(z[j01]-z[j3]); \
			    y3i = c1*(z[j01i]-z[j3i]); \
			    z[j00] = t1r+t2r; \
			    z[j00i] = t1i+t2i; \
			    z[j01] = y1r-y3i; \
			    z[j01i] = y1i+y3r; \
			    z[j2] = t1r-t2r; \
			    z[j2i] = t1i-t2i; \
			    z[j3] = y1r+y3i; \
			    z[j3i] = y1i-y3r; \
			    const int jt = j3+(nextsampleinc); \
			    j3 = j2+(nextsampleinc); \
			    j2 = j01+(nextsampleinc); \
			    j01 = j00+(nextsampleinc); \
			    j00 = jt; \
		    } \
		    continue; \
	    } \
	    j4 = j3+jinc; \
	    if (j4>=jmax) j4 = j4-jmax; \
 \
	    /* if factor is 5 */ \
	    if (ifac==5) { \
		    if (mu==1) { \
			    c1 = P559; \
			    c2 = P951; \
			    c3 = P587; \
		    } else if (mu==2) { \
			    c1 = -P559; \
			    c2 = P587; \
			    c3 = -P951; \
		    } else if (mu==3) { \
			    c1 = -P559; \
			    c2 = -P587; \
			    c3 = P951; \
		    } else {  \
			    c1 = P559; \
			    c2 = -P951; \
			    c3 = -P587; \
		    } \
		    for (l=0; l<m; l++) { \
			    const int j00i = j00+1; \
			    const int j01i = j01+1; \
			    const int j2i = j2+1; \
			    const int j3i = j3+1; \
			    const int j4i = j4+1; \
			    t1r = z[j01]+z[j4]; \
			    t1i = z[j01i]+z[j4i]; \
			    t2r = z[j2]+z[j3]; \
			    t2i = z[j2i]+z[j3i]; \
			    t3r = z[j01]-z[j4]; \
			    t3i = z[j01i]-z[j4i]; \
			    t4r = z[j2]-z[j3]; \
			    t4i = z[j2i]-z[j3i]; \
			    t5r = t1r+t2r; \
			    t5i = t1i+t2i; \
			    t6r = c1*(t1r-t2r); \
			    t6i = c1*(t1i-t2i); \
			    t7r = z[j00]-0.25*t5r; \
			    t7i = z[j00i]-0.25*t5i; \
			    y1r = t7r+t6r; \
			    y1i = t7i+t6i; \
			    y2r = t7r-t6r; \
			    y2i = t7i-t6i; \
			    y3r = c3*t3r-c2*t4r; \
			    y3i = c3*t3i-c2*t4i; \
			    y4r = c2*t3r+c3*t4r; \
			    y4i = c2*t3i+c3*t4i; \
			    z[j00] = z[j00]+t5r; \
			    z[j00i] = z[j00i]+t5i; \
			    z[j01] = y1r-y4i; \
			    z[j01i] = y1i+y4r; \
			    z[j2] = y2r-y3i; \
			    z[j2i] = y2i+y3r; \
			    z[j3] = y2r+y3i; \
			    z[j3i] = y2i-y3r; \
			    z[j4] = y1r+y4i; \
			    z[j4i] = y1i-y4r; \
			    const int jt = j4+(nextsampleinc); \
			    j4 = j3+(nextsampleinc); \
			    j3 = j2+(nextsampleinc); \
			    j2 = j01+(nextsampleinc); \
			    j01 = j00+(nextsampleinc); \
			    j00 = jt; \
		    } \
		    continue; \
	    } \
	    j5 = j4+jinc; \
	    if (j5>=jmax) j5 = j5-jmax; \
	    j6 = j5+jinc; \
	    if (j6>=jmax) j6 = j6-jmax; \
 \
	    /* if factor is 7 */ \
	    if (ifac==7) { \
		    if (mu==1) { \
			    c1 = P623; \
			    c2 = -P222; \
			    c3 = -P900; \
			    c4 = P781; \
			    c5 = P974; \
			    c6 = P433; \
		    } else if (mu==2) { \
			    c1 = -P222; \
			    c2 = -P900; \
			    c3 = P623; \
			    c4 = P974; \
			    c5 = -P433; \
			    c6 = -P781; \
		    } else if (mu==3) { \
			    c1 = -P900; \
			    c2 = P623; \
			    c3 = -P222; \
			    c4 = P433; \
			    c5 = -P781; \
			    c6 = P974; \
		    } else if (mu==4) { \
			    c1 = -P900; \
			    c2 = P623; \
			    c3 = -P222; \
			    c4 = -P433; \
			    c5 = P781; \
			    c6 = -P974; \
		    } else if (mu==5) { \
			    c1 = -P222; \
			    c2 = -P900; \
			    c3 = P623; \
			    c4 = -P974; \
			    c5 = P433; \
			    c6 = P781; \
		    } else { \
			    c1 = P623; \
			    c2 = -P222; \
			    c3 = -P900; \
			    c4 = -P781; \
			    c5 = -P974; \
			    c6 = -P433; \
		    } \
		    for (l=0; l<m; l++) { \
			    const int j00i = j00+1; \
			    const int j01i = j01+1; \
			    const int j2i = j2+1; \
			    const int j3i = j3+1; \
			    const int j4i = j4+1; \
			    const int j5i = j5+1; \
			    const int j6i = j6+1; \
			    t1r = z[j01]+z[j6]; \
			    t1i = z[j01i]+z[j6i]; \
			    t2r = z[j2]+z[j5]; \
			    t2i = z[j2i]+z[j5i]; \
			    t3r = z[j3]+z[j4]; \
			    t3i = z[j3i]+z[j4i]; \
			    t4r = z[j01]-z[j6]; \
			    t4i = z[j01i]-z[j6i]; \
			    t5r = z[j2]-z[j5]; \
			    t5i = z[j2i]-z[j5i]; \
			    t6r = z[j3]-z[j4]; \
			    t6i = z[j3i]-z[j4i]; \
			    t7r = z[j00]-0.5*t3r; \
			    t7i = z[j00i]-0.5*t3i; \
			    t8r = t1r-t3r; \
			    t8i = t1i-t3i; \
			    t9r = t2r-t3r; \
			    t9i = t2i-t3i; \
			    y1r = t7r+c1*t8r+c2*t9r; \
			    y1i = t7i+c1*t8i+c2*t9i; \
			    y2r = t7r+c2*t8r+c3*t9r; \
			    y2i = t7i+c2*t8i+c3*t9i; \
			    y3r = t7r+c3*t8r+c1*t9r; \
			    y3i = t7i+c3*t8i+c1*t9i; \
			    y4r = c6*t4r-c4*t5r+c5*t6r; \
			    y4i = c6*t4i-c4*t5i+c5*t6i; \
			    y5r = c5*t4r-c6*t5r-c4*t6r; \
			    y5i = c5*t4i-c6*t5i-c4*t6i; \
			    y6r = c4*t4r+c5*t5r+c6*t6r; \
			    y6i = c4*t4i+c5*t5i+c6*t6i; \
			    z[j00] = z[j00]+t1r+t2r+t3r; \
			    z[j00i] = z[j00i]+t1i+t2i+t3i; \
			    z[j01] = y1r-y6i; \
			    z[j01i] = y1i+y6r; \
			    z[j2] = y2r-y5i; \
			    z[j2i] = y2i+y5r; \
			    z[j3] = y3r-y4i; \
			    z[j3i] = y3i+y4r; \
			    z[j4] = y3r+y4i; \
			    z[j4i] = y3i-y4r; \
			    z[j5] = y2r+y5i; \
			    z[j5i] = y2i-y5r; \
			    z[j6] = y1r+y6i; \
			    z[j6i] = y1i-y6r; \
			    const int jt = j6+(nextsampleinc); \
			    j6 = j5+(nextsampleinc); \
			    j5 = j4+(nextsampleinc); \
			    j4 = j3+(nextsampleinc); \
			    j3 = j2+(nextsampleinc); \
			    j2 = j01+(nextsampleinc); \
			    j01 = j00+(nextsampleinc); \
			    j00 = jt; \
		    } \
		    continue; \
	    } \
	    j7 = j6+jinc; \
	    if (j7>=jmax) j7 = j7-jmax; \
 \
	    /* if factor is 8 */ \
	    if (ifac==8) { \
		    if (mu==1) { \
			    c1 = 1.0; \
			    c2 = P707; \
		    } else if (mu==3) { \
			    c1 = -1.0; \
			    c2 = -P707; \
		    } else if (mu==5) { \
			    c1 = 1.0; \
			    c2 = -P707; \
		    } else { \
			    c1 = -1.0; \
			    c2 = P707; \
		    } \
		    c3 = c1*c2; \
		    for (l=0; l<m; l++) { \
			    const int j00i = j00+1; \
			    const int j01i = j01+1; \
			    const int j2i = j2+1; \
			    const int j3i = j3+1; \
			    const int j4i = j4+1; \
			    const int j5i = j5+1; \
			    const int j6i = j6+1; \
			    const int j7i = j7+1; \
			    t1r = z[j00]+z[j4]; \
			    t1i = z[j00i]+z[j4i]; \
			    t2r = z[j00]-z[j4]; \
			    t2i = z[j00i]-z[j4i]; \
			    t3r = z[j01]+z[j5]; \
			    t3i = z[j01i]+z[j5i]; \
			    t4r = z[j01]-z[j5]; \
			    t4i = z[j01i]-z[j5i]; \
			    t5r = z[j2]+z[j6]; \
			    t5i = z[j2i]+z[j6i]; \
			    t6r = c1*(z[j2]-z[j6]); \
			    t6i = c1*(z[j2i]-z[j6i]); \
			    t7r = z[j3]+z[j7]; \
			    t7i = z[j3i]+z[j7i]; \
			    t8r = z[j3]-z[j7]; \
			    t8i = z[j3i]-z[j7i]; \
			    t9r = t1r+t5r; \
			    t9i = t1i+t5i; \
			    t10r = t3r+t7r; \
			    t10i = t3i+t7i; \
			    t11r = c2*(t4r-t8r); \
			    t11i = c2*(t4i-t8i); \
			    t12r = c3*(t4r+t8r); \
			    t12i = c3*(t4i+t8i); \
			    y1r = t2r+t11r; \
			    y1i = t2i+t11i; \
			    y2r = t1r-t5r; \
			    y2i = t1i-t5i; \
			    y3r = t2r-t11r; \
			    y3i = t2i-t11i; \
			    y5r = t12r-t6r; \
			    y5i = t12i-t6i; \
			    y6r = c1*(t3r-t7r); \
			    y6i = c1*(t3i-t7i); \
			    y7r = t12r+t6r; \
			    y7i = t12i+t6i; \
			    z[j00] = t9r+t10r; \
			    z[j00i] = t9i+t10i; \
			    z[j01] = y1r-y7i; \
			    z[j01i] = y1i+y7r; \
			    z[j2] = y2r-y6i; \
			    z[j2i] = y2i+y6r; \
			    z[j3] = y3r-y5i; \
			    z[j3i] = y3i+y5r; \
			    z[j4] = t9r-t10r; \
			    z[j4i] = t9i-t10i; \
			    z[j5] = y3r+y5i; \
			    z[j5i] = y3i-y5r; \
			    z[j6] = y2r+y6i; \
			    z[j6i] = y2i-y6r; \
			    z[j7] = y1r+y7i; \
			    z[j7i] = y1i-y7r; \
			    const int jt = j7+(nextsampleinc); \
			    j7 = j6+(nextsampleinc); \
			    j6 = j5+(nextsampleinc); \
			    j5 = j4+(nextsampleinc); \
			    j4 = j3+(nextsampleinc); \
			    j3 = j2+(nextsampleinc); \
			    j2 = j01+(nextsampleinc); \
			    j01 = j00+(nextsampleinc); \
			    j00 = jt; \
		    } \
		    continue; \
	    } \
	    j8 = j7+jinc; \
	    if (j8>=jmax) j8 = j8-jmax; \
 \
	    /* if factor is 9 */ \
	    if (ifac==9) { \
		    if (mu==1) { \
			    c1 = P866; \
			    c2 = P766; \
			    c3 = P642; \
			    c4 = P173; \
			    c5 = P984; \
		    } else if (mu==2) { \
			    c1 = -P866; \
			    c2 = P173; \
			    c3 = P984; \
			    c4 = -P939; \
			    c5 = P342; \
		    } else if (mu==4) { \
			    c1 = P866; \
			    c2 = -P939; \
			    c3 = P342; \
			    c4 = P766; \
			    c5 = -P642; \
		    } else if (mu==5) { \
			    c1 = -P866; \
			    c2 = -P939; \
			    c3 = -P342; \
			    c4 = P766; \
			    c5 = P642; \
		    } else if (mu==7) { \
			    c1 = P866; \
			    c2 = P173; \
			    c3 = -P984; \
			    c4 = -P939; \
			    c5 = -P342; \
		    } else { \
			    c1 = -P866; \
			    c2 = P766; \
			    c3 = -P642; \
			    c4 = P173; \
			    c5 = -P984; \
		    } \
		    c6 = c1*c2; \
		    c7 = c1*c3; \
		    c8 = c1*c4; \
		    c9 = c1*c5; \
		    for (l=0; l<m; l++) { \
			    const int j00i = j00+1; \
			    const int j01i = j01+1; \
			    const int j2i = j2+1; \
			    const int j3i = j3+1; \
			    const int j4i = j4+1; \
			    const int j5i = j5+1; \
			    const int j6i = j6+1; \
			    const int j7i = j7+1; \
			    const int j8i = j8+1; \
			    t1r = z[j3]+z[j6]; \
			    t1i = z[j3i]+z[j6i]; \
			    t2r = z[j00]-0.5*t1r; \
			    t2i = z[j00i]-0.5*t1i; \
			    t3r = c1*(z[j3]-z[j6]); \
			    t3i = c1*(z[j3i]-z[j6i]); \
			    t4r = z[j00]+t1r; \
			    t4i = z[j00i]+t1i; \
			    t5r = z[j4]+z[j7]; \
			    t5i = z[j4i]+z[j7i]; \
			    t6r = z[j01]-0.5*t5r; \
			    t6i = z[j01i]-0.5*t5i; \
			    t7r = z[j4]-z[j7]; \
			    t7i = z[j4i]-z[j7i]; \
			    t8r = z[j01]+t5r; \
			    t8i = z[j01i]+t5i; \
			    t9r = z[j2]+z[j5]; \
			    t9i = z[j2i]+z[j5i]; \
			    t10r = z[j8]-0.5*t9r; \
			    t10i = z[j8i]-0.5*t9i; \
			    t11r = z[j2]-z[j5]; \
			    t11i = z[j2i]-z[j5i]; \
			    t12r = z[j8]+t9r; \
			    t12i = z[j8i]+t9i; \
			    t13r = t8r+t12r; \
			    t13i = t8i+t12i; \
			    t14r = t6r+t10r; \
			    t14i = t6i+t10i; \
			    t15r = t6r-t10r; \
			    t15i = t6i-t10i; \
			    t16r = t7r+t11r; \
			    t16i = t7i+t11i; \
			    t17r = t7r-t11r; \
			    t17i = t7i-t11i; \
			    t18r = c2*t14r-c7*t17r; \
			    t18i = c2*t14i-c7*t17i; \
			    t19r = c4*t14r+c9*t17r; \
			    t19i = c4*t14i+c9*t17i; \
			    t20r = c3*t15r+c6*t16r; \
			    t20i = c3*t15i+c6*t16i; \
			    t21r = c5*t15r-c8*t16r; \
			    t21i = c5*t15i-c8*t16i; \
			    t22r = t18r+t19r; \
			    t22i = t18i+t19i; \
			    t23r = t20r-t21r; \
			    t23i = t20i-t21i; \
			    y1r = t2r+t18r; \
			    y1i = t2i+t18i; \
			    y2r = t2r+t19r; \
			    y2i = t2i+t19i; \
			    y3r = t4r-0.5*t13r; \
			    y3i = t4i-0.5*t13i; \
			    y4r = t2r-t22r; \
			    y4i = t2i-t22i; \
			    y5r = t3r-t23r; \
			    y5i = t3i-t23i; \
			    y6r = c1*(t8r-t12r); \
			    y6i = c1*(t8i-t12i); \
			    y7r = t21r-t3r; \
			    y7i = t21i-t3i; \
			    y8r = t3r+t20r; \
			    y8i = t3i+t20i; \
			    z[j00] = t4r+t13r; \
			    z[j00i] = t4i+t13i; \
			    z[j01] = y1r-y8i; \
			    z[j01i] = y1i+y8r; \
			    z[j2] = y2r-y7i; \
			    z[j2i] = y2i+y7r; \
			    z[j3] = y3r-y6i; \
			    z[j3i] = y3i+y6r; \
			    z[j4] = y4r-y5i; \
			    z[j4i] = y4i+y5r; \
			    z[j5] = y4r+y5i; \
			    z[j5i] = y4i-y5r; \
			    z[j6] = y3r+y6i; \
			    z[j6i] = y3i-y6r; \
			    z[j7] = y2r+y7i; \
			    z[j7i] = y2i-y7r; \
			    z[j8] = y1r+y8i; \
			    z[j8i] = y1i-y8r; \
			    const int jt = j8+(nextsampleinc); \
			    j8 = j7+(nextsampleinc); \
			    j7 = j6+(nextsampleinc); \
			    j6 = j5+(nextsampleinc); \
			    j5 = j4+(nextsampleinc); \
			    j4 = j3+(nextsampleinc); \
			    j3 = j2+(nextsampleinc); \
			    j2 = j01+(nextsampleinc); \
			    j01 = j00+(nextsampleinc); \
			    j00 = jt; \
		    } \
		    continue; \
	    } \
	    j9 = j8+jinc; \
	    if (j9>=jmax) j9 = j9-jmax; \
	    j10 = j9+jinc; \
	    if (j10>=jmax) j10 = j10-jmax; \
 \
	    /* if factor is 11 */ \
	    if (ifac==11) { \
		    if (mu==1) { \
			    c1 = P841; \
			    c2 = P415; \
			    c3 = -P142; \
			    c4 = -P654; \
			    c5 = -P959; \
			    c6 = P540; \
			    c7 = P909; \
			    c8 = P989; \
			    c9 = P755; \
			    c10 = P281; \
		    } else if (mu==2) { \
			    c1 = P415; \
			    c2 = -P654; \
			    c3 = -P959; \
			    c4 = -P142; \
			    c5 = P841; \
			    c6 = P909; \
			    c7 = P755; \
			    c8 = -P281; \
			    c9 = -P989; \
			    c10 = -P540; \
		    } else if (mu==3) { \
			    c1 = -P142; \
			    c2 = -P959; \
			    c3 = P415; \
			    c4 = P841; \
			    c5 = -P654; \
			    c6 = P989; \
			    c7 = -P281; \
			    c8 = -P909; \
			    c9 = P540; \
			    c10 = P755; \
		    } else if (mu==4) { \
			    c1 = -P654; \
			    c2 = -P142; \
			    c3 = P841; \
			    c4 = -P959; \
			    c5 = P415; \
			    c6 = P755; \
			    c7 = -P989; \
			    c8 = P540; \
			    c9 = P281; \
			    c10 = -P909; \
		    } else if (mu==5) { \
			    c1 = -P959; \
			    c2 = P841; \
			    c3 = -P654; \
			    c4 = P415; \
			    c5 = -P142; \
			    c6 = P281; \
			    c7 = -P540; \
			    c8 = P755; \
			    c9 = -P909; \
			    c10 = P989; \
		    } else if (mu==6) { \
			    c1 = -P959; \
			    c2 = P841; \
			    c3 = -P654; \
			    c4 = P415; \
			    c5 = -P142; \
			    c6 = -P281; \
			    c7 = P540; \
			    c8 = -P755; \
			    c9 = P909; \
			    c10 = -P989; \
		    } else if (mu==7) { \
			    c1 = -P654; \
			    c2 = -P142; \
			    c3 = P841; \
			    c4 = -P959; \
			    c5 = P415; \
			    c6 = -P755; \
			    c7 = P989; \
			    c8 = -P540; \
			    c9 = -P281; \
			    c10 = P909; \
		    } else if (mu==8) { \
			    c1 = -P142; \
			    c2 = -P959; \
			    c3 = P415; \
			    c4 = P841; \
			    c5 = -P654; \
			    c6 = -P989; \
			    c7 = P281; \
			    c8 = P909; \
			    c9 = -P540; \
			    c10 = -P755; \
		    } else if (mu==9) { \
			    c1 = P415; \
			    c2 = -P654; \
			    c3 = -P959; \
			    c4 = -P142; \
			    c5 = P841; \
			    c6 = -P909; \
			    c7 = -P755; \
			    c8 = P281; \
			    c9 = P989; \
			    c10 = P540; \
		    } else { \
			    c1 = P841; \
			    c2 = P415; \
			    c3 = -P142; \
			    c4 = -P654; \
			    c5 = -P959; \
			    c6 = -P540; \
			    c7 = -P909; \
			    c8 = -P989; \
			    c9 = -P755; \
			    c10 = -P281; \
		    } \
		    for (l=0; l<m; l++) { \
			    const int j00i = j00+1; \
			    const int j01i = j01+1; \
			    const int j2i = j2+1; \
			    const int j3i = j3+1; \
			    const int j4i = j4+1; \
			    const int j5i = j5+1; \
			    const int j6i = j6+1; \
			    const int j7i = j7+1; \
			    const int j8i = j8+1; \
			    const int j9i = j9+1; \
			    const int j10i = j10+1; \
			    t1r = z[j01]+z[j10]; \
			    t1i = z[j01i]+z[j10i]; \
			    t2r = z[j2]+z[j9]; \
			    t2i = z[j2i]+z[j9i]; \
			    t3r = z[j3]+z[j8]; \
			    t3i = z[j3i]+z[j8i]; \
			    t4r = z[j4]+z[j7]; \
			    t4i = z[j4i]+z[j7i]; \
			    t5r = z[j5]+z[j6]; \
			    t5i = z[j5i]+z[j6i]; \
			    t6r = z[j01]-z[j10]; \
			    t6i = z[j01i]-z[j10i]; \
			    t7r = z[j2]-z[j9]; \
			    t7i = z[j2i]-z[j9i]; \
			    t8r = z[j3]-z[j8]; \
			    t8i = z[j3i]-z[j8i]; \
			    t9r = z[j4]-z[j7]; \
			    t9i = z[j4i]-z[j7i]; \
			    t10r = z[j5]-z[j6]; \
			    t10i = z[j5i]-z[j6i]; \
			    t11r = z[j00]-0.5*t5r; \
			    t11i = z[j00i]-0.5*t5i; \
			    t12r = t1r-t5r; \
			    t12i = t1i-t5i; \
			    t13r = t2r-t5r; \
			    t13i = t2i-t5i; \
			    t14r = t3r-t5r; \
			    t14i = t3i-t5i; \
			    t15r = t4r-t5r; \
			    t15i = t4i-t5i; \
			    y1r = t11r+c1*t12r+c2*t13r+c3*t14r+c4*t15r; \
			    y1i = t11i+c1*t12i+c2*t13i+c3*t14i+c4*t15i; \
			    y2r = t11r+c2*t12r+c4*t13r+c5*t14r+c3*t15r; \
			    y2i = t11i+c2*t12i+c4*t13i+c5*t14i+c3*t15i; \
			    y3r = t11r+c3*t12r+c5*t13r+c2*t14r+c1*t15r; \
			    y3i = t11i+c3*t12i+c5*t13i+c2*t14i+c1*t15i; \
			    y4r = t11r+c4*t12r+c3*t13r+c1*t14r+c5*t15r; \
			    y4i = t11i+c4*t12i+c3*t13i+c1*t14i+c5*t15i; \
			    y5r = t11r+c5*t12r+c1*t13r+c4*t14r+c2*t15r; \
			    y5i = t11i+c5*t12i+c1*t13i+c4*t14i+c2*t15i; \
			    y6r = c10*t6r-c6*t7r+c9*t8r-c7*t9r+c8*t10r; \
			    y6i = c10*t6i-c6*t7i+c9*t8i-c7*t9i+c8*t10i; \
			    y7r = c9*t6r-c8*t7r+c6*t8r+c10*t9r-c7*t10r; \
			    y7i = c9*t6i-c8*t7i+c6*t8i+c10*t9i-c7*t10i; \
			    y8r = c8*t6r-c10*t7r-c7*t8r+c6*t9r+c9*t10r; \
			    y8i = c8*t6i-c10*t7i-c7*t8i+c6*t9i+c9*t10i; \
			    y9r = c7*t6r+c9*t7r-c10*t8r-c8*t9r-c6*t10r; \
			    y9i = c7*t6i+c9*t7i-c10*t8i-c8*t9i-c6*t10i; \
			    y10r = c6*t6r+c7*t7r+c8*t8r+c9*t9r+c10*t10r; \
			    y10i = c6*t6i+c7*t7i+c8*t8i+c9*t9i+c10*t10i; \
			    z[j00] = z[j00]+t1r+t2r+t3r+t4r+t5r; \
			    z[j00i] = z[j00i]+t1i+t2i+t3i+t4i+t5i; \
			    z[j01] = y1r-y10i; \
			    z[j01i] = y1i+y10r; \
			    z[j2] = y2r-y9i; \
			    z[j2i] = y2i+y9r; \
			    z[j3] = y3r-y8i; \
			    z[j3i] = y3i+y8r; \
			    z[j4] = y4r-y7i; \
			    z[j4i] = y4i+y7r; \
			    z[j5] = y5r-y6i; \
			    z[j5i] = y5i+y6r; \
			    z[j6] = y5r+y6i; \
			    z[j6i] = y5i-y6r; \
			    z[j7] = y4r+y7i; \
			    z[j7i] = y4i-y7r; \
			    z[j8] = y3r+y8i; \
			    z[j8i] = y3i-y8r; \
			    z[j9] = y2r+y9i; \
			    z[j9i] = y2i-y9r; \
			    z[j10] = y1r+y10i; \
			    z[j10i] = y1i-y10r; \
			    const int jt = j10+(nextsampleinc); \
			    j10 = j9+(nextsampleinc); \
			    j9 = j8+(nextsampleinc); \
			    j8 = j7+(nextsampleinc); \
			    j7 = j6+(nextsampleinc); \
			    j6 = j5+(nextsampleinc); \
			    j5 = j4+(nextsampleinc); \
			    j4 = j3+(nextsampleinc); \
			    j3 = j2+(nextsampleinc); \
			    j2 = j01+(nextsampleinc); \
			    j01 = j00+(nextsampleinc); \
			    j00 = jt; \
		    } \
		    continue; \
	    } \
	    j11 = j10+jinc; \
	    if (j11>=jmax) j11 = j11-jmax; \
	    j12 = j11+jinc; \
	    if (j12>=jmax) j12 = j12-jmax; \
 \
	    /* if factor is 13 */ \
	    if (ifac==13) { \
		    if (mu==1) { \
			    c1 = P885; \
			    c2 = P568; \
			    c3 = P120; \
			    c4 = -P354; \
			    c5 = -P748; \
			    c6 = -P970; \
			    c7 = P464; \
			    c8 = P822; \
			    c9 = P992; \
			    c10 = P935; \
			    c11 = P663; \
			    c12 = P239; \
		    } else if (mu==2) { \
			    c1 = P568; \
			    c2 = -P354; \
			    c3 = -P970; \
			    c4 = -P748; \
			    c5 = P120; \
			    c6 = P885; \
			    c7 = P822; \
			    c8 = P935; \
			    c9 = P239; \
			    c10 = -P663; \
			    c11 = -P992; \
			    c12 = -P464; \
		    } else if (mu==3) { \
			    c1 = P120; \
			    c2 = -P970; \
			    c3 = -P354; \
			    c4 = P885; \
			    c5 = P568; \
			    c6 = -P748; \
			    c7 = P992; \
			    c8 = P239; \
			    c9 = -P935; \
			    c10 = -P464; \
			    c11 = P822; \
			    c12 = P663; \
		    } else if (mu==4) { \
			    c1 = -P354; \
			    c2 = -P748; \
			    c3 = P885; \
			    c4 = P120; \
			    c5 = -P970; \
			    c6 = P568; \
			    c7 = P935; \
			    c8 = -P663; \
			    c9 = -P464; \
			    c10 = P992; \
			    c11 = -P239; \
			    c12 = -P822; \
		    } else if (mu==5) { \
			    c1 = -P748; \
			    c2 = P120; \
			    c3 = P568; \
			    c4 = -P970; \
			    c5 = P885; \
			    c6 = -P354; \
			    c7 = P663; \
			    c8 = -P992; \
			    c9 = P822; \
			    c10 = -P239; \
			    c11 = -P464; \
			    c12 = P935; \
		    } else if (mu==6) { \
			    c1 = -P970; \
			    c2 = P885; \
			    c3 = -P748; \
			    c4 = P568; \
			    c5 = -P354; \
			    c6 = P120; \
			    c7 = P239; \
			    c8 = -P464; \
			    c9 = P663; \
			    c10 = -P822; \
			    c11 = P935; \
			    c12 = -P992; \
		    } else if (mu==7) { \
			    c1 = -P970; \
			    c2 = P885; \
			    c3 = -P748; \
			    c4 = P568; \
			    c5 = -P354; \
			    c6 = P120; \
			    c7 = -P239; \
			    c8 = P464; \
			    c9 = -P663; \
			    c10 = P822; \
			    c11 = -P935; \
			    c12 = P992; \
		    } else if (mu==8) { \
			    c1 = -P748; \
			    c2 = P120; \
			    c3 = P568; \
			    c4 = -P970; \
			    c5 = P885; \
			    c6 = -P354; \
			    c7 = -P663; \
			    c8 = P992; \
			    c9 = -P822; \
			    c10 = P239; \
			    c11 = P464; \
			    c12 = -P935; \
		    } else if (mu==9) { \
			    c1 = -P354; \
			    c2 = -P748; \
			    c3 = P885; \
			    c4 = P120; \
			    c5 = -P970; \
			    c6 = P568; \
			    c7 = -P935; \
			    c8 = P663; \
			    c9 = P464; \
			    c10 = -P992; \
			    c11 = P239; \
			    c12 = P822; \
		    } else if (mu==10) { \
			    c1 = P120; \
			    c2 = -P970; \
			    c3 = -P354; \
			    c4 = P885; \
			    c5 = P568; \
			    c6 = -P748; \
			    c7 = -P992; \
			    c8 = -P239; \
			    c9 = P935; \
			    c10 = P464; \
			    c11 = -P822; \
			    c12 = -P663; \
		    } else if (mu==11) { \
			    c1 = P568; \
			    c2 = -P354; \
			    c3 = -P970; \
			    c4 = -P748; \
			    c5 = P120; \
			    c6 = P885; \
			    c7 = -P822; \
			    c8 = -P935; \
			    c9 = -P239; \
			    c10 = P663; \
			    c11 = P992; \
			    c12 = P464; \
		    } else { \
			    c1 = P885; \
			    c2 = P568; \
			    c3 = P120; \
			    c4 = -P354; \
			    c5 = -P748; \
			    c6 = -P970; \
			    c7 = -P464; \
			    c8 = -P822; \
			    c9 = -P992; \
			    c10 = -P935; \
			    c11 = -P663; \
			    c12 = -P239; \
		    } \
		    for (l=0; l<m; l++) { \
			    const int j00i = j00+1; \
			    const int j01i = j01+1; \
			    const int j2i = j2+1; \
			    const int j3i = j3+1; \
			    const int j4i = j4+1; \
			    const int j5i = j5+1; \
			    const int j6i = j6+1; \
			    const int j7i = j7+1; \
			    const int j8i = j8+1; \
			    const int j9i = j9+1; \
			    const int j10i = j10+1; \
			    const int j11i = j11+1; \
			    const int j12i = j12+1; \
			    t1r = z[j01]+z[j12]; \
			    t1i = z[j01i]+z[j12i]; \
			    t2r = z[j2]+z[j11]; \
			    t2i = z[j2i]+z[j11i]; \
			    t3r = z[j3]+z[j10]; \
			    t3i = z[j3i]+z[j10i]; \
			    t4r = z[j4]+z[j9]; \
			    t4i = z[j4i]+z[j9i]; \
			    t5r = z[j5]+z[j8]; \
			    t5i = z[j5i]+z[j8i]; \
			    t6r = z[j6]+z[j7]; \
			    t6i = z[j6i]+z[j7i]; \
			    t7r = z[j01]-z[j12]; \
			    t7i = z[j01i]-z[j12i]; \
			    t8r = z[j2]-z[j11]; \
			    t8i = z[j2i]-z[j11i]; \
			    t9r = z[j3]-z[j10]; \
			    t9i = z[j3i]-z[j10i]; \
			    t10r = z[j4]-z[j9]; \
			    t10i = z[j4i]-z[j9i]; \
			    t11r = z[j5]-z[j8]; \
			    t11i = z[j5i]-z[j8i]; \
			    t12r = z[j6]-z[j7]; \
			    t12i = z[j6i]-z[j7i]; \
			    t13r = z[j00]-0.5*t6r; \
			    t13i = z[j00i]-0.5*t6i; \
			    t14r = t1r-t6r; \
			    t14i = t1i-t6i; \
			    t15r = t2r-t6r; \
			    t15i = t2i-t6i; \
			    t16r = t3r-t6r; \
			    t16i = t3i-t6i; \
			    t17r = t4r-t6r; \
			    t17i = t4i-t6i; \
			    t18r = t5r-t6r; \
			    t18i = t5i-t6i; \
			    y1r = t13r+c1*t14r+c2*t15r+c3*t16r+c4*t17r+c5*t18r; \
			    y1i = t13i+c1*t14i+c2*t15i+c3*t16i+c4*t17i+c5*t18i; \
			    y2r = t13r+c2*t14r+c4*t15r+c6*t16r+c5*t17r+c3*t18r; \
			    y2i = t13i+c2*t14i+c4*t15i+c6*t16i+c5*t17i+c3*t18i; \
			    y3r = t13r+c3*t14r+c6*t15r+c4*t16r+c1*t17r+c2*t18r; \
			    y3i = t13i+c3*t14i+c6*t15i+c4*t16i+c1*t17i+c2*t18i; \
			    y4r = t13r+c4*t14r+c5*t15r+c1*t16r+c3*t17r+c6*t18r; \
			    y4i = t13i+c4*t14i+c5*t15i+c1*t16i+c3*t17i+c6*t18i; \
			    y5r = t13r+c5*t14r+c3*t15r+c2*t16r+c6*t17r+c1*t18r; \
			    y5i = t13i+c5*t14i+c3*t15i+c2*t16i+c6*t17i+c1*t18i; \
			    y6r = t13r+c6*t14r+c1*t15r+c5*t16r+c2*t17r+c4*t18r; \
			    y6i = t13i+c6*t14i+c1*t15i+c5*t16i+c2*t17i+c4*t18i; \
			    y7r = c12*t7r-c7*t8r+c11*t9r-c8*t10r+c10*t11r-c9*t12r; \
			    y7i = c12*t7i-c7*t8i+c11*t9i-c8*t10i+c10*t11i-c9*t12i; \
			    y8r = c11*t7r-c9*t8r+c8*t9r-c12*t10r-c7*t11r+c10*t12r; \
			    y8i = c11*t7i-c9*t8i+c8*t9i-c12*t10i-c7*t11i+c10*t12i; \
			    y9r = c10*t7r-c11*t8r-c7*t9r+c9*t10r-c12*t11r-c8*t12r; \
			    y9i = c10*t7i-c11*t8i-c7*t9i+c9*t10i-c12*t11i-c8*t12i; \
			    y10r = c9*t7r+c12*t8r-c10*t9r-c7*t10r+c8*t11r+c11*t12r; \
			    y10i = c9*t7i+c12*t8i-c10*t9i-c7*t10i+c8*t11i+c11*t12i; \
			    y11r = c8*t7r+c10*t8r+c12*t9r-c11*t10r-c9*t11r-c7*t12r; \
			    y11i = c8*t7i+c10*t8i+c12*t9i-c11*t10i-c9*t11i-c7*t12i; \
			    y12r = c7*t7r+c8*t8r+c9*t9r+c10*t10r+c11*t11r+c12*t12r; \
			    y12i = c7*t7i+c8*t8i+c9*t9i+c10*t10i+c11*t11i+c12*t12i; \
			    z[j00] = z[j00]+t1r+t2r+t3r+t4r+t5r+t6r; \
			    z[j00i] = z[j00i]+t1i+t2i+t3i+t4i+t5i+t6i; \
			    z[j01] = y1r-y12i; \
			    z[j01i] = y1i+y12r; \
			    z[j2] = y2r-y11i; \
			    z[j2i] = y2i+y11r; \
			    z[j3] = y3r-y10i; \
			    z[j3i] = y3i+y10r; \
			    z[j4] = y4r-y9i; \
			    z[j4i] = y4i+y9r; \
			    z[j5] = y5r-y8i; \
			    z[j5i] = y5i+y8r; \
			    z[j6] = y6r-y7i; \
			    z[j6i] = y6i+y7r; \
			    z[j7] = y6r+y7i; \
			    z[j7i] = y6i-y7r; \
			    z[j8] = y5r+y8i; \
			    z[j8i] = y5i-y8r; \
			    z[j9] = y4r+y9i; \
			    z[j9i] = y4i-y9r; \
			    z[j10] = y3r+y10i; \
			    z[j10i] = y3i-y10r; \
			    z[j11] = y2r+y11i; \
			    z[j11i] = y2i-y11r; \
			    z[j12] = y1r+y12i; \
			    z[j12i] = y1i-y12r; \
			    const int jt = j12+(nextsampleinc); \
			    j12 = j11+(nextsampleinc); \
			    j11 = j10+(nextsampleinc); \
			    j10 = j9+(nextsampleinc); \
			    j9 = j8+(nextsampleinc); \
			    j8 = j7+(nextsampleinc); \
			    j7 = j6+(nextsampleinc); \
			    j6 = j5+(nextsampleinc); \
			    j5 = j4+(nextsampleinc); \
			    j4 = j3+(nextsampleinc); \
			    j3 = j2+(nextsampleinc); \
			    j2 = j01+(nextsampleinc); \
			    j01 = j00+(nextsampleinc); \
			    j00 = jt; \
		    } \
		    continue; \
	    } \
	    j13 = j12+jinc; \
	    if (j13>=jmax) j13 = j13-jmax; \
	    j14 = j13+jinc; \
	    if (j14>=jmax) j14 = j14-jmax; \
	    j15 = j14+jinc; \
	    if (j15>=jmax) j15 = j15-jmax; \
 \
	    /* if factor is 16 */ \
	    if (ifac==16) { \
		    if (mu==1) { \
			    c1 = 1.0; \
			    c2 = P923; \
			    c3 = P382; \
			    c4 = P707; \
		    } else if (mu==3) { \
			    c1 = -1.0; \
			    c2 = P382; \
			    c3 = P923; \
			    c4 = -P707; \
		    } else if (mu==5) { \
			    c1 = 1.0; \
			    c2 = -P382; \
			    c3 = P923; \
			    c4 = -P707; \
		    } else if (mu==7) { \
			    c1 = -1.0; \
			    c2 = -P923; \
			    c3 = P382; \
			    c4 = P707; \
		    } else if (mu==9) { \
			    c1 = 1.0; \
			    c2 = -P923; \
			    c3 = -P382; \
			    c4 = P707; \
		    } else if (mu==11) { \
			    c1 = -1.0; \
			    c2 = -P382; \
			    c3 = -P923; \
			    c4 = -P707; \
		    } else if (mu==13) { \
			    c1 = 1.0; \
			    c2 = P382; \
			    c3 = -P923; \
			    c4 = -P707; \
		    } else { \
			    c1 = -1.0; \
			    c2 = P923; \
			    c3 = -P382; \
			    c4 = P707; \
		    } \
		    c5 = c1*c4; \
		    c6 = c1*c3; \
		    c7 = c1*c2; \
		    for (l=0; l<m; l++) { \
			    const int j00i = j00+1; \
			    const int j01i = j01+1; \
			    const int j2i = j2+1; \
			    const int j3i = j3+1; \
			    const int j4i = j4+1; \
			    const int j5i = j5+1; \
			    const int j6i = j6+1; \
			    const int j7i = j7+1; \
			    const int j8i = j8+1; \
			    const int j9i = j9+1; \
			    const int j10i = j10+1; \
			    const int j11i = j11+1; \
			    const int j12i = j12+1; \
			    const int j13i = j13+1; \
			    const int j14i = j14+1; \
			    const int j15i = j15+1; \
			    t1r = z[j00]+z[j8]; \
			    t1i = z[j00i]+z[j8i]; \
			    t2r = z[j4]+z[j12]; \
			    t2i = z[j4i]+z[j12i]; \
			    t3r = z[j00]-z[j8]; \
			    t3i = z[j00i]-z[j8i]; \
			    t4r = c1*(z[j4]-z[j12]); \
			    t4i = c1*(z[j4i]-z[j12i]); \
			    t5r = t1r+t2r; \
			    t5i = t1i+t2i; \
			    t6r = t1r-t2r; \
			    t6i = t1i-t2i; \
			    t7r = z[j01]+z[j9]; \
			    t7i = z[j01i]+z[j9i]; \
			    t8r = z[j5]+z[j13]; \
			    t8i = z[j5i]+z[j13i]; \
			    t9r = z[j01]-z[j9]; \
			    t9i = z[j01i]-z[j9i]; \
			    t10r = z[j5]-z[j13]; \
			    t10i = z[j5i]-z[j13i]; \
			    t11r = t7r+t8r; \
			    t11i = t7i+t8i; \
			    t12r = t7r-t8r; \
			    t12i = t7i-t8i; \
			    t13r = z[j2]+z[j10]; \
			    t13i = z[j2i]+z[j10i]; \
			    t14r = z[j6]+z[j14]; \
			    t14i = z[j6i]+z[j14i]; \
			    t15r = z[j2]-z[j10]; \
			    t15i = z[j2i]-z[j10i]; \
			    t16r = z[j6]-z[j14]; \
			    t16i = z[j6i]-z[j14i]; \
			    t17r = t13r+t14r; \
			    t17i = t13i+t14i; \
			    t18r = c4*(t15r-t16r); \
			    t18i = c4*(t15i-t16i); \
			    t19r = c5*(t15r+t16r); \
			    t19i = c5*(t15i+t16i); \
			    t20r = c1*(t13r-t14r); \
			    t20i = c1*(t13i-t14i); \
			    t21r = z[j3]+z[j11]; \
			    t21i = z[j3i]+z[j11i]; \
			    t22r = z[j7]+z[j15]; \
			    t22i = z[j7i]+z[j15i]; \
			    t23r = z[j3]-z[j11]; \
			    t23i = z[j3i]-z[j11i]; \
			    t24r = z[j7]-z[j15]; \
			    t24i = z[j7i]-z[j15i]; \
			    t25r = t21r+t22r; \
			    t25i = t21i+t22i; \
			    t26r = t21r-t22r; \
			    t26i = t21i-t22i; \
			    t27r = t9r+t24r; \
			    t27i = t9i+t24i; \
			    t28r = t10r+t23r; \
			    t28i = t10i+t23i; \
			    t29r = t9r-t24r; \
			    t29i = t9i-t24i; \
			    t30r = t10r-t23r; \
			    t30i = t10i-t23i; \
			    t31r = t5r+t17r; \
			    t31i = t5i+t17i; \
			    t32r = t11r+t25r; \
			    t32i = t11i+t25i; \
			    t33r = t3r+t18r; \
			    t33i = t3i+t18i; \
			    t34r = c2*t29r-c6*t30r; \
			    t34i = c2*t29i-c6*t30i; \
			    t35r = t3r-t18r; \
			    t35i = t3i-t18i; \
			    t36r = c7*t27r-c3*t28r; \
			    t36i = c7*t27i-c3*t28i; \
			    t37r = t4r+t19r; \
			    t37i = t4i+t19i; \
			    t38r = c3*t27r+c7*t28r; \
			    t38i = c3*t27i+c7*t28i; \
			    t39r = t4r-t19r; \
			    t39i = t4i-t19i; \
			    t40r = c6*t29r+c2*t30r; \
			    t40i = c6*t29i+c2*t30i; \
			    t41r = c4*(t12r-t26r); \
			    t41i = c4*(t12i-t26i); \
			    t42r = c5*(t12r+t26r); \
			    t42i = c5*(t12i+t26i); \
			    y1r = t33r+t34r; \
			    y1i = t33i+t34i; \
			    y2r = t6r+t41r; \
			    y2i = t6i+t41i; \
			    y3r = t35r+t40r; \
			    y3i = t35i+t40i; \
			    y4r = t5r-t17r; \
			    y4i = t5i-t17i; \
			    y5r = t35r-t40r; \
			    y5i = t35i-t40i; \
			    y6r = t6r-t41r; \
			    y6i = t6i-t41i; \
			    y7r = t33r-t34r; \
			    y7i = t33i-t34i; \
			    y9r = t38r-t37r; \
			    y9i = t38i-t37i; \
			    y10r = t42r-t20r; \
			    y10i = t42i-t20i; \
			    y11r = t36r+t39r; \
			    y11i = t36i+t39i; \
			    y12r = c1*(t11r-t25r); \
			    y12i = c1*(t11i-t25i); \
			    y13r = t36r-t39r; \
			    y13i = t36i-t39i; \
			    y14r = t42r+t20r; \
			    y14i = t42i+t20i; \
			    y15r = t38r+t37r; \
			    y15i = t38i+t37i; \
			    z[j00] = t31r+t32r; \
			    z[j00i] = t31i+t32i; \
			    z[j01] = y1r-y15i; \
			    z[j01i] = y1i+y15r; \
			    z[j2] = y2r-y14i; \
			    z[j2i] = y2i+y14r; \
			    z[j3] = y3r-y13i; \
			    z[j3i] = y3i+y13r; \
			    z[j4] = y4r-y12i; \
			    z[j4i] = y4i+y12r; \
			    z[j5] = y5r-y11i; \
			    z[j5i] = y5i+y11r; \
			    z[j6] = y6r-y10i; \
			    z[j6i] = y6i+y10r; \
			    z[j7] = y7r-y9i; \
			    z[j7i] = y7i+y9r; \
			    z[j8] = t31r-t32r; \
			    z[j8i] = t31i-t32i; \
			    z[j9] = y7r+y9i; \
			    z[j9i] = y7i-y9r; \
			    z[j10] = y6r+y10i; \
			    z[j10i] = y6i-y10r; \
			    z[j11] = y5r+y11i; \
			    z[j11i] = y5i-y11r; \
			    z[j12] = y4r+y12i; \
			    z[j12i] = y4i-y12r; \
			    z[j13] = y3r+y13i; \
			    z[j13i] = y3i-y13r; \
			    z[j14] = y2r+y14i; \
			    z[j14i] = y2i-y14r; \
			    z[j15] = y1r+y15i; \
			    z[j15i] = y1i-y15r; \
			    const int jt = j15+(nextsampleinc); \
			    j15 = j14+(nextsampleinc); \
			    j14 = j13+(nextsampleinc); \
			    j13 = j12+(nextsampleinc); \
			    j12 = j11+(nextsampleinc); \
			    j11 = j10+(nextsampleinc); \
			    j10 = j9+(nextsampleinc); \
			    j9 = j8+(nextsampleinc); \
			    j8 = j7+(nextsampleinc); \
			    j7 = j6+(nextsampleinc); \
			    j6 = j5+(nextsampleinc); \
			    j5 = j4+(nextsampleinc); \
			    j4 = j3+(nextsampleinc); \
			    j3 = j2+(nextsampleinc); \
			    j2 = j01+(nextsampleinc); \
			    j01 = j00+(nextsampleinc); \
			    j00 = jt; \
		    } \
		    continue; \
	    } \
    }



#define mPFACCND( istep, loopinit, loopstart, loopend ) \
    register float *z=(float*)cz; \
    register int j0,j1,j2,j3,j4,j5,j6,j7,j8,j9,j10,j11,j12,j13,j14,j15; \
    int nleft,jfax,ifac,jfac,iinc,imax,ndiv,m,mm=0,mu=0,l, \
            jt,i0,i1,i2,i3,i4,i5,i6,i7,i8,i9,i10,i11,i12,i13,i14,i15; \
    float t1r,t1i,t2r,t2i,t3r,t3i,t4r,t4i,t5r,t5i, \
              t6r,t6i,t7r,t7i,t8r,t8i,t9r,t9i,t10r,t10i, \
            t11r,t11i,t12r,t12i,t13r,t13i,t14r,t14i,t15r,t15i, \
        t16r,t16i,t17r,t17i,t18r,t18i,t19r,t19i,t20r,t20i, \
        t21r,t21i,t22r,t22i,t23r,t23i,t24r,t24i,t25r,t25i, \
        t26r,t26i,t27r,t27i,t28r,t28i,t29r,t29i,t30r,t30i, \
        t31r,t31i,t32r,t32i,t33r,t33i,t34r,t34i,t35r,t35i, \
        t36r,t36i,t37r,t37i,t38r,t38i,t39r,t39i,t40r,t40i, \
        t41r,t41i,t42r,t42i, \
        y1r,y1i,y2r,y2i,y3r,y3i,y4r,y4i,y5r,y5i, \
        y6r,y6i,y7r,y7i,y8r,y8i,y9r,y9i,y10r,y10i, \
        y11r,y11i,y12r,y12i,y13r,y13i,y14r,y14i,y15r,y15i, \
        c1,c2,c3,c4,c5,c6,c7,c8,c9,c10,c11,c12; \
	\
    nleft = n; \
    for (jfax=0; jfax<NFAX; jfax++) { \
	\
	/* skip if not a mutually prime factor of n */ \
	ifac = kfax[jfax]; \
	ndiv = nleft/ifac; \
	if (ndiv*ifac!=nleft) continue; \
	\
	/* update n left and determine n divided by factor */ \
	nleft = ndiv; \
	m = n/ifac; \
	\
	/* determine rotation factor mu and stride mm */ \
	for (jfac=1; jfac<=ifac; jfac++) { \
	    mu = jfac; \
	    mm = jfac*m; \
	    if (mm%ifac==1) break; \
	} \
	\
	/* adjust rotation factor for sign of transform */ \
	if (isign<0) mu = ifac-mu; \
	\
	/* compute stride, limit, and pointers */ \
	iinc = istep*mm; \
	imax = istep*n; \
	i0 = 0; \
	i1 = i0+iinc; \
	\
	if (ifac==2) { \
	    for (l=0; l<m; l++) { \
		loopinit; \
		for (jt=0; jt<nt; jt++) { \
		    loopstart; \
		    j0 = i0 + offset; \
		    j1 = i1 + offset; \
		    const int j0i = j0+1; \
		    const int j1i = j1+1; \
		    t1r = z[j0]-z[j1]; \
		    t1i = z[j0i]-z[j1i]; \
		    z[j0] = z[j0]+z[j1]; \
		    z[j0i] = z[j0i]+z[j1i]; \
		    z[j1] = t1r; \
		    z[j1i] = t1i; \
		    loopend; \
		} \
		const int it = i1+istep; \
		i1 = i0+istep; \
		i0 = it; \
	    } \
	    continue; \
	} \
	i2 = i1+iinc; \
	if (i2>=imax) i2 = i2-imax; \
	 \
	if (ifac==3) { \
	    if (mu==1) \
		c1 = P866; \
	    else \
		c1 = -P866; \
	    for (l=0; l<m; l++) { \
		loopinit; \
		for (jt=0; jt<nt; jt++) { \
		    loopstart; \
		    j0 = i0+offset; \
		    j1 = i1+offset; \
		    j2 = i2+offset; \
		    const int j0i = j0+1; \
		    const int j1i = j1+1; \
		    const int j2i = j2+1; \
		    t1r = z[j1]+z[j2]; \
		    t1i = z[j1i]+z[j2i]; \
		    y1r = z[j0]-0.5*t1r; \
		    y1i = z[j0i]-0.5*t1i; \
		    y2r = c1*(z[j1]-z[j2]); \
		    y2i = c1*(z[j1i]-z[j2i]); \
		    z[j0] = z[j0]+t1r; \
		    z[j0i] = z[j0i]+t1i; \
		    z[j1] = y1r-y2i; \
		    z[j1i] = y1i+y2r; \
		    z[j2] = y1r+y2i; \
		    z[j2i] = y1i-y2r; \
		    loopend; \
		} \
		const int it = i2+istep; \
		i2 = i1+istep; \
		i1 = i0+istep; \
		i0 = it; \
	    } \
            continue; \
        } \
        i3 = i2+iinc; \
        if (i3>=imax) i3 = i3-imax; \
	 \
        if (ifac==4) { \
                if (mu==1) \
	                c1 = 1.0; \
                else \
                    c1 = -1.0; \
            for (l=0; l<m; l++) { \
		loopinit; \
                for (jt=0; jt<nt; jt++) { \
		    loopstart; \
		    j0 = i0 + offset; \
		    j1 = i1 + offset; \
		    j2 = i2 + offset; \
		    j3 = i3 + offset; \
		    const int j0i = j0+1; \
		    const int j1i = j1+1; \
		    const int j2i = j2+1; \
		    const int j3i = j3+1; \
		    t1r = z[j0]+z[j2]; \
		    t1i = z[j0i]+z[j2i]; \
                    t2r = z[j1]+z[j3]; \
                    t2i = z[j1i]+z[j3i]; \
                    y1r = z[j0]-z[j2]; \
                    y1i = z[j0i]-z[j2i]; \
                    y3r = c1*(z[j1]-z[j3]); \
                    y3i = c1*(z[j1i]-z[j3i]); \
                    z[j0] = t1r+t2r; \
                    z[j0i] = t1i+t2i; \
                    z[j1] = y1r-y3i; \
                    z[j1i] = y1i+y3r; \
                    z[j2] = t1r-t2r; \
                    z[j2i] = t1i-t2i; \
                    z[j3] = y1r+y3i; \
                    z[j3i] = y1i-y3r; \
		    loopend; \
                } \
                const int it = i3+istep; \
                i3 = i2+istep; \
                i2 = i1+istep; \
                i1 = i0+istep; \
                i0 = it; \
            } \
            continue; \
        } \
        i4 = i3+iinc; \
        if (i4>=imax) i4 = i4-imax; \
	 \
        if (ifac==5) { \
                if (mu==1) { \
	                c1 = P559; \
	                    c2 = P951; \
	                    c3 = P587; \
	            } else if (mu==2) { \
                    c1 = -P559; \
                c2 = P587; \
                c3 = -P951; \
            } else if (mu==3) { \
                c1 = -P559; \
                c2 = -P587; \
                c3 = P951; \
            } else { \
                c1 = P559; \
                c2 = -P951; \
                c3 = -P587; \
            } \
            for (l=0; l<m; l++) { \
		loopinit; \
                for (jt=0; jt<nt; jt++) { \
		    loopstart; \
		    j0 = i0 + offset; \
		    j1 = i1 + offset; \
		    j2 = i2 + offset; \
		    j3 = i3 + offset; \
		    j4 = i4 + offset; \
		    const int j0i = j0+1; \
		    const int j1i = j1+1; \
		    const int j2i = j2+1; \
		    const int j3i = j3+1; \
		    const int j4i = j4+1; \
		    t1r = z[j1]+z[j4]; \
		    t1i = z[j1i]+z[j4i]; \
                    t2r = z[j2]+z[j3]; \
                    t2i = z[j2i]+z[j3i]; \
                    t3r = z[j1]-z[j4]; \
                    t3i = z[j1i]-z[j4i]; \
                    t4r = z[j2]-z[j3]; \
                    t4i = z[j2i]-z[j3i]; \
                    t5r = t1r+t2r; \
                    t5i = t1i+t2i; \
                    t6r = c1*(t1r-t2r); \
                    t6i = c1*(t1i-t2i); \
                    t7r = z[j0]-0.25*t5r; \
                    t7i = z[j0i]-0.25*t5i; \
                    y1r = t7r+t6r; \
                    y1i = t7i+t6i; \
                    y2r = t7r-t6r; \
                    y2i = t7i-t6i; \
                    y3r = c3*t3r-c2*t4r; \
                    y3i = c3*t3i-c2*t4i; \
                    y4r = c2*t3r+c3*t4r; \
                    y4i = c2*t3i+c3*t4i; \
                    z[j0] = z[j0]+t5r; \
                    z[j0i] = z[j0i]+t5i; \
                    z[j1] = y1r-y4i; \
                    z[j1i] = y1i+y4r; \
                    z[j2] = y2r-y3i; \
                    z[j2i] = y2i+y3r; \
                    z[j3] = y2r+y3i; \
                    z[j3i] = y2i-y3r; \
                    z[j4] = y1r+y4i; \
                    z[j4i] = y1i-y4r; \
		    loopend; \
                } \
                const int it = i4+istep; \
                i4 = i3+istep; \
                i3 = i2+istep; \
                i2 = i1+istep; \
                i1 = i0+istep; \
                i0 = it; \
            } \
            continue; \
        } \
        i5 = i4+iinc; \
        if (i5>=imax) i5 = i5-imax; \
        i6 = i5+iinc; \
        if (i6>=imax) i6 = i6-imax; \
	 \
        if (ifac==7) { \
                if (mu==1) { \
	                c1 = P623; \
	                    c2 = -P222; \
	                    c3 = -P900; \
	                c4 = P781; \
                    c5 = P974; \
                c6 = P433; \
            } else if (mu==2) { \
                c1 = -P222; \
                c2 = -P900; \
                c3 = P623; \
                c4 = P974; \
                c5 = -P433; \
                c6 = -P781; \
            } else if (mu==3) { \
                c1 = -P900; \
                c2 = P623; \
                c3 = -P222; \
                c4 = P433; \
                c5 = -P781; \
                c6 = P974; \
            } else if (mu==4) { \
                c1 = -P900; \
                c2 = P623; \
                c3 = -P222; \
                c4 = -P433; \
                c5 = P781; \
                c6 = -P974; \
            } else if (mu==5) { \
                c1 = -P222; \
                c2 = -P900; \
                c3 = P623; \
                c4 = -P974; \
                c5 = P433; \
                c6 = P781; \
            } else { \
                c1 = P623; \
                c2 = -P222; \
                c3 = -P900; \
                c4 = -P781; \
                c5 = -P974; \
                c6 = -P433; \
            } \
            for (l=0; l<m; l++) { \
		loopinit; \
                for (jt=0; jt<nt; jt++) { \
		    loopstart; \
		    j0 = i0 + offset; \
		    j1 = i1 + offset; \
		    j2 = i2 + offset; \
		    j3 = i3 + offset; \
		    j4 = i4 + offset; \
		    j5 = i5 + offset; \
		    j6 = i6 + offset; \
		    const int j0i = j0+1; \
		    const int j1i = j1+1; \
		    const int j2i = j2+1; \
		    const int j3i = j3+1; \
		    const int j4i = j4+1; \
		    const int j5i = j5+1; \
		    const int j6i = j6+1; \
                    t1r = z[j1]+z[j6]; \
                    t1i = z[j1i]+z[j6i]; \
                    t2r = z[j2]+z[j5]; \
                    t2i = z[j2i]+z[j5i]; \
                    t3r = z[j3]+z[j4]; \
                    t3i = z[j3i]+z[j4i]; \
                    t4r = z[j1]-z[j6]; \
                    t4i = z[j1i]-z[j6i]; \
                    t5r = z[j2]-z[j5]; \
                    t5i = z[j2i]-z[j5i]; \
                    t6r = z[j3]-z[j4]; \
                    t6i = z[j3i]-z[j4i]; \
                    t7r = z[j0]-0.5*t3r; \
                    t7i = z[j0i]-0.5*t3i; \
                    t8r = t1r-t3r; \
                    t8i = t1i-t3i; \
                    t9r = t2r-t3r; \
                    t9i = t2i-t3i; \
                    y1r = t7r+c1*t8r+c2*t9r; \
                    y1i = t7i+c1*t8i+c2*t9i; \
                    y2r = t7r+c2*t8r+c3*t9r; \
                    y2i = t7i+c2*t8i+c3*t9i; \
                    y3r = t7r+c3*t8r+c1*t9r; \
                    y3i = t7i+c3*t8i+c1*t9i; \
                    y4r = c6*t4r-c4*t5r+c5*t6r; \
                    y4i = c6*t4i-c4*t5i+c5*t6i; \
                    y5r = c5*t4r-c6*t5r-c4*t6r; \
                    y5i = c5*t4i-c6*t5i-c4*t6i; \
                    y6r = c4*t4r+c5*t5r+c6*t6r; \
                    y6i = c4*t4i+c5*t5i+c6*t6i; \
                    z[j0] = z[j0]+t1r+t2r+t3r; \
                    z[j0i] = z[j0i]+t1i+t2i+t3i; \
                    z[j1] = y1r-y6i; \
                    z[j1i] = y1i+y6r; \
                    z[j2] = y2r-y5i; \
                    z[j2i] = y2i+y5r; \
                    z[j3] = y3r-y4i; \
                    z[j3i] = y3i+y4r; \
                    z[j4] = y3r+y4i; \
                    z[j4i] = y3i-y4r; \
                    z[j5] = y2r+y5i; \
                    z[j5i] = y2i-y5r; \
                    z[j6] = y1r+y6i; \
                    z[j6i] = y1i-y6r; \
		    loopend; \
                } \
		 \
                const int it = i6+istep; \
                i6 = i5+istep; \
                i5 = i4+istep; \
                i4 = i3+istep; \
                i3 = i2+istep; \
                i2 = i1+istep; \
                i1 = i0+istep; \
                i0 = it; \
            } \
            continue; \
        } \
        i7 = i6+iinc; \
        if (i7>=imax) i7 = i7-imax; \
	 \
        /* if factor is 8 */  \
        if (ifac==8) { \
                if (mu==1) { \
	                c1 = 1.0; \
	                    c2 = P707; \
	                } else if (mu==3) { \
	                c1 = -1.0;  \
                    c2 = -P707; \
            } else if (mu==5) { \
                c1 = 1.0; \
                c2 = -P707; \
            } else { \
                c1 = -1.0; \
                c2 = P707; \
            } \
            c3 = c1*c2; \
            for (l=0; l<m; l++) { \
		loopinit; \
                for (jt=0; jt<nt; jt++) { \
		    loopstart; \
                    j0 = i0 + offset; \
                    j1 = i1 + offset; \
		    j2 = i2 + offset; \
		    j3 = i3 + offset; \
		    j4 = i4 + offset; \
		    j5 = i5 + offset; \
		    j6 = i6 + offset; \
		    j7 = i7 + offset; \
		    const int j0i = j0+1; \
		    const int j1i = j1+1; \
		    const int j2i = j2+1; \
		    const int j3i = j3+1; \
		    const int j4i = j4+1; \
		    const int j5i = j5+1; \
		    const int j6i = j6+1; \
		    const int j7i = j7+1; \
                    t1r = z[j0]+z[j4]; \
                    t1i = z[j0i]+z[j4i]; \
                    t2r = z[j0]-z[j4]; \
                    t2i = z[j0i]-z[j4i]; \
                    t3r = z[j1]+z[j5]; \
                    t3i = z[j1i]+z[j5i]; \
                    t4r = z[j1]-z[j5]; \
                    t4i = z[j1i]-z[j5i]; \
                    t5r = z[j2]+z[j6];  \
                    t5i = z[j2i]+z[j6i]; \
                    t6r = c1*(z[j2]-z[j6]); \
                    t6i = c1*(z[j2i]-z[j6i]); \
                    t7r = z[j3]+z[j7];  \
                    t7i = z[j3i]+z[j7i];\
                    t8r = z[j3]-z[j7]; \
                    t8i = z[j3i]-z[j7i]; \
                    t9r = t1r+t5r; \
                    t9i = t1i+t5i; \
                    t10r = t3r+t7r; \
                    t10i = t3i+t7i; \
                    t11r = c2*(t4r-t8r); \
                    t11i = c2*(t4i-t8i); \
                    t12r = c3*(t4r+t8r); \
                    t12i = c3*(t4i+t8i); \
                    y1r = t2r+t11r; \
                    y1i = t2i+t11i; \
                    y2r = t1r-t5r; \
                    y2i = t1i-t5i;  \
                    y3r = t2r-t11r; \
                    y3i = t2i-t11i; \
                    y5r = t12r-t6r; \
                    y5i = t12i-t6i;  \
                    y6r = c1*(t3r-t7r); \
                    y6i = c1*(t3i-t7i);  \
                    y7r = t12r+t6r; \
                    y7i = t12i+t6i; \
                    z[j0] = t9r+t10r; \
                    z[j0i] = t9i+t10i; \
                    z[j1] = y1r-y7i;  \
                    z[j1i] = y1i+y7r; \
                    z[j2] = y2r-y6i; \
                    z[j2i] = y2i+y6r; \
                    z[j3] = y3r-y5i;  \
                    z[j3i] = y3i+y5r; \
                    z[j4] = t9r-t10r; \
                    z[j4i] = t9i-t10i; \
                    z[j5] = y3r+y5i; \
                    z[j5i] = y3i-y5r; \
                    z[j6] = y2r+y6i;  \
                    z[j6i] = y2i-y6r; \
                    z[j7] = y1r+y7i; \
                    z[j7i] = y1i-y7r; \
		    loopend; \
                }  \
                const int it = i7+istep; \
                i7 = i6+istep; \
                i6 = i5+istep;  \
                i5 = i4+istep; \
                i4 = i3+istep; \
                i3 = i2+istep;  \
                i2 = i1+istep; \
                i1 = i0+istep; \
                i0 = it;  \
            } \
            continue; \
        } \
        i8 = i7+iinc;  \
        if (i8>=imax) i8 = i8-imax; \
	 \
        /* if factor is 9 */ \
        if (ifac==9) { \
                if (mu==1) {  \
	                c1 = P866; \
	                    c2 = P766; \
	                    c3 = P642; \
	                c4 = P173;  \
                    c5 = P984; \
            } else if (mu==2) { \
                c1 = -P866; \
                c2 = P173; \
                c3 = P984;  \
                c4 = -P939; \
                c5 = P342; \
            } else if (mu==4) {  \
                c1 = P866; \
                c2 = -P939; \
                c3 = P342; \
                c4 = P766; \
                c5 = -P642;  \
            } else if (mu==5) { \
                c1 = -P866; \
                c2 = -P939; \
                c3 = -P342;  \
                c4 = P766; \
                c5 = P642; \
            } else if (mu==7) { \
                c1 = P866; \
                c2 = P173; \
                c3 = -P984;  \
                c4 = -P939; \
                c5 = -P342; \
            } else { \
                c1 = -P866; \
                c2 = P766; \
                c3 = -P642; \
                c4 = P173; \
                c5 = -P984; \
            } \
            c6 = c1*c2; \
            c7 = c1*c3; \
            c8 = c1*c4;  \
            c9 = c1*c5; \
            for (l=0; l<m; l++) { \
		loopinit; \
                for (jt=0; jt<nt; jt++) { \
		    loopstart; \
                    j0 = i0 + offset; \
                    j1 = i1 + offset; \
		    j2 = i2 + offset; \
		    j3 = i3 + offset;  \
		    j4 = i4 + offset; \
		    j5 = i5 + offset; \
		    j6 = i6 + offset; \
		    j7 = i7 + offset; \
		    j8 = i8 + offset; \
		    const int j0i = j0+1; \
		    const int j1i = j1+1; \
		    const int j2i = j2+1; \
		    const int j3i = j3+1;  \
		    const int j4i = j4+1; \
		    const int j5i = j5+1; \
		    const int j6i = j6+1;  \
		    const int j7i = j7+1; \
		    const int j8i = j8+1; \
		    t1r = z[j3]+z[j6]; \
		    t1i = z[j3i]+z[j6i]; \
                    t2r = z[j0]-0.5*t1r; \
                    t2i = z[j0i]-0.5*t1i; \
                    t3r = c1*(z[j3]-z[j6]); \
                    t3i = c1*(z[j3i]-z[j6i]);  \
                    t4r = z[j0]+t1r; \
                    t4i = z[j0i]+t1i; \
                    t5r = z[j4]+z[j7]; \
                    t5i = z[j4i]+z[j7i]; \
                    t6r = z[j1]-0.5*t5r;  \
                    t6i = z[j1i]-0.5*t5i; \
                    t7r = z[j4]-z[j7]; \
                    t7i = z[j4i]-z[j7i]; \
                    t8r = z[j1]+t5r;  \
                    t8i = z[j1i]+t5i; \
                    t9r = z[j2]+z[j5]; \
                    t9i = z[j2i]+z[j5i];  \
                    t10r = z[j8]-0.5*t9r; \
                    t10i = z[j8i]-0.5*t9i; \
                    t11r = z[j2]-z[j5]; \
                    t11i = z[j2i]-z[j5i]; \
                    t12r = z[j8]+t9r; \
                    t12i = z[j8i]+t9i;  \
                    t13r = t8r+t12r; \
                    t13i = t8i+t12i; \
                    t14r = t6r+t10r; \
                    t14i = t6i+t10i; \
                    t15r = t6r-t10r; \
                    t15i = t6i-t10i;  \
                    t16r = t7r+t11r; \
                    t16i = t7i+t11i; \
                    t17r = t7r-t11r; \
                    t17i = t7i-t11i; \
                    t18r = c2*t14r-c7*t17r; \
                    t18i = c2*t14i-c7*t17i;  \
                    t19r = c4*t14r+c9*t17r; \
                    t19i = c4*t14i+c9*t17i; \
                    t20r = c3*t15r+c6*t16r; \
                    t20i = c3*t15i+c6*t16i; \
                    t21r = c5*t15r-c8*t16r; \
                    t21i = c5*t15i-c8*t16i; \
                    t22r = t18r+t19r;  \
                    t22i = t18i+t19i; \
                    t23r = t20r-t21r; \
                    t23i = t20i-t21i; \
                    y1r = t2r+t18r; \
                    y1i = t2i+t18i;  \
                    y2r = t2r+t19r; \
                    y2i = t2i+t19i; \
                    y3r = t4r-0.5*t13r; \
                    y3i = t4i-0.5*t13i; \
                    y4r = t2r-t22r; \
                    y4i = t2i-t22i; \
                    y5r = t3r-t23r; \
                    y5i = t3i-t23i;  \
                    y6r = c1*(t8r-t12r); \
                    y6i = c1*(t8i-t12i); \
                    y7r = t21r-t3r; \
                    y7i = t21i-t3i; \
                    y8r = t3r+t20r; \
                    y8i = t3i+t20i;  \
                    z[j0] = t4r+t13r; \
                    z[j0i] = t4i+t13i;  \
                    z[j1] = y1r-y8i; \
                    z[j1i] = y1i+y8r; \
                    z[j2] = y2r-y7i;  \
                    z[j2i] = y2i+y7r; \
                    z[j3] = y3r-y6i;  \
                    z[j3i] = y3i+y6r;  \
                    z[j4] = y4r-y5i; \
                    z[j4i] = y4i+y5r;  \
                    z[j5] = y4r+y5i; \
                    z[j5i] = y4i-y5r; \
                    z[j6] = y3r+y6i; \
                    z[j6i] = y3i-y6r;  \
                    z[j7] = y2r+y7i; \
                    z[j7i] = y2i-y7r; \
                    z[j8] = y1r+y8i; \
                    z[j8i] = y1i-y8r; \
		    loopend; \
                } \
                const int it = i8+istep;  \
                i8 = i7+istep; \
                i7 = i6+istep; \
                i6 = i5+istep; \
                i5 = i4+istep; \
                i4 = i3+istep;  \
                i3 = i2+istep; \
                i2 = i1+istep; \
                i1 = i0+istep; \
                i0 = it; \
            } \
            continue;  \
        } \
        i9 = i8+iinc; \
        if (i9>=imax) i9 = i9-imax; \
        i10 = i9+iinc; \
        if (i10>=imax) i10 = i10-imax;  \
	 \
        /* if factor is 11 */  \
        if (ifac==11) {  \
                if (mu==1) {  \
	                c1 = P841; \
	                    c2 = P415; \
	                    c3 = -P142;  \
	                c4 = -P654; \
                    c5 = -P959; \
                c6 = P540; \
                c7 = P909;  \
                c8 = P989; \
                c9 = P755; \
                c10 = P281;  \
            } else if (mu==2) { \
                c1 = P415; \
                c2 = -P654;  \
                c3 = -P959; \
                c4 = -P142;  \
                c5 = P841; \
                c6 = P909; \
               c7 = P755;  \
                c8 = -P281; \
                 c9 = -P989; \
                 c10 = -P540; \
            } else if (mu==3) {  \
                c1 = -P142; \
                c2 = -P959; \
                c3 = P415; \
                c4 = P841;  \
                c5 = -P654; \
                c6 = P989; \
                c7 = -P281; \
                c8 = -P909; \
                c9 = P540; \
                c10 = P755;  \
            } else if (mu==4) { \
                c1 = -P654; \
                c2 = -P142;  \
                c3 = P841; \
                c4 = -P959; \
                c5 = P415;  \
                c6 = P755; \
                c7 = -P989; \
                c8 = P540;  \
                c9 = P281; \
                c10 = -P909; \
            } else if (mu==5) { \
                c1 = -P959; \
                c2 = P841;  \
                c3 = -P654; \
                c4 = P415; \
                c5 = -P142; \
                c6 = P281; \
                c7 = -P540;  \
                c8 = P755; \
                c9 = -P909; \
                c10 = P989; \
            } else if (mu==6) {  \
                c1 = -P959; \
                c2 = P841; \
                c3 = -P654; \
                c4 = P415; \
                c5 = -P142;  \
                c6 = -P281; \
                c7 = P540; \
                c8 = -P755;  \
                c9 = P909; \
                c10 = -P989; \
            } else if (mu==7) { \
                c1 = -P654; \
                c2 = -P142;  \
                c3 = P841; \
                c4 = -P959; \
                c5 = P415; \
                c6 = -P755; \
                c7 = P989;  \
                c8 = -P540; \
                c9 = -P281; \
                c10 = P909; \
            } else if (mu==8) { \
                c1 = -P142;  \
                c2 = -P959; \
                c3 = P415; \
                c4 = P841; \
                c5 = -P654; \
                c6 = -P989; \
                c7 = P281;  \
                c8 = P909; \
                c9 = -P540; \
                c10 = -P755; \
            } else if (mu==9) { \
                c1 = P415; \
                c2 = -P654; \
                c3 = -P959; \
                c4 = -P142; \
                c5 = P841; \
                c6 = -P909; \
                c7 = -P755;  \
                c8 = P281; \
                c9 = P989; \
                c10 = P540; \
            } else { \
                c1 = P841; \
                c2 = P415; \
                c3 = -P142;  \
                c4 = -P654; \
                c5 = -P959; \
                c6 = -P540; \
                c7 = -P909; \
                c8 = -P989; \
                c9 = -P755; \
                c10 = -P281; \
            }  \
            for (l=0; l<m; l++) { \
		loopinit; \
                for (jt=0; jt<nt; jt++) { \
		    loopstart; \
		    j0 = i0 + offset; \
		    j1 = i1 + offset; \
		    j2 = i2 + offset; \
		    j3 = i3 + offset;  \
		    j4 = i4 + offset; \
		    j5 = i5 + offset; \
		    j6 = i6 + offset; \
		    j7 = i7 + offset; \
		    j8 = i8 + offset; \
		    j9 = i9 + offset; \
		    j10 = i10 + offset;  \
		    const int j0i = j0+1; \
		    const int j1i = j1+1; \
		    const int j2i = j2+1; \
		    const int j3i = j3+1; \
		    const int j4i = j4+1;  \
		    const int j5i = j5+1; \
		    const int j6i = j6+1; \
		    const int j7i = j7+1; \
		    const int j8i = j8+1; \
		    const int j9i = j9+1; \
		    const int j10i = j10+1; \
                    t1r = z[j1]+z[j10];  \
                    t1i = z[j1i]+z[j10i]; \
                    t2r = z[j2]+z[j9]; \
                    t2i = z[j2i]+z[j9i]; \
                    t3r = z[j3]+z[j8]; \
                    t3i = z[j3i]+z[j8i];  \
                    t4r = z[j4]+z[j7]; \
                    t4i = z[j4i]+z[j7i]; \
                    t5r = z[j5]+z[j6]; \
                    t5i = z[j5i]+z[j6i];  \
                    t6r = z[j1]-z[j10]; \
                    t6i = z[j1i]-z[j10i]; \
                    t7r = z[j2]-z[j9]; \
                    t7i = z[j2i]-z[j9i];  \
                    t8r = z[j3]-z[j8]; \
                    t8i = z[j3i]-z[j8i]; \
                    t9r = z[j4]-z[j7]; \
                    t9i = z[j4i]-z[j7i]; \
                    t10r = z[j5]-z[j6];  \
                    t10i = z[j5i]-z[j6i]; \
                    t11r = z[j0]-0.5*t5r; \
                    t11i = z[j0i]-0.5*t5i; \
                    t12r = t1r-t5r;  \
                    t12i = t1i-t5i; \
                    t13r = t2r-t5r; \
                    t13i = t2i-t5i; \
                    t14r = t3r-t5r; \
                    t14i = t3i-t5i;  \
                    t15r = t4r-t5r; \
                    t15i = t4i-t5i; \
                    y1r = t11r+c1*t12r+c2*t13r+c3*t14r+c4*t15r; \
                    y1i = t11i+c1*t12i+c2*t13i+c3*t14i+c4*t15i; \
                    y2r = t11r+c2*t12r+c4*t13r+c5*t14r+c3*t15r; \
                    y2i = t11i+c2*t12i+c4*t13i+c5*t14i+c3*t15i;  \
                    y3r = t11r+c3*t12r+c5*t13r+c2*t14r+c1*t15r; \
                    y3i = t11i+c3*t12i+c5*t13i+c2*t14i+c1*t15i; \
                    y4r = t11r+c4*t12r+c3*t13r+c1*t14r+c5*t15r; \
                    y4i = t11i+c4*t12i+c3*t13i+c1*t14i+c5*t15i; \
                    y5r = t11r+c5*t12r+c1*t13r+c4*t14r+c2*t15r; \
                    y5i = t11i+c5*t12i+c1*t13i+c4*t14i+c2*t15i;  \
                    y6r = c10*t6r-c6*t7r+c9*t8r-c7*t9r+c8*t10r; \
                    y6i = c10*t6i-c6*t7i+c9*t8i-c7*t9i+c8*t10i; \
                    y7r = c9*t6r-c8*t7r+c6*t8r+c10*t9r-c7*t10r; \
                    y7i = c9*t6i-c8*t7i+c6*t8i+c10*t9i-c7*t10i; \
                    y8r = c8*t6r-c10*t7r-c7*t8r+c6*t9r+c9*t10r;  \
                    y8i = c8*t6i-c10*t7i-c7*t8i+c6*t9i+c9*t10i; \
                    y9r = c7*t6r+c9*t7r-c10*t8r-c8*t9r-c6*t10r; \
                    y9i = c7*t6i+c9*t7i-c10*t8i-c8*t9i-c6*t10i; \
                    y10r = c6*t6r+c7*t7r+c8*t8r+c9*t9r+c10*t10r; \
                    y10i = c6*t6i+c7*t7i+c8*t8i+c9*t9i+c10*t10i;  \
                    z[j0] = z[j0]+t1r+t2r+t3r+t4r+t5r; \
                    z[j0i] = z[j0i]+t1i+t2i+t3i+t4i+t5i; \
                    z[j1] = y1r-y10i; \
                    z[j1i] = y1i+y10r; \
                    z[j2] = y2r-y9i;  \
                    z[j2i] = y2i+y9r; \
                    z[j3] = y3r-y8i; \
                    z[j3i] = y3i+y8r; \
                    z[j4] = y4r-y7i;  \
                    z[j4i] = y4i+y7r; \
                    z[j5] = y5r-y6i; \
                    z[j5i] = y5i+y6r;  \
                    z[j6] = y5r+y6i; \
                    z[j6i] = y5i-y6r;  \
                    z[j7] = y4r+y7i; \
                    z[j7i] = y4i-y7r; \
                    z[j8] = y3r+y8i;  \
                    z[j8i] = y3i-y8r; \
                    z[j9] = y2r+y9i; \
                    z[j9i] = y2i-y9r; \
                    z[j10] = y1r+y10i; \
                    z[j10i] = y1i-y10r; \
		    loopend; \
                } \
                const int it = i10+istep; \
                i10 = i9+istep; \
                i9 = i8+istep;  \
                i8 = i7+istep; \
                i7 = i6+istep; \
                i6 = i5+istep; \
                i5 = i4+istep; \
                i4 = i3+istep; \
                i3 = i2+istep; \
                i2 = i1+istep; \
                i1 = i0+istep; \
                i0 = it; \
            } \
            continue;  \
        } \
        i11 = i10+iinc; \
        if (i11>=imax) i11 = i11-imax; \
        i12 = i11+iinc; \
        if (i12>=imax) i12 = i12-imax; \
 	\
	/* if factor is 13 */ \
        if (ifac==13) { \
                if (mu==1) { \
	                c1 = P885;  \
	                    c2 = P568; \
	                    c3 = P120; \
	                c4 = -P354; \
                    c5 = -P748; \
                c6 = -P970; \
                c7 = P464;  \
                c8 = P822; \
                c9 = P992; \
                c10 = P935; \
                c11 = P663; \
                c12 = P239; \
            } else if (mu==2) { \
                c1 = P568; \
                c2 = -P354; \
                c3 = -P970;  \
                c4 = -P748; \
                c5 = P120; \
                c6 = P885; \
                c7 = P822; \
                c8 = P935; \
                c9 = P239; \
                c10 = -P663;  \
                c11 = -P992; \
                c12 = -P464; \
            } else if (mu==3) { \
                c1 = P120; \
                c2 = -P970;  \
                c3 = -P354; \
                c4 = P885;  \
                c5 = P568;  \
                c6 = -P748;  \
                c7 = P992;  \
                c8 = P239;   \
                c9 = -P935;  \
                c10 = -P464;  \
                c11 = P822;  \
                c12 = P663;  \
            } else if (mu==4) {   \
                c1 = -P354;  \
                c2 = -P748;  \
                c3 = P885;  \
                c4 = P120;   \
                c5 = -P970;  \
                c6 = P568;  \
                c7 = P935;   \
                c8 = -P663;  \
                c9 = -P464;  \
                c10 = P992;  \
                c11 = -P239;   \
                c12 = -P822;  \
            } else if (mu==5) {  \
                c1 = -P748;  \
                c2 = P120;   \
                c3 = P568;   \
                c4 = -P970;  \
                c5 = P885;  \
                c6 = -P354;  \
                c7 = P663;  \
                c8 = -P992;   \
                c9 = P822; \
                c10 = -P239;  \
                c11 = -P464;  \
                c12 = P935;  \
            } else if (mu==6) {  \
                c1 = -P970;   \
                c2 = P885;  \
                c3 = -P748;  \
                c4 = P568;   \
                c5 = -P354;  \
                c6 = P120;  \
                c7 = P239;  \
                c8 = -P464;  \
                c9 = P663;   \
                c10 = -P822;  \
                c11 = P935;  \
                c12 = -P992;  \
            } else if (mu==7) {    \
                c1 = -P970;   \
                c2 = P885;  \
                c3 = -P748;  \
                c4 = P568;  \
                c5 = -P354;  \
                c6 = P120;  \
                c7 = -P239;  \
                c8 = P464;  \
                c9 = -P663;  \
                c10 = P822;  \
                c11 = -P935;  \
                c12 = P992;  \
            } else if (mu==8) {  \
                c1 = -P748; \
                c2 = P120; \
                c3 = P568; \
                c4 = -P970; \
                c5 = P885; \
                c6 = -P354; \
                c7 = -P663; \
                c8 = P992; \
                c9 = -P822; \
                c10 = P239; \
                c11 = P464; \
                c12 = -P935; \
            } else if (mu==9) { \
                c1 = -P354; \
                c2 = -P748; \
                c3 = P885; \
                c4 = P120; \
                c5 = -P970; \
                c6 = P568; \
                c7 = -P935; \
                c8 = P663; \
                c9 = P464; \
                c10 = -P992; \
                c11 = P239; \
                c12 = P822; \
            } else if (mu==10) { \
                c1 = P120; \
                c2 = -P970; \
                c3 = -P354; \
                c4 = P885; \
                c5 = P568; \
                c6 = -P748; \
                c7 = -P992; \
                c8 = -P239; \
                c9 = P935; \
                c10 = P464; \
                c11 = -P822; \
                c12 = -P663; \
            } else if (mu==11) { \
                c1 = P568; \
                c2 = -P354; \
                c3 = -P970; \
                c4 = -P748; \
                c5 = P120; \
                c6 = P885; \
                c7 = -P822; \
                c8 = -P935; \
                c9 = -P239; \
                c10 = P663; \
                c11 = P992; \
                c12 = P464; \
            } else { \
                c1 = P885; \
                c2 = P568; \
                c3 = P120; \
                c4 = -P354; \
                c5 = -P748; \
                c6 = -P970; \
                c7 = -P464; \
                c8 = -P822; \
                c9 = -P992; \
                c10 = -P935; \
                c11 = -P663; \
                c12 = -P239; \
            } \
            for (l=0; l<m; l++) { \
		loopinit; \
                for (jt=0; jt<nt; jt++) { \
		    loopstart; \
		    j0 = i0 + offset; \
		    j1 = i1 + offset; \
		    j2 = i2 + offset; \
		    j3 = i3 + offset; \
		    j4 = i4 + offset; \
		    j5 = i5 + offset; \
		    j6 = i6 + offset; \
		    j7 = i7 + offset; \
		    j8 = i8 + offset; \
		    j9 = i9 + offset; \
		    j10 = i10 + offset; \
		    j11 = i11 + offset; \
		    j12 = i12 + offset; \
		    const int j0i = j0+1; \
		    const int j1i = j1+1; \
		    const int j2i = j2+1; \
		    const int j3i = j3+1; \
		    const int j4i = j4+1; \
		    const int j5i = j5+1; \
		    const int j6i = j6+1; \
		    const int j7i = j7+1; \
		    const int j8i = j8+1; \
		    const int j9i = j9+1; \
		    const int j10i = j10+1; \
		    const int j11i = j11+1; \
		    const int j12i = j12+1; \
		    t1r = z[j1]+z[j12]; \
		    t1i = z[j1i]+z[j12i]; \
                    t2r = z[j2]+z[j11]; \
                    t2i = z[j2i]+z[j11i]; \
                    t3r = z[j3]+z[j10]; \
                    t3i = z[j3i]+z[j10i]; \
                    t4r = z[j4]+z[j9]; \
                    t4i = z[j4i]+z[j9i]; \
                    t5r = z[j5]+z[j8]; \
                    t5i = z[j5i]+z[j8i]; \
                    t6r = z[j6]+z[j7]; \
                    t6i = z[j6i]+z[j7i]; \
                    t7r = z[j1]-z[j12]; \
                    t7i = z[j1i]-z[j12i]; \
                    t8r = z[j2]-z[j11]; \
                    t8i = z[j2i]-z[j11i]; \
                    t9r = z[j3]-z[j10]; \
                    t9i = z[j3i]-z[j10i]; \
                    t10r = z[j4]-z[j9]; \
                    t10i = z[j4i]-z[j9i]; \
                    t11r = z[j5]-z[j8]; \
                    t11i = z[j5i]-z[j8i]; \
                    t12r = z[j6]-z[j7]; \
                    t12i = z[j6i]-z[j7i]; \
                    t13r = z[j0]-0.5*t6r; \
                    t13i = z[j0i]-0.5*t6i; \
                    t14r = t1r-t6r; \
                    t14i = t1i-t6i; \
                    t15r = t2r-t6r; \
                    t15i = t2i-t6i; \
                    t16r = t3r-t6r; \
                    t16i = t3i-t6i; \
                    t17r = t4r-t6r; \
                    t17i = t4i-t6i; \
                    t18r = t5r-t6r; \
                    t18i = t5i-t6i; \
                    y1r = t13r+c1*t14r+c2*t15r+c3*t16r+c4*t17r+c5*t18r; \
                    y1i = t13i+c1*t14i+c2*t15i+c3*t16i+c4*t17i+c5*t18i; \
                    y2r = t13r+c2*t14r+c4*t15r+c6*t16r+c5*t17r+c3*t18r; \
                    y2i = t13i+c2*t14i+c4*t15i+c6*t16i+c5*t17i+c3*t18i; \
                    y3r = t13r+c3*t14r+c6*t15r+c4*t16r+c1*t17r+c2*t18r; \
                    y3i = t13i+c3*t14i+c6*t15i+c4*t16i+c1*t17i+c2*t18i; \
                    y4r = t13r+c4*t14r+c5*t15r+c1*t16r+c3*t17r+c6*t18r; \
                    y4i = t13i+c4*t14i+c5*t15i+c1*t16i+c3*t17i+c6*t18i; \
                    y5r = t13r+c5*t14r+c3*t15r+c2*t16r+c6*t17r+c1*t18r; \
                    y5i = t13i+c5*t14i+c3*t15i+c2*t16i+c6*t17i+c1*t18i; \
                    y6r = t13r+c6*t14r+c1*t15r+c5*t16r+c2*t17r+c4*t18r; \
                    y6i = t13i+c6*t14i+c1*t15i+c5*t16i+c2*t17i+c4*t18i; \
                    y7r = c12*t7r-c7*t8r+c11*t9r-c8*t10r+c10*t11r-c9*t12r; \
                    y7i = c12*t7i-c7*t8i+c11*t9i-c8*t10i+c10*t11i-c9*t12i; \
                    y8r = c11*t7r-c9*t8r+c8*t9r-c12*t10r-c7*t11r+c10*t12r; \
                    y8i = c11*t7i-c9*t8i+c8*t9i-c12*t10i-c7*t11i+c10*t12i; \
                    y9r = c10*t7r-c11*t8r-c7*t9r+c9*t10r-c12*t11r-c8*t12r; \
                    y9i = c10*t7i-c11*t8i-c7*t9i+c9*t10i-c12*t11i-c8*t12i; \
                    y10r = c9*t7r+c12*t8r-c10*t9r-c7*t10r+c8*t11r+c11*t12r; \
                    y10i = c9*t7i+c12*t8i-c10*t9i-c7*t10i+c8*t11i+c11*t12i; \
                    y11r = c8*t7r+c10*t8r+c12*t9r-c11*t10r-c9*t11r-c7*t12r; \
                    y11i = c8*t7i+c10*t8i+c12*t9i-c11*t10i-c9*t11i-c7*t12i; \
                    y12r = c7*t7r+c8*t8r+c9*t9r+c10*t10r+c11*t11r+c12*t12r; \
                    y12i = c7*t7i+c8*t8i+c9*t9i+c10*t10i+c11*t11i+c12*t12i; \
                    z[j0] = z[j0]+t1r+t2r+t3r+t4r+t5r+t6r; \
                    z[j0i] = z[j0i]+t1i+t2i+t3i+t4i+t5i+t6i; \
                    z[j1] = y1r-y12i; \
                    z[j1i] = y1i+y12r; \
                    z[j2] = y2r-y11i; \
                    z[j2i] = y2i+y11r; \
                    z[j3] = y3r-y10i; \
                    z[j3i] = y3i+y10r; \
                    z[j4] = y4r-y9i; \
                    z[j4i] = y4i+y9r; \
                    z[j5] = y5r-y8i; \
                    z[j5i] = y5i+y8r; \
                    z[j6] = y6r-y7i; \
                    z[j6i] = y6i+y7r; \
                    z[j7] = y6r+y7i; \
                    z[j7i] = y6i-y7r; \
                    z[j8] = y5r+y8i; \
                    z[j8i] = y5i-y8r; \
                    z[j9] = y4r+y9i; \
                    z[j9i] = y4i-y9r; \
                    z[j10] = y3r+y10i; \
                    z[j10i] = y3i-y10r; \
                    z[j11] = y2r+y11i; \
                    z[j11i] = y2i-y11r; \
                    z[j12] = y1r+y12i; \
                    z[j12i] = y1i-y12r; \
		    loopend; \
                } \
                const int it = i12+istep; \
                i12 = i11+istep; \
                i11 = i10+istep; \
                i10 = i9+istep; \
                i9 = i8+istep; \
                i8 = i7+istep; \
                i7 = i6+istep; \
                i6 = i5+istep; \
                i5 = i4+istep; \
                i4 = i3+istep; \
                i3 = i2+istep; \
                i2 = i1+istep; \
                i1 = i0+istep; \
                i0 = it; \
            } \
            continue; \
        } \
        i13 = i12+iinc; \
        if (i13>=imax) i13 = i13-imax; \
        i14 = i13+iinc; \
        if (i14>=imax) i14 = i14-imax; \
        i15 = i14+iinc; \
        if (i15>=imax) i15 = i15-imax; \
        if (ifac==16) { \
                if (mu==1) { \
	                c1 = 1.0; \
	                    c2 = P923; \
	                    c3 = P382; \
	                c4 = P707; \
                } else if (mu==3) { \
                c1 = -1.0; \
                c2 = P382; \
                c3 = P923; \
                c4 = -P707; \
            } else if (mu==5) { \
                c1 = 1.0; \
                c2 = -P382; \
                c3 = P923; \
                c4 = -P707; \
            } else if (mu==7) { \
                c1 = -1.0; \
                c2 = -P923; \
                c3 = P382; \
                c4 = P707; \
            } else if (mu==9) { \
                c1 = 1.0; \
                c2 = -P923; \
                c3 = -P382; \
                c4 = P707; \
            } else if (mu==11) { \
                c1 = -1.0; \
                c2 = -P382; \
                c3 = -P923; \
                c4 = -P707; \
            } else if (mu==13) { \
                c1 = 1.0; \
                c2 = P382; \
                c3 = -P923; \
                c4 = -P707; \
            } else { \
                c1 = -1.0; \
                c2 = P923; \
                c3 = -P382; \
                c4 = P707; \
            } \
            c5 = c1*c4; \
            c6 = c1*c3; \
            c7 = c1*c2; \
            for (l=0; l<m; l++) { \
		loopinit; \
                for (jt=0; jt<nt; jt++) { \
		    loopstart; \
                    j0 = i0 + offset; \
                    j1 = i1 + offset; \
		    j2 = i2 + offset; \
		    j3 = i3 + offset; \
		    j4 = i4 + offset; \
		    j5 = i5 + offset; \
		    j6 = i6 + offset; \
		    j7 = i7 + offset; \
		    j8 = i8 + offset; \
		    j9 = i9 + offset; \
		    j10 = i10 + offset; \
		    j11 = i11 + offset; \
		    j12 = i12 + offset; \
		    j13 = i13 + offset; \
		    j14 = i14 + offset; \
		    j15 = i15 + offset; \
		    const int j0i = j0+1; \
		    const int j1i = j1+1; \
		    const int j2i = j2+1; \
		    const int j3i = j3+1; \
		    const int j4i = j4+1; \
		    const int j5i = j5+1; \
		    const int j6i = j6+1; \
		    const int j7i = j7+1; \
		    const int j8i = j8+1; \
		    const int j9i = j9+1; \
		    const int j10i = j10+1; \
		    const int j11i = j11+1; \
		    const int j12i = j12+1; \
		    const int j13i = j13+1; \
		    const int j14i = j14+1; \
		    const int j15i = j15+1; \
		    t1r = z[j0]+z[j8]; \
		    t1i = z[j0i]+z[j8i]; \
		    t2r = z[j4]+z[j12]; \
		    t2i = z[j4i]+z[j12i]; \
		    t3r = z[j0]-z[j8]; \
		    t3i = z[j0i]-z[j8i]; \
		    t4r = c1*(z[j4]-z[j12]); \
		    t4i = c1*(z[j4i]-z[j12i]); \
		    t5r = t1r+t2r; \
		    t5i = t1i+t2i; \
		    t6r = t1r-t2r; \
		    t6i = t1i-t2i; \
		    t7r = z[j1]+z[j9]; \
		    t7i = z[j1i]+z[j9i]; \
		    t8r = z[j5]+z[j13]; \
		    t8i = z[j5i]+z[j13i]; \
		    t9r = z[j1]-z[j9]; \
		    t9i = z[j1i]-z[j9i]; \
		    t10r = z[j5]-z[j13]; \
		    t10i = z[j5i]-z[j13i]; \
		    t11r = t7r+t8r; \
		    t11i = t7i+t8i; \
		    t12r = t7r-t8r; \
		    t12i = t7i-t8i; \
		    t13r = z[j2]+z[j10]; \
		    t13i = z[j2i]+z[j10i]; \
		    t14r = z[j6]+z[j14]; \
		    t14i = z[j6i]+z[j14i]; \
		    t15r = z[j2]-z[j10]; \
		    t15i = z[j2i]-z[j10i]; \
		    t16r = z[j6]-z[j14]; \
		    t16i = z[j6i]-z[j14i]; \
		    t17r = t13r+t14r; \
		    t17i = t13i+t14i; \
		    t18r = c4*(t15r-t16r); \
		    t18i = c4*(t15i-t16i); \
		    t19r = c5*(t15r+t16r); \
		    t19i = c5*(t15i+t16i); \
		    t20r = c1*(t13r-t14r); \
		    t20i = c1*(t13i-t14i); \
		    t21r = z[j3]+z[j11]; \
		    t21i = z[j3i]+z[j11i]; \
		    t22r = z[j7]+z[j15]; \
		    t22i = z[j7i]+z[j15i]; \
		    t23r = z[j3]-z[j11]; \
		    t23i = z[j3i]-z[j11i]; \
		    t24r = z[j7]-z[j15]; \
		    t24i = z[j7i]-z[j15i]; \
		    t25r = t21r+t22r; \
		    t25i = t21i+t22i; \
		    t26r = t21r-t22r; \
		    t26i = t21i-t22i; \
		    t27r = t9r+t24r; \
		    t27i = t9i+t24i; \
		    t28r = t10r+t23r; \
		    t28i = t10i+t23i; \
		    t29r = t9r-t24r; \
		    t29i = t9i-t24i; \
		    t30r = t10r-t23r; \
		    t30i = t10i-t23i; \
		    t31r = t5r+t17r; \
		    t31i = t5i+t17i; \
		    t32r = t11r+t25r; \
		    t32i = t11i+t25i; \
		    t33r = t3r+t18r; \
		    t33i = t3i+t18i; \
		    t34r = c2*t29r-c6*t30r; \
		    t34i = c2*t29i-c6*t30i; \
		    t35r = t3r-t18r; \
		    t35i = t3i-t18i; \
		    t36r = c7*t27r-c3*t28r; \
		    t36i = c7*t27i-c3*t28i; \
		    t37r = t4r+t19r; \
		    t37i = t4i+t19i; \
		    t38r = c3*t27r+c7*t28r; \
		    t38i = c3*t27i+c7*t28i; \
		    t39r = t4r-t19r; \
		    t39i = t4i-t19i; \
		    t40r = c6*t29r+c2*t30r; \
		    t40i = c6*t29i+c2*t30i; \
		    t41r = c4*(t12r-t26r); \
		    t41i = c4*(t12i-t26i); \
		    t42r = c5*(t12r+t26r); \
		    t42i = c5*(t12i+t26i); \
		    y1r = t33r+t34r; \
		    y1i = t33i+t34i; \
		    y2r = t6r+t41r; \
		    y2i = t6i+t41i; \
		    y3r = t35r+t40r; \
		    y3i = t35i+t40i; \
		    y4r = t5r-t17r; \
		    y4i = t5i-t17i; \
		    y5r = t35r-t40r; \
		    y5i = t35i-t40i; \
		    y6r = t6r-t41r; \
		    y6i = t6i-t41i; \
		    y7r = t33r-t34r; \
		    y7i = t33i-t34i; \
		    y9r = t38r-t37r; \
		    y9i = t38i-t37i; \
		    y10r = t42r-t20r; \
		    y10i = t42i-t20i; \
		    y11r = t36r+t39r; \
		    y11i = t36i+t39i; \
		    y12r = c1*(t11r-t25r); \
		    y12i = c1*(t11i-t25i); \
		    y13r = t36r-t39r; \
		    y13i = t36i-t39i; \
		    y14r = t42r+t20r; \
		    y14i = t42i+t20i; \
		    y15r = t38r+t37r; \
		    y15i = t38i+t37i; \
		    z[j0] = t31r+t32r; \
		    z[j0i] = t31i+t32i; \
		    z[j1] = y1r-y15i; \
		    z[j1i] = y1i+y15r; \
		    z[j2] = y2r-y14i; \
		    z[j2i] = y2i+y14r; \
		    z[j3] = y3r-y13i; \
		    z[j3i] = y3i+y13r; \
		    z[j4] = y4r-y12i; \
		    z[j4i] = y4i+y12r; \
		    z[j5] = y5r-y11i; \
		    z[j5i] = y5i+y11r; \
		    z[j6] = y6r-y10i; \
		    z[j6i] = y6i+y10r; \
		    z[j7] = y7r-y9i; \
		    z[j7i] = y7i+y9r; \
		    z[j8] = t31r-t32r; \
		    z[j8i] = t31i-t32i; \
		    z[j9] = y7r+y9i; \
		    z[j9i] = y7i-y9r; \
		    z[j10] = y6r+y10i; \
		    z[j10i] = y6i-y10r; \
		    z[j11] = y5r+y11i; \
		    z[j11i] = y5i-y11r; \
		    z[j12] = y4r+y12i; \
		    z[j12i] = y4i-y12r; \
		    z[j13] = y3r+y13i; \
		    z[j13i] = y3i-y13r; \
		    z[j14] = y2r+y14i; \
		    z[j14i] = y2i-y14r; \
		    z[j15] = y1r+y15i; \
		    z[j15i] = y1i-y15r; \
		    loopend; \
                } \
		const int it = i15+istep; \
		i15 = i14+istep; \
		i14 = i13+istep; \
		i13 = i12+istep; \
		i12 = i11+istep; \
                i11 = i10+istep; \
                i10 = i9+istep; \
                i9 = i8+istep; \
                i8 = i7+istep; \
                i7 = i6+istep; \
                i6 = i5+istep; \
                i5 = i4+istep; \
                i4 = i3+istep; \
                i3 = i2+istep; \
                i2 = i1+istep; \
                i1 = i0+istep; \
                i0 = it; \
            } \
            continue; \
        } \
    }


static int kfax[] = { 16,13,11,9,8,7,5,4,3,2 };
#define NTAB 240
static struct {
int n;  float c;
} nctab[NTAB] = {
{       1, 0.000052 },
{       2, 0.000061 },
{       3, 0.000030 },
{       4, 0.000053 },
{       5, 0.000066 },
{       6, 0.000067 },
{       7, 0.000071 },
{       8, 0.000062 },
{       9, 0.000079 },
{      10, 0.000080 },
{      11, 0.000052 },
{      12, 0.000069 },
{      13, 0.000103 },
{      14, 0.000123 },
{      15, 0.000050 },
{      16, 0.000086 },
{      18, 0.000108 },
{      20, 0.000101 },
{      21, 0.000098 },
{      22, 0.000135 },
{      24, 0.000090 },
{      26, 0.000165 },
{      28, 0.000084 },
{      30, 0.000132 },
{      33, 0.000158 },
{      35, 0.000138 },
{      36, 0.000147 },
{      39, 0.000207 },
{      40, 0.000156 },
{      42, 0.000158 },
{      44, 0.000176 },
{      45, 0.000171 },
{      48, 0.000185 },
{      52, 0.000227 },
{      55, 0.000242 },
{      56, 0.000194 },
{      60, 0.000215 },
{      63, 0.000233 },
{      65, 0.000288 },
{      66, 0.000271 },
{      70, 0.000248 },
{      72, 0.000247 },
{      77, 0.000285 },
{      78, 0.000395 },
{      80, 0.000285 },
{      84, 0.000209 },
{      88, 0.000332 },
{      90, 0.000321 },
{      91, 0.000372 },
{      99, 0.000400 },
{     104, 0.000391 },
{     105, 0.000358 },
{     110, 0.000440 },
{     112, 0.000367 },
{     117, 0.000494 },
{     120, 0.000413 },
{     126, 0.000424 },
{     130, 0.000549 },
{     132, 0.000480 },
{     140, 0.000450 },
{     143, 0.000637 },
{     144, 0.000497 },
{     154, 0.000590 },
{     156, 0.000626 },
{     165, 0.000654 },
{     168, 0.000536 },
{     176, 0.000656 },
{     180, 0.000611 },
{     182, 0.000730 },
{     195, 0.000839 },
{     198, 0.000786 },
{     208, 0.000835 },
{     210, 0.000751 },
{     220, 0.000826 },
{     231, 0.000926 },
{     234, 0.000991 },
{     240, 0.000852 },
{     252, 0.000820 },
{     260, 0.001053 },
{     264, 0.000987 },
{     273, 0.001152 },
{     280, 0.000952 },
{     286, 0.001299 },
{     308, 0.001155 },
{     312, 0.001270 },
{     315, 0.001156 },
{     330, 0.001397 },
{     336, 0.001173 },
{     360, 0.001259 },
{     364, 0.001471 },
{     385, 0.001569 },
{     390, 0.001767 },
{     396, 0.001552 },
{     420, 0.001516 },
{     429, 0.002015 },
{     440, 0.001748 },
{     455, 0.001988 },
{     462, 0.001921 },
{     468, 0.001956 },
{     495, 0.002106 },
{     504, 0.001769 },
{     520, 0.002196 },
{     528, 0.002127 },
{     546, 0.002454 },
{     560, 0.002099 },
{     572, 0.002632 },
{     585, 0.002665 },
{     616, 0.002397 },
{     624, 0.002711 },
{     630, 0.002496 },
{     660, 0.002812 },
{     693, 0.002949 },
{     715, 0.003571 },
{     720, 0.002783 },
{     728, 0.003060 },
{     770, 0.003392 },
{     780, 0.003553 },
{     792, 0.003198 },
{     819, 0.003726 },
{     840, 0.003234 },
{     858, 0.004354 },
{     880, 0.003800 },
{     910, 0.004304 },
{     924, 0.003975 },
{     936, 0.004123 },
{     990, 0.004517 },
{    1001, 0.005066 },
{    1008, 0.003902 },
{    1040, 0.004785 },
{    1092, 0.005017 },
{    1144, 0.005599 },
{    1155, 0.005380 },
{    1170, 0.005730 },
{    1232, 0.005323 },
{    1260, 0.005112 },
{    1287, 0.006658 },
{    1320, 0.005974 },
{    1365, 0.006781 },
{    1386, 0.006413 },
{    1430, 0.007622 },
{    1456, 0.006679 },
{    1540, 0.007032 },
{    1560, 0.007538 },
{    1584, 0.007126 },
{    1638, 0.007979 },
{    1680, 0.007225 },
{    1716, 0.008961 },
{    1820, 0.008818 },
{    1848, 0.008427 },
{    1872, 0.009004 },
{    1980, 0.009398 },
{    2002, 0.010830 },
{    2145, 0.012010 },
{    2184, 0.010586 },
{    2288, 0.012058 },
{    2310, 0.011673 },
{    2340, 0.011700 },
{    2520, 0.011062 },
{    2574, 0.014313 },
{    2640, 0.013021 },
{    2730, 0.014606 },
{    2772, 0.013216 },
{    2860, 0.015789 },
{    3003, 0.016988 },
{    3080, 0.014911 },
{    3120, 0.016393 },
{    3276, 0.016741 },
{    3432, 0.018821 },
{    3465, 0.018138 },
{    3640, 0.018892 },
{    3696, 0.018634 },
{    3960, 0.020216 },
{    4004, 0.022455 },
{    4095, 0.022523 },
{    4290, 0.026087 },
{    4368, 0.023474 },
{    4620, 0.024590 },
{    4680, 0.025641 },
{    5005, 0.030303 },
{    5040, 0.025253 },
{    5148, 0.030364 },
{    5460, 0.031250 },
{    5544, 0.029412 },
{    5720, 0.034404 },
{    6006, 0.037500 },
{    6160, 0.034091 },
{    6435, 0.040214 },
{    6552, 0.037221 },
{    6864, 0.042735 },
{    6930, 0.040214 },
{    7280, 0.042980 },
{    7920, 0.045872 },
{    8008, 0.049505 },
{    8190, 0.049834 },
{    8580, 0.055762 },
{    9009, 0.057034 },
{    9240, 0.054945 },
{    9360, 0.056818 },
{   10010, 0.066667 },
{   10296, 0.065502 },
{   10920, 0.068182 },
{   11088, 0.065217 },
{   11440, 0.075000 },
{   12012, 0.078534 },
{   12870, 0.087719 },
{   13104, 0.081081 },
{   13860, 0.084270 },
{   15015, 0.102740 },
{   16016, 0.106383 },
{   16380, 0.105634 },
{   17160, 0.119048 },
{   18018, 0.123967 },
{   18480, 0.119048 },
{   20020, 0.137615 },
{   20592, 0.140187 },
{   21840, 0.154639 },
{   24024, 0.168539 },
{   25740, 0.180723 },
{   27720, 0.180723 },
{   30030, 0.220588 },
{   32760, 0.241935 },
{   34320, 0.254237 },
{   36036, 0.254237 },
{   40040, 0.288462 },
{   45045, 0.357143 },
{   48048, 0.357143 },
{   51480, 0.384615 },
{   55440, 0.384615 },
{   60060, 0.454545 },
{   65520, 0.517241 },
{   72072, 0.576923 },
{   80080, 0.625000 },
{   90090, 0.833333 },
{  102960, 0.789474 },
{  120120, 1.153846 },
{  144144, 1.153846 },
{  180180, 1.875000 },
{  240240, 2.500000 },
{  360360, 3.750000 },
{  720720, 7.500000 },
};


void CC::pfacc( char isign, int n, float_complex* cz )
{
    mPFACCIMPL( 2 );
}


void CC::pfacc( char isign, int n, int step, float_complex* cz )
{
    const int samplestep = 2 * step;
    mPFACCIMPL( samplestep );
}


void CC::pfacc( char isign, int n, int step, int nt, const int* starts,
	    float_complex* cz )
{
  const int istep = 2*step;
  mPFACCND( istep, , const int offset = 2*starts[jt], );
}


void CC::pfacc( char isign, int n, int step, int nt, int batchstep,
	    float_complex* cz )
{
    const int istep = 2*step;
    const int batchincr = 2*batchstep;
    mPFACCND( istep, int offset = 0, , offset += batchincr );
}


void CC::pfarc( int isign, int n, const float* rz, float_complex* cz )
{
    int i,ir,ii,jr,ji,no2;
    float *z,tempr,tempi,sumr,sumi,difr,difi;
    double wr,wi,wpr,wpi,wtemp,theta;

    /* copy input to output while scaling */
    z = (float*)cz;
    for (i=0; i<n; i++)
	z[i] = 0.5*rz[i];

    /* do complex to complex transform */
    pfacc(isign,n/2,cz);

    /* fix dc and nyquist */
    z[n] = 2.0*(z[0]-z[1]);
    z[0] = 2.0*(z[0]+z[1]);
    z[n+1] = 0.0;
    z[1] = 0.0;

    /* initialize cosine-sine recurrence */
    theta = 2.0*M_PI/(double)n;
    if (isign<0) theta = -theta;
	wtemp = sin(0.5*theta);
    wpr = -2.0*wtemp*wtemp;
    wpi = sin(theta);
    wr = 1.0+wpr;
    wi = wpi;

    /* twiddle */
    no2 = n/2;
    for (ir=2,ii=3,jr=n-2,ji=n-1; ir<=no2; ir+=2,ii+=2,jr-=2,ji-=2) {
	sumr = z[ir]+z[jr];
	sumi = z[ii]+z[ji];
	difr = z[ir]-z[jr];
	difi = z[ii]-z[ji];
	tempr = wi*difr+wr*sumi;
	tempi = wi*sumi-wr*difr;
	z[ir] = sumr+tempr;
	z[ii] = difi+tempi;
	z[jr] = sumr-tempr;
	z[ji] = tempi-difi;
	wtemp = wr;
	wr += wr*wpr-wi*wpi;
	wi += wi*wpr+wtemp*wpi;
    }
}


void CC::pfacr(int isign, int n, const float_complex* cz, float* rz )
{
    int i,ir,ii,jr,ji,no2;
    float *z,tempr,tempi,sumr,sumi,difr,difi;
    double wr,wi,wpr,wpi,wtemp,theta;

    /* copy input to output and fix dc and nyquist */
    z = (float*)cz;
    for (i=2; i<n; i++)
	rz[i] = z[i];
    rz[1] = z[0]-z[n];
    rz[0] = z[0]+z[n];
    z = rz;

    /* initialize cosine-sine recurrence */
    theta = 2.0*M_PI/(double)n;
    if (isign>0) theta = -theta;
    wtemp = sin(0.5*theta);
    wpr = -2.0*wtemp*wtemp;
    wpi = sin(theta);
    wr = 1.0+wpr;
    wi = wpi;
    no2 = n/2;
    for (ir=2,ii=3,jr=n-2,ji=n-1; ir<=no2; ir+=2,ii+=2,jr-=2,ji-=2) {
	sumr = z[ir]+z[jr];
	sumi = z[ii]+z[ji];
	difr = z[ir]-z[jr];
	difi = z[ii]-z[ji];
	tempr = wi*difr-wr*sumi;
	tempi = wi*sumi+wr*difr;
	z[ir] = sumr+tempr;
	z[ii] = difi+tempi;
	z[jr] = sumr-tempr;
	z[ji] = tempi-difi;
	wtemp = wr;
	wr += wr*wpr-wi*wpi;
	wi += wi*wpr+wtemp*wpi;
    }

    /* do complex to complex transform */
    pfacc(isign,n/2,(float_complex*)z);
}


int CC::CC1D::getFastSize( int nmin )
{
    int i;
    for ( i=0; i<NTAB-1 && nctab[i].n<nmin; ++i );
    return nctab[i].n;
}


int CC::npfao( int nmin, int nmax )
{
    int i,j;
    for (i=0; i<NTAB-1 && nctab[i].n<nmin; ++i);
    for (j=i+1; j<NTAB-1 && nctab[j].n<=nmax; ++j)
	if (nctab[j].c<nctab[i].c) i = j;

    return nctab[i].n;
}


int CC::npfaro( int nmin, int nmax )
{
    return 2*npfao((nmin+1)/2,(nmax+1)/2);
}

}; //namespace
