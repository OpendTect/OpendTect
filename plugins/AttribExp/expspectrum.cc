/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "expspectrum.h"
#include "attribprovider.h"
#include "seistrc.h"
#include "simpnumer.h"
#include "samplfunc.h"
#include "genericnumer.h"
#include "arrayndalgo.h"

#include <stdio.h>

TraceSpectrumAttrib::TraceSpectrumAttrib( Parameters* param )
    : windowtype( (ArrayNDWindow::WindowType) ((int) param->window) )
    , AttribCalc( new TraceSpectrumAttrib::Task( *this ) )
{ 

    AttribInputSpec* spec = new AttribInputSpec;
    spec->setDesc(
		"Real data on which the Frequency spectrum should be measured");
    inputspec += spec;

    if ( param->complex )
    {
	spec = new AttribInputSpec;
	spec->setDesc(
	    "Imag data on which the Frequency spectrum should be measured");
	inputspec += spec;
    }

    param->fillDefStr( desc );
    delete param;
    prepareInputs();
}


TraceSpectrumAttrib::~TraceSpectrumAttrib()
{
}


bool TraceSpectrumAttrib::init()
{
    return AttribCalc::init();
}


TraceSpectrumAttrib::Task::~Task()
{
    delete timedomain;
    delete freqdomain;
    delete window;
}


int TraceSpectrumAttrib::Task::nextStep()
{
    TraceSpectrumAttrib::Task::Input* inp =
				(TraceSpectrumAttrib::Task::Input*)input;

    const SeisTrc* realtrc = inp->realtrc;
    const SeisTrc* imagtrc = inp->imagtrc;

    const int reattrib = inp->reattrib;
    const int imattrib = inp->imattrib;

    Array1DInfoImpl size(nrtimes);

    if ( !timedomain || timedomain->info() != size )
    {
	delete timedomain;
	delete freqdomain;
	delete window;

	timedomain = new Array1DImpl<float_complex>( nrtimes );
	freqdomain = new Array1DImpl<float_complex>( nrtimes );
	window = new ArrayNDWindow( size, false, calculator.windowtype );
	fft.setInputInfo(size);
	fft.setDir(true);
	fft.init();
    }

    for ( int idx=0; idx<nrtimes; idx++ )
    {
	const float curt = t1 + idx*step;
	float real = realtrc ? realtrc->getValue( curt, reattrib ) : 0;
	float imag = imagtrc ? -imagtrc->getValue( curt, imattrib ) : 0;

	timedomain->set( idx,float_complex( real, imag ));
    }

    window->apply( timedomain );

    fft.transform(*timedomain, *freqdomain);

    for ( int idx=0; idx<nrtimes; idx++ )
    {
	float_complex val = freqdomain->get(idx);
	float real = val.real();
	float imag = val.imag();

	if ( power )
	    power[idx] = real*real+imag*imag;

	if ( realout ) realout[idx] = real;
	if ( imagout ) imagout[idx] = imag;
    }

    return 0;
}


AttribCalc::Task* TraceSpectrumAttrib::Task::clone() const
{ return new TraceSpectrumAttrib::Task(calculator); }


bool TraceSpectrumAttrib::Task::Input::set(const BinID& pos, 
	const ObjectSet<AttribProvider>& inp, const TypeSet<int>& attrib,
			     const TypeSet<float*>&)
{
    realtrc = inp[0]->getTrc( pos.inl, pos.crl ); 

    if ( calculator.nrInputs() == 2 )
    {
	imagtrc = inp[1]->getTrc( pos.inl, pos.crl );
	if ( !imagtrc ) return false;
	imattrib = inp[1]->attrib2component( attrib[1] ); 
    }

    reattrib = inp[0]->attrib2component( attrib[0] ); 
    return realtrc;
} 


