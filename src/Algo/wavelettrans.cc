/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Mar 2000
-*/

static const char* rcsID mUsedVar = "$Id$";

#include <iostream>

#include "wavelettrans.h"
#include "arrayndimpl.h"
#include "simpnumer.h"


DefineEnumNames(WaveletTransform,WaveletType,0,"Wavelet Type")
{ "Haar", "Daubechies4", "Daubechies6", "Daubechies8", "Daubechies10",
  "Daubechies12", "Daubechies14", "Daubechies16", "Daubechies18",
  "Daubechies20", "Beylkin", "Coiflet1", "Coiflet2", "Coiflet3",
  "Coiflet4", "Coiflet5", "Symmlet4", "Symmlet5", "Symmlet6",
  "Symmlet7", "Symmlet8", "Symmlet9", "Symmlet10", "Vaidyanathan", 0 };



bool WaveletTransform::isCplx(WaveletType wt)
{
    return false;
}


/* The filter coefficients are taken from the MATLAB function 'MakeONFilter'
from the Stanford University Wavelet package 'WaveLab'. */


const float WaveletTransform::haar[3] =
				{ 0.0,  0.70710678,  0.70710678 };

const float WaveletTransform::beylkin[19] =
				{ 0.0,   0.099305765374,  0.424215360813,
					 0.699825214057,  0.449718251149,
					-0.110927598348, -0.264497231446,
					 0.026900308804,  0.155538731877,
					-0.017520746267, -0.088543630623,
					 0.019679866044,  0.042916387274,
					-0.017460408696, -0.014365807969,
					 0.010040411845,  0.001484234782,
					-0.002736031626,  0.000640485329};

const float WaveletTransform::daub4[5] =
				{ 0.0,  0.482962913145,  0.836516303738,
					0.224143868042, -0.129409522551 };

const float WaveletTransform::daub6[7] =
				{ 0.0,   0.332670552950,  0.806891509311,
					 0.459877502118, -0.135011020010,
					-0.085441273882,  0.035226291882 };

const float WaveletTransform::daub8[9] =
				{ 0.0,   0.230377813309,  0.714846570553,
					 0.630880767930, -0.027983769410,
					-0.187034811719,  0.030841381836,
					 0.032883011667, -0.010597401785  };

const float WaveletTransform::daub10[11] =
				{ 0.0,   0.160102397974,  0.603829269797,
					 0.724308528438,  0.138428145901,
					-0.242294887066, -0.032244869585,
					 0.077571493840, -0.006241490213,
					-0.012580751999,  0.003335725285 };

const float WaveletTransform::daub12[13] =
				{ 0.0,   0.111540743350,  0.494623890398,
					 0.751133908021,  0.315250351709,
					-0.226264693965, -0.129766867567,
					 0.097501605587,  0.027522865530,
					-0.031582039317,  0.000553842201,
					 0.004777257511, -0.001077301085 };

const float WaveletTransform::daub14[15] =
				{ 0.0,   0.077852054085,  0.396539319482,
					 0.729132090846,  0.469782287405,
					-0.143906003929, -0.224036184994,
					 0.071309219267,  0.080612609151,
					-0.038029936935, -0.016574541631,
					 0.012550998556,  0.000429577973,
					-0.001801640704,  0.000353713800 };

const float WaveletTransform::daub16[17] =
				{ 0.0,   0.054415842243,  0.312871590914,
					 0.675630736297,  0.585354683654,
					-0.015829105256, -0.284015542962,
					 0.000472484574,  0.128747426620,
					-0.017369301002, -0.044088253931,
					 0.013981027917,  0.008746094047,
					-0.004870352993, -0.000391740373,
					 0.000675449406, -0.000117476784 };

const float WaveletTransform::daub18[19] =
				{ 0.0,   0.038077947364,  0.243834674613,
					 0.604823123690,  0.657288078051,
					 0.133197385825, -0.293273783279,
					-0.096840783223,  0.148540749338,
					 0.030725681479, -0.067632829061,
					 0.000250947115,  0.022361662124,
					-0.004723204758, -0.004281503682,
					 0.001847646883,  0.000230385764,
					-0.000251963189,  0.000039347320 };

const float WaveletTransform::daub20[21] =
				{ 0.0,   0.026670057901,  0.188176800078,
					 0.527201188932,  0.688459039454,
					 0.281172343661, -0.249846424327,
					-0.195946274377,  0.127369340336,
					 0.093057364604, -0.071394147166,
					-0.029457536822,  0.033212674059,
					 0.003606553567, -0.010733175483,
					 0.001395351747,  0.001992405295,
					-0.000685856695, -0.000116466855,
					 0.000093588670, -0.000013264203 };

