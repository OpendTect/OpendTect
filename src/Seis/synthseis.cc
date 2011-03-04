/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : 23-3-1996
 * FUNCTION : Wavelet
-*/

static const char* rcsID = "$Id: synthseis.cc,v 1.9 2011-03-04 09:18:05 cvsbruno Exp $";

#include "aimodel.h"
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
    , refmodel_(*new ReflectivityModel())		 
{
    outtrc_ = new SeisTrc;
}


Seis::SynthGeneratorBase::~SynthGeneratorBase()
{
    if ( waveletismine_ )
	delete wavelet_;
}


bool Seis::SynthGeneratorBase::setModel( const ReflectivityModel& refmodel )
{
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


bool Seis::SynthGeneratorBase::setOutSampling( const StepInterval<float>& si )
{
    outputsampling_ = si;
    outtrc_->reSize( si.nrSteps(), false );
    outtrc_->info().sampling = si;
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
    if ( !computeTrace( (float*)outtrc_->data().getComponent(0)->data() ) )
	return ErrorOccurred();

    return Finished(); 
}


bool Seis::SynthGeneratorBase::computeTrace( float* result ) 
{
    if ( refmodel_.isEmpty() ) 
	{ errmsg_ = "No reflectivity model found"; return false; }

    if ( !wavelet_ ) 
	{ errmsg_ = "No wavelet found"; return false; }

    int ns = outtrc_->size();
    outtrc_->zero();
    if ( ns < 2 )
	return false;

    TypeSet<float_complex> crefl;
    ReflectivitySampler sampler( refmodel_, outputsampling_, crefl );
    sampler.execute();
    samprefl_.erase();

    for ( int idx=0; idx<crefl.size(); idx++ )
    {
	float refval = crefl[idx].real();
	if ( mIsUdf( refval ) ) 
	    refval = 0;

	samprefl_ += refval;
    }

    const int wvltsz = wavelet_->size(); 
    const int wvltcs = wavelet_->centerSample();
    GenericConvolve( wvltsz, -wvltcs-1, wavelet_->samples(),
		     samprefl_.size(), 0, samprefl_.arr(),
		     ns, 0, result );
    return true;
}





Seis::SynthGenerator::SynthGenerator()
	: outtrc_(*new SeisTrc)
{
    init( 0, 0 );
}


Seis::SynthGenerator::SynthGenerator( const Wavelet& wvlt )
	: outtrc_(*new SeisTrc)
{
    init( 0, &wvlt );
}


Seis::SynthGenerator::SynthGenerator( const AIModel& mdl )
	: outtrc_(*new SeisTrc)
{
    init( &mdl, 0 );
}


Seis::SynthGenerator::SynthGenerator( const AIModel& mdl, const Wavelet& wvlt )
	: outtrc_(*new SeisTrc)
{
    init( &mdl, &wvlt );
}


Seis::SynthGenerator::~SynthGenerator()
{
    delete aimdl_; delete wvlt_;
    delete &outtrc_;
}


void Seis::SynthGenerator::init( const AIModel* mdl, const Wavelet* wvlt )
{
    aimdl_ = 0; inpaimdl_ = mdl;
    wvlt_ = 0; inpwvlt_ = wvlt;

    if ( inpaimdl_ )
	prepAIModel();
    if ( inpwvlt_ )
	prepWavelet();
}


void Seis::SynthGenerator::prepAIModel()
{
    delete aimdl_; aimdl_ = 0;
    if ( !inpaimdl_ ) return;
    aimdl_ = new AIModel( *inpaimdl_ );
    aimdl_->antiAlias();
}


void Seis::SynthGenerator::prepWavelet()
{
    delete wvlt_; wvlt_ = 0;
    if ( !inpwvlt_ ) return;
    wvlt_ = new Wavelet( *inpwvlt_ );
}


SamplingData<float> Seis::SynthGenerator::getDefOutSampling(
		const AIModel& aimod, const Wavelet& wvlt, int& ns )
{
    SamplingData<float> sd( 0, wvlt.sampleRate() );
    float tend = aimod.endTime();
    sd.snap( tend );
    ns = sd.nearestIndex( tend );
    return sd;
}


void Seis::SynthGenerator::setOutSampling( const SamplingData<float>& sd,
					   int ns )
{
    outtrc_.reSize( ns, false );
    outtrc_.info().sampling = sd;
}


void Seis::SynthGenerator::generate( const Wavelet& wvlt )
{
    inpwvlt_ = &wvlt;
    prepWavelet();
    generate();
}


void Seis::SynthGenerator::generate( const AIModel& mdl )
{
    inpaimdl_ = &mdl;
    prepAIModel();
    generate();
}


void Seis::SynthGenerator::generate( const AIModel& mdl, const Wavelet& wvlt )
{
    inpaimdl_ = &mdl; inpwvlt_ = &wvlt;
    prepAIModel(); prepWavelet();
    generate();
}


void Seis::SynthGenerator::generate()
{
    if ( !wvlt_ || !aimdl_ )
	return;

    int ns = outtrc_.size();
    if ( ns < 1 )
    {
	outtrc_.info().sampling = getDefOutSampling( *inpaimdl_, *inpwvlt_,
						     ns );
	if ( ns < 1 ) ns = 1;
	outtrc_.reSize( ns, false );
    }
    outtrc_.zero();
    if ( ns < 2 )
	return;

    TypeSet<float> refl;
    aimdl_->getReflectivity( outtrc_.info().sampling, refl );

    const int wvltsz = wvlt_->size(); const int wvltcs = wvlt_->centerSample();
    float* trcarr = (float*)outtrc_.data().getComponent(0)->data();
    GenericConvolve( wvltsz, -wvltcs-1, wvlt_->samples(),
		     refl.size(), 0, refl.arr(),
		     ns, 0, trcarr );
}



