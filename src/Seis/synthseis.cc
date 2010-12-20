/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : 23-3-1996
 * FUNCTION : Wavelet
-*/

static const char* rcsID = "$Id: synthseis.cc,v 1.6 2010-12-20 14:04:05 cvsbert Exp $";

#include "synthseis.h"
#include "wavelet.h"
#include "aimodel.h"
#include "seistrc.h"
#include "survinfo.h"
#include "genericnumer.h"


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
    GenericConvolve( wvltsz, -wvltcs, wvlt_->samples(),
		     refl.size(), 0, refl.arr(),
		     ns, 0, trcarr );
}