const float WaveletTransform::coiflet1[7] =
				{ 0.0,   0.038580777748, -0.126969125396,
					-0.077161555496, 0.607491641386,
					 0.745687558934, 0.226584265197 };


const float WaveletTransform::coiflet2[13] =
				{ 0.0,   0.016387336463, -0.041464936782,
					-0.067372554722,  0.386110066823,
					 0.812723635450,  0.417005184424,
					-0.076488599078, -0.059434418646,
					 0.023680171947,  0.005611434819,
					-0.001823208871, -.000720549445 };


const float WaveletTransform::coiflet3[19] =
				{ 0.0,  -0.003793512864,  0.007782596426,
					 0.023452696142, -0.065771911281,
					-0.061123390003,  0.405176902410,
					 0.793777222626,  0.428483476378,
					-0.071799821619, -0.082301927106,
					 0.034555027573,  0.015880544864,
					-0.009007976137, -0.002574517688,
					 0.001117518771,  0.000466216960,
					-0.000070983303, -0.000034599773 };


const float WaveletTransform::coiflet4[25] =
				{ 0.0,   0.000892313668, -0.001629492013,
					-0.007346166328,  0.016068943964,
					 0.026682300156, -0.081266699680,
					-0.056077313316,  0.415308407030,
					 0.782238930920,  0.434386056491,
					-0.066627474263, -0.096220442034,
					 0.039334427123,  0.025082261845,
					-0.015211731527, -0.005658286686,
					 0.003751436157,  0.001266561929,
					-0.000589020757, -0.000259974552,
					 0.000062339034,  0.000031229876,
					-0.000003259680, -0.000001784985 };

const float WaveletTransform::coiflet5[31] =
				{ 0.0,  -0.000212080863,  0.000358589677,
					 0.002178236305, -0.004159358782,
					-0.010131117538,  0.023408156762,
					 0.028168029062, -0.091920010549,
					-0.052043163216,  0.421566206729,
					 0.774289603740,  0.437991626228,
					-0.062035963906, -0.105574208706,
					 0.041289208741,  0.032683574283,
					-0.019761779012, -0.009164231153,
					 0.006764185419,  0.002433373209,
					-0.001662863769, -0.000638131296,
					 0.000302259520,  0.000140541149,
					-0.000041340484, -0.000021315014,
					 0.000003734597,  0.000002063806,
					-0.000000167408, -0.000000095158};
 
const float WaveletTransform::symmlet4[9] =
				{ 0.0,  -0.107148901418, -0.041910965125,
					 0.703739068656,  1.136658243408,
					 0.421234534204, -0.140317624179,
					-0.017824701442,  0.045570345896};

const float WaveletTransform::symmlet5[11] =
				{ 0.0,   0.038654795955,  0.041746864422,
					-0.055344186117,  0.281990696854,
					 1.023052966894,  0.896581648380,
					 0.023478923136, -0.247951362613,
					-0.029842499869,  0.027632152958};

const float WaveletTransform::symmlet6[13] =
                                { 0.0,   0.021784700327,  0.004936612372,
					-0.166863215412, -0.068323121587,
					 0.694457972958,  1.113892783926,
					 0.477904371333, -0.102724969862,
					-0.029783751299,  0.063250562660,
					 0.002499922093, -0.011031867509};


const float WaveletTransform::symmlet7[15] =
				{ 0.0,   0.003792658534, -0.001481225915,
					-0.017870431651,  0.043155452582,
					 0.096014767936, -0.070078291222,
					 0.024665659489,  0.758162601964,
					 1.085782709814,  0.408183939725,
					-0.198056706807, -0.152463871896,
					 0.005671342686,  0.014521394762};


const float WaveletTransform::symmlet8[17] =
				{ 0.0,   0.002672793393, -0.000428394300,
					-0.021145686528,  0.005386388754,
					 0.069490465911, -0.038493521263,
					-0.073462508761,  0.515398670374,
					 1.099106630537,  0.680745347190,
					-0.086653615406, -0.202648655286,
					 0.010758611751,  0.044823623042,
					-0.000766690896, -0.004783458512};


const float WaveletTransform::symmlet9[19] =
				{ 0.0,   0.001512487309, -0.000669141509,
					-0.014515578553,  0.012528896242,
					 0.087791251554, -0.025786445930,
					-0.270893783503,  0.049882830959,
					 0.873048407349,  1.015259790832,
					 0.337658923602, -0.077172161097,
					 0.000825140929,  0.042744433602,
					-0.016303351226, -0.018769396836,
					 0.000876502539,  0.001981193736};

