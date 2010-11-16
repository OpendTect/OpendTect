/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : 23-3-1996
 * FUNCTION : Wavelet
-*/

static const char* rcsID = "$Id: synthseis.cc,v 1.2 2010-11-16 09:49:10 cvsbert Exp $";

#include "synthseis.h"
#include "wavelet.h"
#include "aimodel.h"
#include "seistrc.h"
#include "survinfo.h"
#include "genericnumer.h"


Seis::SynthGenerator::SynthGenerator( const Wavelet& wvlt )
	: wvlt_(&wvlt)
	, aimdl_(0)
	, outtrc_(*new SeisTrc)
{
    init();
}


Seis::SynthGenerator::SynthGenerator( const AIModel& mdl )
	: wvlt_(0)
	, aimdl_(&mdl)
	, outtrc_(*new SeisTrc)
{
    init();
}


Seis::SynthGenerator::~SynthGenerator()
{
    delete &outtrc_;
}


void Seis::SynthGenerator::init()
{
}


void Seis::SynthGenerator::generate( const Wavelet& wvlt )
{
    if ( !aimdl_ )
	{ outtrc_.zero(); return; }
    wvlt_ = &wvlt;
    generate();
}


void Seis::SynthGenerator::generate( const AIModel& mdl )
{
    if ( !wvlt_ )
	{ outtrc_.zero(); return; }
    aimdl_ = &mdl;
    generate();
}


void Seis::SynthGenerator::generate()
{
    if ( !wvlt_ || !aimdl_ )
	{ outtrc_.zero(); return; }

    SamplingData<float>& sdout = outtrc_.info().sampling;
    SamplingData<float> aisd = aimdl_->timeSampling();
    sdout.start = aisd.start;
    SI().snapZ( sdout.start );
    sdout.step = wvlt_->sampleRate();
    float zend = aisd.start + (aimdl_->modelData().size() - 1) * aisd.step;
    SI().snapZ( zend );
    const int ns = sdout.nearestIndex( zend ) + 1;
    outtrc_.reSize( ns, false );

    TypeSet<float> refl;
    float ai0 = aimdl_->aiAt( AIModel::Time, sdout.atIndex(0) );
    for ( int isamp=0; isamp<ns; isamp++ )
    {
	const float t = sdout.atIndex( isamp );
	const float ai1 = aimdl_->aiAt( AIModel::Time, t );
	refl += (ai1-ai0) / (ai1+ai0);
	ai0 = ai1;
    }

    float* trcarr = (float*)outtrc_.data().getComponent(0)->data();
    GenericConvolve( ns, 0, refl.arr(),
	    	     wvlt_->size(), wvlt_->centerSample(), wvlt_->samples(),
		     ns, 0, trcarr );
}
