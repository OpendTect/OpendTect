/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : 23-3-1996
 * FUNCTION : Wavelet
-*/

static const char* rcsID = "$Id: synthseis.cc,v 1.10 2011-03-11 13:42:10 cvsbruno Exp $";

#include "arrayndimpl.h"
#include "fourier.h"
#include "genericnumer.h"
#include "reflectivitymodel.h"
#include "reflectivitysampler.h"
#include "seistrc.h"
#include "synthseis.h"
#include "survinfo.h"
#include "wavelet.h"

mImplFactory( Seis::SynthGeneratorBase, Seis::SynthGeneratorBase::factory );

Seis::SynthGeneratorBase::SynthGeneratorBase()
    : Executor("Fast Synth Generator")
    , wavelet_(0)
    , fft_(0)
    , fftsz_(0)			   
    , freqwavelet_(0)
{
    outtrc_ = new SeisTrc;
}


Seis::SynthGeneratorBase::~SynthGeneratorBase()
{
    if ( waveletismine_ )
	delete wavelet_;

    delete freqwavelet_;
}


bool Seis::SynthGeneratorBase::setModel( const ReflectivityModel& refmodel )
{
    refmodel_.erase();
    refmodel_.copy( refmodel );
    return true;
}


bool Seis::SynthGeneratorBase::setWavelet( Wavelet* wvlt, OD::PtrPolicy pol )
{
    if ( !wvlt ) 
	{ errmsg_ = "No valid wavelet given"; return false; }
    if ( pol == OD::CopyPtr )
    {
	mDeclareAndTryAlloc(float*, newdata, float[wvlt->size()] );
	if ( !newdata ) return false;
	MemCopier<float> copier( wavelet_->samples(), newdata, wvlt->size() );
	copier.execute();
    }
    else 
    {
	wavelet_ = wvlt;
    }
    waveletismine_ = pol != OD::UsePtr;

    return true;
}


void Seis::SynthGeneratorBase::setConvolDomain( bool fourier )
{
    if ( fourier == ((bool) fft_ ) )
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



bool Seis::SynthGeneratorBase::setOutSampling( const StepInterval<float>& si )
{
    outputsampling_ = si;
    outtrc_->reSize( si.nrSteps(), false );
    outtrc_->info().sampling = si;
    fftsz_ = si.nrSteps(); //TODO fast sz
    return true;
}


void Seis::SynthGeneratorBase::fillPar( IOPar& par ) const 
{
}


bool Seis::SynthGeneratorBase::usePar( const IOPar& par )
{
    return true;
}


int Seis::SynthGeneratorBase::nextStep()
{
    if ( refmodel_.isEmpty() ) 
	{ errmsg_ = "No reflectivity model found"; return false; }

    if ( !wavelet_ ) 
	{ errmsg_ = "No wavelet found"; return false; }

    if ( !computeTrace( (float*)outtrc_->data().getComponent(0)->data() ) )
	return ErrorOccurred();

    return Finished(); 
}


bool Seis::SynthGeneratorBase::computeTrace( float* result ) 
{
    outtrc_->zero();
    if ( outputsampling_.nrSteps() < 2 )
	return false;

    const int wvltsz = wavelet_->size(); 
    if ( wvltsz < 2 )
	return false;

    cresamprefl_.erase();
    ReflectivitySampler sampler( refmodel_, outputsampling_, cresamprefl_ );
    sampler.execute();

    if (fft_ ) 
	FFTConvolve( result);
    else
	genericConvolve( result );

    return true;
}


bool Seis::SynthGeneratorBase::FFTConvolve( float* result )
{
    if ( !fft_ ) return false;

    const int wvltsz = wavelet_->size();
    float_complex* cres = new float_complex[ fftsz_ ];

    delete freqwavelet_;
    freqwavelet_ = new Array1DImpl<float_complex>( fftsz_ );
    for ( int idx=0; idx<fftsz_; idx++ )
    {
	float val = 0; 
	if ( idx < wvltsz ) 
	    val = wavelet_->samples()[idx];
	freqwavelet_->set( idx, val );
    }

    fft_->setInputInfo( freqwavelet_->info() );
    fft_->setDir( true );
    fft_->setNormalization( false );
    fft_->setInput( freqwavelet_->arr() );
    fft_->setOutput( freqwavelet_->arr() );
    fft_->run( true );
    for ( int idx=0; idx<fftsz_; idx++ )
	cres[idx] = cresamprefl_[idx] * freqwavelet_->get( idx ); 

    fft_->setDir( false );
    fft_->setNormalization( true );
    fft_->setInput( cres );
    fft_->setOutput( cres );
    fft_->run( false );
    for ( int idx=0; idx<cresamprefl_.size(); idx++ )
	result[idx] = cres[idx].real();
    
    return true;
}


bool Seis::SynthGeneratorBase::genericConvolve( float* result )
{
    int ns = outtrc_->size();
    TypeSet<float> refs; getSampledReflectivities( refs );
    const int wvltcs = wavelet_->centerSample();
    const int wvltsz = wavelet_->size();
    GenericConvolve( wvltsz, -wvltcs-1, wavelet_->samples(),
		     refs.size(), 0, refs.arr(),
		     ns, 0, result );
    return true;
}


void Seis::SynthGeneratorBase::getSampledReflectivities( 
					    TypeSet<float>& refs ) const
{
    for ( int idx=0; idx<cresamprefl_.size(); idx++ )
	refs += cresamprefl_[idx].real();
}