const float WaveletTransform::symmlet10[21] =
				{ 0.0,   0.001089170447,  0.000135245020,
					-0.012220642630, -0.002072363923,
					 0.064950924579,  0.016418869426,
					-0.225558972234, -0.100240215031,
					 0.667071338154,  1.088251530500,
					 0.542813011213, -0.050256540092,
					-0.045240772218,  0.070703567550,
					 0.008152816799, -0.028786231926,
					-0.001137535314,  0.006495728375,
					 0.000080661204, -0.000649589896};

const float WaveletTransform::vaidyanathan[25] =
				{ 0.0,  -0.000062906118,  0.000343631905,
					-0.000453956620, -0.000944897136,
					 0.002843834547,  0.000708137504,
					-0.008839103409,  0.003153847056,
					 0.019687215010, -0.014853448005,
					-0.035470398607,  0.038742619293,
					 0.055892523691, -0.077709750902,
					-0.083928884366,  0.131971661417,
					 0.135084227129, -0.194450471766,
					-0.263494802488,  0.201612161775,
					 0.635601059872,  0.572797793211,
					 0.250184129505,  0.045799334111};


void WaveletTransform::getInfo( WaveletType wt, int& len, 
				TypeSet<float>& wavelet )
{
    const float* wc;
    switch ( wt )
    {
	case WaveletTransform::Haar:
		len = 2;	wc = WaveletTransform::haar;		break;
	case WaveletTransform::Daubechies4:
		len = 4;	wc = WaveletTransform::daub4;		break;
	case WaveletTransform::Daubechies6:
		len = 6;	wc = WaveletTransform::daub6;		break;
	case WaveletTransform::Daubechies8:
		len = 8;	wc = WaveletTransform::daub8;		break;
	case WaveletTransform::Daubechies10:
		len = 10;	wc = WaveletTransform::daub10;		break;
	case WaveletTransform::Daubechies12:
		len = 12;	wc = WaveletTransform::daub12;		break;
	case WaveletTransform::Daubechies14:
		len = 14;	wc = WaveletTransform::daub14;		break;
	case WaveletTransform::Daubechies16:
		len = 16;	wc = WaveletTransform::daub16;		break;
	case WaveletTransform::Daubechies18:
		len = 18;	wc = WaveletTransform::daub18;		break;
	case WaveletTransform::Daubechies20:
		len = 20;	wc = WaveletTransform::daub20;		break;
	case WaveletTransform::Beylkin:	
		len = 18;  	wc = WaveletTransform::beylkin;		break;
	case WaveletTransform::Coiflet1:	
		len = 6;	wc = WaveletTransform::coiflet1;	break;
	case WaveletTransform::Coiflet2:	
		len = 12;	wc = WaveletTransform::coiflet2; 	break;
	case WaveletTransform::Coiflet3:	
		len = 18;	wc = WaveletTransform::coiflet3; 	break;
	case WaveletTransform::Coiflet4:	
		len = 24;	wc = WaveletTransform::coiflet4; 	break;
	case WaveletTransform::Coiflet5:	
		len = 30;	wc = WaveletTransform::coiflet5; 	break;
	case WaveletTransform::Symmlet4:	
		len = 8;	wc = WaveletTransform::symmlet4; 	break;
	case WaveletTransform::Symmlet5:	
		len = 10;	wc = WaveletTransform::symmlet5; 	break;
	case WaveletTransform::Symmlet6:	
		len = 12;	wc = WaveletTransform::symmlet6; 	break;
	case WaveletTransform::Symmlet7:	
		len = 14;	wc = WaveletTransform::symmlet7; 	break;
	case WaveletTransform::Symmlet8:	
		len = 16;	wc = WaveletTransform::symmlet8; 	break;
	case WaveletTransform::Symmlet9:	
		len = 18;	wc = WaveletTransform::symmlet9; 	break;
	case WaveletTransform::Symmlet10:
		len = 20;	wc = WaveletTransform::symmlet10;	break;
	case WaveletTransform::Vaidyanathan:
		len = 24;	wc = WaveletTransform::vaidyanathan; 	break;
    }

    for ( int idx=0; idx<len; idx++ )
	wavelet += wc[idx];
}


DWT::DWT( WaveletTransform::WaveletType t )
    : wt_( t )
{}


bool DWT::setup()
{
    if ( !GenericTransformND::setup() ) return false;

    for ( int idx=0; idx<transforms_.size(); idx++ )
    {
	((FilterWT1D*)transforms_[idx])->setWaveletType( wt_ );
	if ( !transforms_[idx]->init() ) return false;
    }

    return true;
}
	

