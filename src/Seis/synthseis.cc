/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : 23-3-1996
 * FUNCTION : Wavelet
-*/

static const char* rcsID = "$Id: synthseis.cc,v 1.3 2010-12-07 16:15:43 cvsbert Exp $";

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
    SamplingData<float> sd = aimod.timeSampling();
    float zend = sd.start + (aimod.modelData().size() - 1) * sd.step;
    sd.step = wvlt.sampleRate();
    zend = sd.snap( zend );
    ns = sd.nearestIndex( zend ) + 1 + wvlt.size();
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
	{ outtrc_.zero(); return; }

    int ns = outtrc_.size();
    if ( ns < 1 )
    {
	outtrc_.info().sampling = getDefOutSampling( *inpaimdl_, *inpwvlt_,
						     ns );
	if ( ns < 1 ) return;
	outtrc_.reSize( ns, false );
	if ( ns < 2 )
	    { outtrc_.zero(); return; }
    }

    TypeSet<float> denserefl; aimdl_->getReflectivity( denserefl );
    TypeSet<float> refl;
    const SamplingData<float> aimdlsd( aimdl_->timeSampling() );
    const int aimdlns = denserefl.size();
    const int startsamp = outtrc_.nearestSample( aimdlsd.start );
    const int endsamp = outtrc_.nearestSample( aimdlsd.atIndex(aimdlns-1) );
    for ( int isamp=startsamp; isamp<endsamp; isamp++ )
    {
	int nearsamp = aimdlsd.nearestIndex( outtrc_.samplePos(isamp) );
	if ( nearsamp < 0 ) nearsamp = 0;
	else if ( nearsamp >= aimdlns ) nearsamp = aimdlns-1;
	refl += denserefl[nearsamp];
    }

    float* trcarr = (float*)outtrc_.data().getComponent(0)->data();
    GenericConvolve( refl.size(), -startsamp, refl.arr(),
	    	     wvlt_->size(), wvlt_->centerSample(), wvlt_->samples(),
		     ns, 0, trcarr );
}
