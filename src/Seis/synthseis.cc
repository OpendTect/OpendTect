/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : 23-3-1996
 * FUNCTION : Wavelet
-*/

static const char* rcsID = "$Id: synthseis.cc,v 1.12 2011-03-25 14:42:05 cvsbruno Exp $";

#include "arrayndimpl.h"
#include "fourier.h"
#include "genericnumer.h"
#include "raytrace1d.h"
#include "reflectivitymodel.h"
#include "reflectivitysampler.h"
#include "seistrc.h"
#include "synthseis.h"
#include "survinfo.h"
#include "wavelet.h"

namespace Seis
{

SynthGenerator::SynthGenerator()
    : wavelet_(0)
    , fft_(new Fourier::CC)
    , fftsz_(0)			   
    , freqwavelet_(0)
    , outtrc_(*new SeisTrc)
    , needprepare_(true)			   
{}


SynthGenerator::~SynthGenerator()
{
    if ( waveletismine_ )
	delete wavelet_;

    delete fft_;
    delete [] freqwavelet_;
    delete &outtrc_;
}


bool SynthGenerator::setModel( const ReflectivityModel& refmodel )
{
    refmodel_.erase();
    refmodel_.copy( refmodel );
    return true;
}


bool SynthGenerator::setWavelet( const Wavelet* wvlt, OD::PtrPolicy pol )
{
    if ( waveletismine_ ) 
	{ delete wavelet_; wavelet_ = 0; }
    if ( !wvlt ) 
	{ errmsg_ = "No valid wavelet given"; return false; }
    if ( pol == OD::CopyPtr )
    {
	mDeclareAndTryAlloc( float*, wavelet_, float[wvlt->size()] );
	if ( !wavelet_ )
	    { errmsg_ = "Not enough memory"; return false; }
	MemCopier<float> copier( wavelet_, wvlt->samples(), wvlt->size() );
	copier.execute();
    }
    else 
    {
	wavelet_ = wvlt;
    }
    waveletismine_ = pol != OD::UsePtr;
    needprepare_ = true;

    return true;
}


void SynthGenerator::setConvolDomain( bool fourier )
{
    if ( fourier == ((bool) fft_ ) )
	return;

    if ( fft_ )
    {
	delete fft_; fft_ = 0;
    }
    else
    {
	fft_ = new Fourier::CC;
    }
}


bool SynthGenerator::setOutSampling( const StepInterval<float>& si )
{
    outputsampling_ = si;
    outtrc_.reSize( si.nrSteps(), false );
    outtrc_.info().sampling = si;
    fftsz_ = fft_->getFastSize(  si.nrSteps()+1 );
    needprepare_ = true;
    return true;
}


#define mDoFFT( arr, dir )\
    fft_->setInputInfo( Array1DInfoImpl(fftsz_) );\
    fft_->setDir( dir );\
    fft_->setNormalization( !dir );\
    fft_->setInput( arr );\
    fft_->setOutput( arr );\
    fft_->run( true );

bool SynthGenerator::doPrepare()
{
    if ( !needprepare_ )
	return true;

    if ( freqwavelet_ )
	delete [] freqwavelet_;

    freqwavelet_ = new float_complex[ fftsz_];

    for ( int idx=0; idx<fftsz_; idx++ )
	freqwavelet_[idx] = idx<wavelet_->size() ? wavelet_->samples()[idx] : 0;

    mDoFFT( freqwavelet_, true )

    needprepare_ = false;
    return true;
}


bool SynthGenerator::doWork()
{
    if ( needprepare_ )
	doPrepare();

    if ( refmodel_.isEmpty() ) 
	{ errmsg_ = "No reflectivity model found"; return false; }

    if ( !wavelet_ ) 
	{ errmsg_ = "No wavelet found"; return false; }

    if ( outputsampling_.nrSteps() < 2 )
	{ errmsg_ = "Output sampling is too small"; return false; }

    const int wvltsz = wavelet_->size(); 
    if ( wvltsz < 2 )
	{ errmsg_ = "Wavelet size is too small"; return false; }

    if ( wvltsz > outtrc_.size() )
	{ errmsg_ = "Wavelet is longer than the output trace"; return false; }

    return computeTrace( (float*)outtrc_.data().getComponent(0)->data() );
}


bool SynthGenerator::computeTrace( float* result ) 
{
    outtrc_.zero();
    cresamprefl_.erase();

    ReflectivitySampler sampler( refmodel_, outputsampling_, cresamprefl_ );
    sampler.setTargetDomain( (bool)fft_ );
    sampler.execute();

    return fft_ ? doFFTConvolve( result) : doTimeConvolve( result );
}


bool SynthGenerator::doFFTConvolve( float* result )
{
    if ( !fft_ ) return false; 

    const int wvltsz = wavelet_->size();
    const int nrsamp = outtrc_.size();
    float_complex* cres = new float_complex[ fftsz_ ];

    for ( int idx=0; idx<cresamprefl_.size(); idx++ )
	cres[idx] = cresamprefl_[idx] * freqwavelet_[idx]; 

    mDoFFT( cres, false )

    for ( int idx=0; idx<wvltsz/2; idx++ )
	result[nrsamp-idx-1] = cres[idx].real();
    for ( int idx=0; idx<fftsz_-wvltsz/2; idx++ )
    {
	if ( idx >= nrsamp ) break;
	result[idx] = cres[idx+wvltsz/2].real();
    }

    delete [] cres;
    
    return true;
}


bool SynthGenerator::doTimeConvolve( float* result )
{
    const int ns = outtrc_.size();
    TypeSet<float> refs; getSampledReflectivities( refs );
    const int wvltcs = wavelet_->centerSample();
    const int wvltsz = wavelet_->size();
    GenericConvolve( wvltsz, -wvltcs-1, wavelet_->samples(),
		     refs.size(), 0, refs.arr(),
		     ns, 0, result );
    return true;
}


void SynthGenerator::getSampledReflectivities( TypeSet<float>& refs ) const
{
    for ( int idx=0; idx<cresamprefl_.size(); idx++ )
	refs += cresamprefl_[idx].real();
}


RaySynthGenerator::RaySynthGenerator()
    : raytracer_(*new RayTracer1D(RayTracer1D::Setup()))
{}	


RaySynthGenerator::~RaySynthGenerator()
{
    deepErase( outtrcs_ );
    delete &raytracer_;
}


bool RaySynthGenerator::setOutSampling( const StepInterval<float>& outs )
{
    return synthgenbase_.setOutSampling( outs );
}


void RaySynthGenerator::setConvolDomain( bool fourier )
{
    synthgenbase_.setConvolDomain( fourier );
}


bool RaySynthGenerator::setWavelet( const Wavelet* wvlt, OD::PtrPolicy ptr )
{
    return synthgenbase_.setWavelet( wvlt, ptr );
}


bool RaySynthGenerator::setModel( const AIModel& aim )
{
    aimodel_.copy( aim );
    return true;
}


bool RaySynthGenerator::setOffsets( const TypeSet<float>& offs )
{
    offsets_.copy( offs );
    return true;
}


od_int64 RaySynthGenerator::nrIterations() const
{
    return offsets_.isEmpty() ? 1 : offsets_.size();
}


bool RaySynthGenerator::doPrepare( int thread )
{
    raytracer_.setModel( true, aimodel_ );
    raytracer_.setOffsets( offsets_ );
    if ( !raytracer_.execute() )
    {
	errmsg_ = raytracer_.errMsg(); 
	return false;
    }

    if ( !synthgenbase_.doPrepare() )
    {
	errmsg_ = synthgenbase_.errMsg();
	return false;
    }
    return true;
}


bool RaySynthGenerator::doWork( od_int64 start, od_int64 stop, int nrthreads )
{
    ReflectivityModel refmodel;
    for ( int idoffset=start; idoffset<stop; idoffset++ )
    {	
	raytracer_.getReflectivity( idoffset, refmodel );
	synthgenbase_.setModel( refmodel );
	if ( !synthgenbase_.doWork() )
	{
	    errmsg_ = synthgenbase_.errMsg();
	    return Executor::ErrorOccurred();
	}
	outtrcs_ += new SeisTrc(synthgenbase_.result()); 
	offsetidxs_ += idoffset;
    }
    return true;
}


void RaySynthGenerator::fillPar( IOPar& par ) const
{

}


bool RaySynthGenerator::usePar( const IOPar& par ) 
{
    return true;
}

const SeisTrc* RaySynthGenerator::result( int offset ) const
{
    return offsetidxs_.validIdx(offset) ? outtrcs_[offsetidxs_[offset]] : 0;
}

}