bool DWT::FilterWT1D::init()
{
    if ( sz_ < 0 ) return false;

    TypeSet<float> tcc;
    WaveletTransform::getInfo( wt_, filtersz_, tcc );

    if ( cc_ ) delete [] cc_;
    cc_ = new float[filtersz_+1];

    float len = 0;
    for ( int idx=0; idx<=filtersz_; idx++ )
    { len += tcc[idx]*tcc[idx]; }

    for ( int idx=0; idx<=filtersz_; idx++ )
    { cc_[idx] = tcc[idx] / len; }

    if ( cr_ ) delete [] cr_;
    cr_ = new float[filtersz_+1];

    int sig = -1;
    for ( int k=1; k<=filtersz_; k++ )
    {
	cr_[filtersz_+1-k]=sig*cc_[k];
	sig = -sig;
    }

    ioff_ = -2; // -(filtersz_ >> 1);
    joff_ = -filtersz_;

    return true;
}


bool DWT::FilterWT1D::run( bool )
{
    if ( cinput_ && coutput_ )
    {
	for ( int idx=0; idx<nr_; idx++ )
	{
	    int offset = batchstarts_ ? batchstarts_[idx] : idx*batchsampling_;
	    transform1Dt( cinput_+offset, coutput_+offset, sampling_ );
	}
    }
    else if ( rinput_ && routput_ )
    {
	for ( int idx=0; idx<nr_; idx++ )
	{
	    int offset = batchstarts_ ? batchstarts_[idx] : idx*batchsampling_;
	    transform1Dt( rinput_+offset, routput_+offset, sampling_ );
	}
    }

    return true;
}


void DWT::FilterWT1D::setWaveletType( WaveletTransform::WaveletType nt )
{ wt_ = nt; }


// ***** CWTWavelets *****
void CWT::CWTWavelets::createWavelet( WaveletType wt, int nrsamples,
				      float scale )
{
    TypeSet<float> wavelet;
    if ( wt == Gaussian )
	createGaussWavelet( nrsamples, scale, wavelet );
    else if ( wt == Morlet )
	createMorletWavelet( nrsamples, scale, wavelet );
    else if ( wt == MexicanHat )
	createMexhatWavelet( nrsamples, scale, wavelet );

    scales_ += scale;
    wavelets_ += wavelet;
}


void CWT::CWTWavelets::createMorletWavelet( int nrsamples, float scale,
       					    TypeSet<float>& wavelet )
{

    for ( int idx=0; idx<nrsamples; idx++ )
    {
	int omidx = idx<=nrsamples/2 ? idx : idx-nrsamples;
	float omega0 = idx<=nrsamples/2 ? 5.f : -5.f;
	float omega =  2 * M_PIf * omidx / scale;
        float val = (omega-omega0) * (omega-omega0) / 2;
	wavelet += exp( -val );
    }
}


void CWT::CWTWavelets::createMexhatWavelet( int nrsamples, float scale,
       					    TypeSet<float>& wavelet )
{
    for ( int idx=0; idx<nrsamples; idx++ )
    {
	int omidx = idx<=nrsamples/2 ? idx : idx-nrsamples;
	float omega = (float) ( 2 * M_PI * omidx / scale );
        float omega2 = omega*omega;
	wavelet += omega2 * exp( -omega2/2 );
    }
}


void CWT::CWTWavelets::createGaussWavelet( int nrsamples, float scale,
       					   TypeSet<float>& wavelet )
{
    for ( int idx=0; idx<nrsamples; idx++ )
    {
	int omidx = idx<=nrsamples/2 ? idx : idx-nrsamples;
	float omega = (float) ( 2 * M_PI * omidx / scale );
        float omega2 = omega*omega;
	wavelet += exp( -omega2/2 );
    }
}


const TypeSet<float>* CWT::CWTWavelets::getWavelet( float scale ) const
{
    float diff = mUdf(float);
    int nearidx = -1;
    for ( int idx=0; idx<scales_.size(); idx++ )
    {
	float newdiff = fabs( scales_[idx] - scale );
	if ( newdiff < diff ) { diff = newdiff; nearidx = idx; }
    }

    return nearidx<0 ? 0 : &wavelets_[nearidx];
}


// ****** CWT *****

DefineEnumNames(CWT,WaveletType,0,"Wavelet Type")
{ "Morlet", "Gaussian", "Mexican Hat", 0 };


