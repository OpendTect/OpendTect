/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : 9-3-1999
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "spectrogram.h"
#include "arrayndimpl.h"

    
Spectrogram::Spectrogram()
    : fft_( Fourier::CC::createDefault() )
    , tempin_( 0 )
    , tempout_( 0 )
{}    


Spectrogram::~Spectrogram() 
{ 
    delete fft_;
    delete tempin_; 
    delete tempout_; 
}


bool Spectrogram::init()
{
    if ( tempin_ )
    {
	delete tempin_;
	delete tempout_;
    }

    const ArrayNDInfo& info = fft_->getInputInfo();
    tempin_ = ArrayNDImpl<float_complex>::create( info );
    tempout_= ArrayNDImpl<float_complex>::create( info );
 
    fft_->setInput( tempin_->getData() );
    fft_->setOutput( tempout_->getData() );

    return true;
}


bool Spectrogram::setInputInfo( const ArrayNDInfo& ni )
{ return fft_->setInputInfo( ni ); }


const ArrayNDInfo& Spectrogram::getInputInfo() const 
{ return fft_->getInputInfo(); }


bool Spectrogram::transform( const ArrayND< float > &in, ArrayND< float > &out )
{
    const ArrayNDInfo& info = fft_->getInputInfo();

    float_complex* tindata = tempin_->getData();
    const float* indata = in.getData();
    unsigned long size = info.getTotalSz();

    for ( unsigned int idx=0; idx<size; idx++ )
	tindata[idx] = indata[idx];

    if ( !fft_->run(true) ) return false;

    float_complex* toutdata = tempout_->getData();
    float* outdata = out.getData();

    for ( unsigned int idx=0; idx<size; idx++ )
    {
	float val = abs( toutdata[idx] );
	outdata[idx] = val*val;
    }

    return true;
}


bool Spectrogram::transform( const ArrayND< float_complex > &in, 
			     ArrayND< float_complex > &out )
{
    fft_->setInputInfo( in.info() );
    fft_->setInput( in.getData() );
    fft_->setOutput( out.getData() );
    if ( !fft_->run(true) ) return false;

    unsigned long size = in.info().getTotalSz();
    float_complex* data = out.getData();

    for ( unsigned int idx=0; idx<size; idx++ )
    {
	float val = abs( data[idx] );
	data[idx] = val*val;
    }

    return true;
}



