/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : 23-3-1996
 * FUNCTION : Wavelet
-*/

static const char* rcsID = "$Id: synthseis.cc,v 1.1 2010-11-11 16:16:45 cvsbert Exp $";

#include "synthseis.h"
#include "wavelet.h"
#include "aimodel.h"
#include "seistrc.h"


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
    //TODO implement.
    // Also resize the output trace and set sampling right.
    // Then users of the class can then immediately look at the that if needed.
}


void Seis::SynthGenerator::generate( const Wavelet& wvlt ) const
{
    if ( !wvlt_ )
	{ outtrc_.zero(); return; }
    //TODO implement
}


void Seis::SynthGenerator::generate( const AIModel& mdl ) const
{
    if ( !aimdl_ )
	{ outtrc_.zero(); return; }
    //TODO implement
}