CWT::CWT()
    : info_(0)
    , wt_(WaveletType(0))
    , inited_(false)
    , freqrg_(0,0,0)
    , fft_(Fourier::CC::createDefault())
    , ifft_(Fourier::CC::createDefault())
{
    ifft_->setNormalization( true );
}


CWT::~CWT()
{
    delete info_;
    delete fft_;
    delete ifft_;
}


void CWT::setWavelet( WaveletType wt )
{ wt_ = wt; }


bool CWT::isPossible( int sz ) const
{
    if ( sz < 4 ) return false;
    return isPower( sz, 2 );
}


bool CWT::setDir( bool forward )
{
    if ( inited_ && forward == fft_->getDir() ) return true;
    if ( !forward ) return false;

    fft_->setDir( forward );
    fft_->setNormalization( !forward );
    ifft_->setDir( !forward );
    ifft_->setNormalization( forward );

    inited_ = false;
    return true;
}


bool CWT::setInputInfo( const ArrayNDInfo& ni )
{
    if ( info_ && info_->getSize(0) == ni.getSize(0) ) return true;

    fft_->setInputInfo( ni );
    ifft_->setInputInfo( ni );

    delete info_;
    info_ = ni.clone();

    inited_ = false;
    return true;
}


bool CWT::init()
{
    if ( inited_ ) return true;
    
    const int nrsamp = info_->getSize( 0 );
    const int nrsteps = freqrg_.nrSteps()+1;
    
    for ( int idx=0; idx<nrsteps; idx++ ) 
    {
	if ( outfreqidxs_.indexOf(idx) < 0 )
	    continue;

	const float freq = freqrg_.atIndex( idx );
	const float curscale = getScale( nrsamp, dt_, freq );
	wvlts_.createWavelet( wt_, nrsamp, curscale );
    }

    inited_ = true;
    return true;
}


bool CWT::transform( const ArrayND<float_complex>& inp,
		     ArrayND<float>& outp ) 
{
    const int ndim = inp.info().getNDim();
    if ( ndim > 1 ) return false;

    const int outdim = outp.info().getNDim();
    if ( outdim != 2 ) return false;

    mDynamicCastGet(Array2DImpl<float>*,arr2d,&outp)
    if ( !arr2d ) return false;

    const int nrsamples = inp.info().getSize( 0 );
    Array1DImpl<float_complex> freqdom( nrsamples );
    fft_->setInput( inp.getData() );
    fft_->setOutput( freqdom.getData() );
    if ( !fft_->run( true ) )
	return false;

    const int nrsteps = freqrg_.nrSteps()+1;
    arr2d->setSize( nrsamples, nrsteps );
    for ( int idx=0; idx<nrsteps; idx++ ) 
    {
	if ( outfreqidxs_.indexOf(idx) < 0 )
	    continue;

	const float freq = freqrg_.atIndex( idx );
	const float curscale = getScale( nrsamples, dt_, freq );
	transform( nrsamples, curscale, idx, freqdom, *arr2d );
    }

    return true;
}


void CWT::transform( int nrsamples, float curscale, int scaleidx,
		     const Array1DImpl<float_complex>& freqdom,
		     Array2DImpl<float>& outp )
{
    const TypeSet<float>& wavelet = *wvlts_.getWavelet( curscale );

    Array1DImpl<float_complex> filtered( nrsamples );
    for ( int idx=0; idx<nrsamples; idx++ )
    {
	float_complex cval = (freqdom.get(idx) * wavelet[idx])
			   / (float)Math::Sqrt(curscale);
	filtered.set( idx, cval );
    }

    Array1DImpl<float_complex> newsignal( nrsamples );
    ifft_->setInput( filtered.getData() );
    ifft_->setOutput( newsignal.getData() );
    ifft_->run( true );

    for ( int idx=0; idx<nrsamples; idx++ )
    {
	float real = newsignal.get(idx).real();
	float imag = newsignal.get(idx).imag();
	float val = Math::Sqrt( real*real + imag*imag );
	outp.set( idx, scaleidx, val );
    }
}


float CWT::getScale( int nrsamples, float dt, float freq ) const
{
    if ( !nrsamples || mIsZero(dt, mDefEps) )
	return mUdf(float);

    const float df = 1.f / ( dt * nrsamples );
    const float freqidx = freq / df;

    float omega0 = 5;
    if ( wt_ == Gaussian )
	omega0 = sqrt(2.f);
    else if ( wt_ == Morlet )
	omega0 = 5;
    else if ( wt_ == MexicanHat )
	omega0 = sqrt(2.f);

    return (float) ( freqidx * (2*M_PI) / omega0 );
}
